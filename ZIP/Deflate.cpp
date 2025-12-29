#include "Deflate.h"
#include "BitStream.h"
#include "HuffmanConstants.h"
#include "CodeLenghts.h"
#include <stdint.h>
#include <vector>

/*
Обусловимся, что работаем ТОЛЬКО с количеством файлов на ЭТОМ диске
Можно брать и количество файлов всего, но в современных zip архивах это одно и то же
*/

std::vector<CodeLengthsForCodeLengths> alphabetLengthDecode;
std::vector<TableOfCodeLenghts> huffmanDecode;
std::vector<TableOfCodeLenghts> distDecode;

bool decodeStatic(BitArray& bits, uint32_t& i, uint16_t& byte, uint16_t& copySize, uint8_t& bitsCount, bool& _newBlock, 
                uint32_t& index, uint8_t* resultBuf) {
    byte = (byte << 1) | bits[i];
    bitsCount++;
    uint16_t commandByte = byte + 256;
    switch (bitsCount) {
    case 5: {
        // Если считали 5 битов и задана длина копирования - это длина смещения
        if (copySize != 0) {
            uint8_t addBitsLen = staticDeflate_ExtraBitsDistance[byte];
            uint16_t lenByte = staticDeflate_LenOfDistance[byte];

            uint16_t addBits = 0;

            for (int j = 0; j < addBitsLen; j++)
                addBits |= (bits[++i] << j);

            lenByte += addBits;

            uint16_t backIndex = index - lenByte;
            for (int j = 0; j < copySize; j++) {
                resultBuf[index] = resultBuf[backIndex];
                index++;
                backIndex++;
            }

            byte = bitsCount = copySize = 0;
        }
        break;
    }
    case 7: {
        // Диапазон команд длины
        if (257 <= commandByte && commandByte <= 279) {
            copySize = staticDeflate_LengthOfCommand[commandByte - 257];
            uint16_t addBits = 0;
            int addBitsLen = 0;

            if (265 <= commandByte && commandByte <= 268) copySize += bits[++i];
            else if (269 <= commandByte && commandByte <= 272) addBitsLen = 2;
            else if (273 <= commandByte && commandByte <= 276) addBitsLen = 3;
            else if (commandByte >= 277) addBitsLen = 4;
            else addBits = 0;

            for (int j = 0; j < addBitsLen; j++)
                addBits |= (bits[++i] << j);

            copySize += addBits;
            byte = bitsCount = 0;
        }
        else if (commandByte == 256) {
            _newBlock = true;
            byte = bitsCount = 0;
        }
        break;
    }
    case 8: {
        // Диапазон статических кодов для символов 0-143
        if (48 <= byte && byte <= 191) {
            resultBuf[index] = byte - 0x30;
            byte = bitsCount = copySize = 0;
            index++;
        }

        // Расширенный диапазон команд длины
        if (448 <= commandByte && commandByte <= 453) {
            uint16_t sym = 280 + (commandByte - 448);
            copySize = staticDeflate_LengthOfCommand[sym - 257];
            uint16_t addBits = 0;
            int addBitsLen = 0;

            if (sym == 280) addBitsLen = 4;
            if (281 <= sym && sym <= 284) addBitsLen = 5;

            for (int j = 0; j < addBitsLen; j++)
                addBits |= (bits[++i] << j);

            copySize += addBits;
            byte = bitsCount = 0;
        }
        break;
    }
    case 9: {
        // Диапазон статических кодов для символов 144-255
        if (400 <= byte && byte <= 511) {
            resultBuf[index] = 144 + (byte - 400);
            byte = bitsCount = copySize = 0;
            index++;
        }
        break;
    }
    }
	return true;
}

bool decodeDynamic(BitArray& bits, uint32_t& i, uint16_t& byte, uint16_t& copySize, uint8_t& bitsCount, bool& _newBlock,
    uint32_t& index, uint8_t* resultBuf, LocalFileHeader& localFileHeader) {
    alphabetLengthDecode.clear();
    huffmanDecode.clear();
    distDecode.clear();

    uint16_t HLIT = readBits(bits, &i, 5) + 257;
    uint16_t HDIST = readBits(bits, &i, 5) + 1;
    uint16_t HCLEN = readBits(bits, &i, 4) + 4;

    CodeLengthsForCodeLengths codesLenghts[19];

    for (int j = 0; j < 19; j++) {
        codesLenghts[dynamicDeflate_HowToReadCodeLengthsForCodeLengths[j]].numInOrder = dynamicDeflate_HowToReadCodeLengthsForCodeLengths[j];
        codesLenghts[dynamicDeflate_HowToReadCodeLengthsForCodeLengths[j]].numOfBits = (j < HCLEN ? readBits(bits, &i, 3) : 0);
    }

    qsort(codesLenghts, 19, sizeof(CodeLengthsForCodeLengths), codeLengthsForCodeLengthsCompare);

    uint8_t _arrBitsCount[16] = { 0 };
    uint16_t nextCodes[16] = { 0 };

    uint16_t code = 0;
    // Считаем количество символов с заданным количеством битов
    for (int bits = 1; bits <= 15; bits++) {
        for (int j = 0; j < 19; j++) {
            if (codesLenghts[j].numOfBits == bits) _arrBitsCount[bits]++;
        }
        // Высчитываем первый код для каждой длины
        code = (code + _arrBitsCount[bits - 1]) << 1;
        nextCodes[bits] = code;
    }
    // Заполняем таблицу Хаффмана
    for (int j = 0; j < 19; j++) {
        uint8_t bits = codesLenghts[j].numOfBits;
        if (bits) {
            codesLenghts[j].numOfCode = nextCodes[bits]++;
            alphabetLengthDecode.emplace_back(codesLenghts[j]);
        }
    }

    //Резервируем HLIT структур для вектора
    huffmanDecode.reserve(HLIT);
    auto tableForCreatingHuffmanCode = std::make_unique<TableOfCodeLenghts[]>(HLIT);

    for (uint16_t countHLIT = 0; countHLIT < HLIT; ) {
        TableOfCodeLenghts tempTable;
        for (uint8_t bitInByte = 0; bitInByte < sizeof(uint32_t) * 8; bitInByte++) {
            byte = (byte << 1) | bits[i++];
            bitsCount++;

            CodeLengthsForCodeLengths tempForSearch;
            tempForSearch.numOfBits = bitsCount;
            tempForSearch.numOfCode = byte;

            auto it = std::find(alphabetLengthDecode.begin(), alphabetLengthDecode.end(), tempForSearch);
            if (it != alphabetLengthDecode.end()) {
                switch ((*it).numInOrder) {
                case 16: {
                    uint8_t countOfSym = 3 + readBits(bits, &i, 2);
                    auto lastLen = huffmanDecode.back().numOfBits;
                    for (uint8_t _syms = 0; _syms < countOfSym; _syms++) {
                        tempTable.numOfBits = lastLen;
                        tempTable.symbol = countHLIT + _syms;
                        tableForCreatingHuffmanCode[countHLIT] = tempTable;
                    }
                    countHLIT += countOfSym;
                    break;
                }
                case 17:
                case 18: {
                    uint8_t countOfSym = ((*it).numInOrder == 17 ? 3 + readBits(bits, &i, 3)
                        : 11 + readBits(bits, &i, 7));
                    for (uint8_t _syms = 0; _syms < countOfSym; _syms++) {
                        tempTable.numOfBits = 0;
                        tempTable.symbol = countHLIT + _syms;
                        tableForCreatingHuffmanCode[countHLIT] = tempTable;
                    }
                    countHLIT += countOfSym;
                    break;
                }
                default: {
                    tempTable.numOfBits = (*it).numInOrder;
                    tempTable.symbol = countHLIT;
                    tableForCreatingHuffmanCode[countHLIT] = tempTable;
                    countHLIT++;
                    break;
                }
                }
                byte = bitsCount = 0;
                break;
            }
        }
    }

    qsort(tableForCreatingHuffmanCode.get(), HLIT, sizeof(TableOfCodeLenghts), tableOfCodeLenghtsCompare);

    uint8_t _arrBitsDataCount[17] = { 0 };
    uint16_t nextDataCodes[17] = { 0 };

    code = 0;
    for (int bits = 1; bits <= 16; bits++) {
        for (int j = 0; j < HLIT; j++) {
            if (tableForCreatingHuffmanCode[j].numOfBits == bits) _arrBitsDataCount[bits]++;
        }

        code = (code + _arrBitsDataCount[bits - 1]) << 1;
        nextDataCodes[bits] = code;
    }

    for (int j = 0; j < HLIT; j++) {
        uint8_t bits = tableForCreatingHuffmanCode[j].numOfBits;
        if (bits) {
            tableForCreatingHuffmanCode[j].numOfCode = nextDataCodes[bits]++;
            huffmanDecode.emplace_back(tableForCreatingHuffmanCode[j]);
        }
    }

    auto tableForCreatingHuffmanDistance = std::make_unique<TableOfCodeLenghts[]>(HDIST);

    for (uint16_t countHDIST = 0; countHDIST < HDIST; ) {
        TableOfCodeLenghts tempTable;
        for (uint8_t bitInByte = 0; bitInByte < sizeof(uint32_t) * 8; bitInByte++) {
            byte = (byte << 1) | bits[i++];
            bitsCount++;

            CodeLengthsForCodeLengths tempForSearch;
            tempForSearch.numOfBits = bitsCount;
            tempForSearch.numOfCode = byte;

            auto it = std::find(alphabetLengthDecode.begin(), alphabetLengthDecode.end(), tempForSearch);
            if (it != alphabetLengthDecode.end()) {
                switch ((*it).numInOrder) {
                case 16: {
                    uint8_t countOfSym = 3 + readBits(bits, &i, 2);
                    auto lastLen = huffmanDecode.back().numOfBits;
                    for (uint8_t _syms = 0; _syms < countOfSym; _syms++) {
                        tempTable.numOfBits = lastLen;
                        tempTable.symbol = countHDIST + _syms;
                        tableForCreatingHuffmanDistance[countHDIST] = tempTable;
                    }
                    countHDIST += countOfSym;
                    break;
                }
                case 17:
                case 18: {
                    uint8_t countOfSym = ((*it).numInOrder == 17 ? 3 + readBits(bits, &i, 3)
                        : 11 + readBits(bits, &i, 7));
                    for (uint8_t _syms = 0; _syms < countOfSym; _syms++) {
                        tempTable.numOfBits = 0;
                        tempTable.symbol = countHDIST + _syms;
                        tableForCreatingHuffmanDistance[countHDIST] = tempTable;
                    }
                    countHDIST += countOfSym;
                    break;
                }
                default: {
                    tempTable.numOfBits = (*it).numInOrder;
                    tempTable.symbol = countHDIST;
                    tableForCreatingHuffmanDistance[countHDIST] = tempTable;
                    countHDIST++;
                    break;
                }
                }
                byte = bitsCount = 0;
                break;
            }
        }
    }

    qsort(tableForCreatingHuffmanDistance.get(), HDIST, sizeof(TableOfCodeLenghts), tableOfCodeLenghtsCompare);

    uint8_t _arrBitsDistCount[17] = { 0 };
    uint16_t nextDistCodes[17] = { 0 };

    code = 0;
    for (int bits = 1; bits <= 16; bits++) {
        for (int j = 0; j < HDIST; j++) {
            if (tableForCreatingHuffmanDistance[j].numOfBits == bits) _arrBitsDistCount[bits]++;
        }

        code = (code + _arrBitsDistCount[bits - 1]) << 1;
        nextDistCodes[bits] = code;
    }

    for (int j = 0; j < HDIST; j++) {
        uint8_t bits = tableForCreatingHuffmanDistance[j].numOfBits;
        if (bits) {
            tableForCreatingHuffmanDistance[j].numOfCode = nextDistCodes[bits]++;
            distDecode.emplace_back(tableForCreatingHuffmanDistance[j]);
        }
    }

    while (i < localFileHeader.compressSize * 8) {
        byte = (byte << 1) | bits[i++];
        bitsCount++;

        TableOfCodeLenghts tempForSearch;
        tempForSearch.numOfBits = bitsCount;
        tempForSearch.numOfCode = byte;

        auto it = std::find(huffmanDecode.begin(), huffmanDecode.end(), tempForSearch);
        if (it != huffmanDecode.end()) {
            if ((*it).symbol < 256) {
                resultBuf[index] = (*it).symbol;
                index++;
            }
            else if ((*it).symbol == 256) {
                _newBlock = true;
                break;
            }
            else {
                uint8_t localIndex = (*it).symbol - 257;
                uint32_t lenOfRepeat = dynamicDeflate_LengthOfBaseDistance[localIndex]
                    + readBits(bits, &i, dynamicDeflate_CountOfExtraBitsDistance[localIndex]);

                uint32_t offsetOfRepeat = 0;

                byte = bitsCount = 0;
                while (1) {
                    byte = (byte << 1) | bits[i++];
                    bitsCount++;

                    tempForSearch.numOfBits = bitsCount;
                    tempForSearch.numOfCode = byte;

                    auto _it = std::find(distDecode.begin(), distDecode.end(), tempForSearch);
                    if (_it != distDecode.end()) {
                        offsetOfRepeat = dynamicDeflate_BaseDistForAlphabetOfDist[(*_it).symbol]
                            + readBits(bits, &i, dynamicDeflate_ExtraBitsForAlphabetOfDist[(*_it).symbol]);
                        byte = bitsCount = 0;
                        break;
                    }
                }

                uint32_t backIndex = index - offsetOfRepeat;
                for (uint32_t j = 0; j < lenOfRepeat; j++) {
                    resultBuf[index] = resultBuf[backIndex];
                    index++;
                    backIndex++;
                }

            }
            byte = bitsCount = 0;
        }
    }
	return true;
}