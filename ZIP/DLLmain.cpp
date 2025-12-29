#define _CRT_SECURE_NO_WARNINGS

#include "../No-ZIP/DeclarationOfFile.h"
#include "Token.h"
#include "BitStream.h"
#include "HuffmanConstants.h"
#include "files.h"
#include "CodeLenghts.h"
#include "Deflate.h"

void Cleanup(ZipFile* file) {
    if (file->CD) {
        for (uint16_t i = 0; i < file->endCD.numOfCDRecordsOnThisDisk; i++) {
            delete[] file->CD[i].fileName;
            delete[] file->CD[i].extraField;
            delete[] file->CD[i].comment;
        }
        delete[] file->CD;
        file->CD = nullptr;
    }

    if (file->endCD.comment) {
        delete[] file->endCD.comment;
        file->endCD.comment = nullptr;
    }

    if (file->file) {
        fclose(file->file);
        file->file = NULL;
    }
}

void Cleanup(LocalFileHeader* localFileHeader) {
    if (localFileHeader->fileName) {
        delete[] localFileHeader->fileName;
        localFileHeader->fileName = nullptr;
    }

    if (localFileHeader->extraField) {
        delete[] localFileHeader->extraField;
        localFileHeader->extraField = nullptr;
    }
}

extern "C" {

    __declspec(dllexport) FileToken OpenArchive(const wchar_t* path) {

        ZipFile temp;
        memset(&temp, 0, sizeof(temp));

        temp.file = _wfopen(path, L"rb");
        if (!temp.file) return -1;

        if (fseek(temp.file, -(long)_sizeof(EndOfCentralDirectory, 1), SEEK_END)) {
            Cleanup(&temp);
            return -2;
        }

        if (fread(&temp.endCD, _sizeof(EndOfCentralDirectory, 1), 1, temp.file) != 1) {
            Cleanup(&temp);
            return -3;
        }

        if (temp.endCD.lenOfComment > 0) {
            temp.endCD.comment = new uint8_t[temp.endCD.lenOfComment + 1]{};

            if (fread(temp.endCD.comment, temp.endCD.lenOfComment, 1, temp.file) != 1) {
                Cleanup(&temp);
                return -4;
            }
        }

        temp.CD = new CentralDirectory[temp.endCD.numOfCDRecordsOnThisDisk];
        auto blockOfCD = std::make_unique<uint8_t[]>(temp.endCD.sizeOfCD);

        if (fseek(temp.file, temp.endCD.offsetOfCD, SEEK_SET) ||
            fread(blockOfCD.get(), temp.endCD.sizeOfCD, 1, temp.file) != 1) {
            Cleanup(&temp);
            return -5;
        }

        try {

            uint8_t* offset = blockOfCD.get();
            for (uint16_t i = 0; i < temp.endCD.numOfCDRecordsOnThisDisk; i++) {
                memcpy(&temp.CD[i], offset, _sizeof(CentralDirectory, 3));
                offset += _sizeof(CentralDirectory, 3);

                temp.CD[i].fileName = new uint8_t[temp.CD[i].lenOfFileName + 1]{};
                temp.CD[i].extraField = new uint8_t[temp.CD[i].lenOfExtraField]{};
                temp.CD[i].comment = new uint8_t[temp.CD[i].lenOfComment + 1]{};

                memcpy(temp.CD[i].fileName, offset, temp.CD[i].lenOfFileName);
                offset += temp.CD[i].lenOfFileName;
                memcpy(temp.CD[i].extraField, offset, temp.CD[i].lenOfExtraField);
                offset += temp.CD[i].lenOfExtraField;
                memcpy(temp.CD[i].comment, offset, temp.CD[i].lenOfComment);
            }
        }
        catch (...) {
            Cleanup(&temp);
            return -6;
        }

        if (freeTokens.empty()) {
            files.push_back(std::move(temp));
            return files.size() - 1;
        }

        auto freeToken = freeTokens.front();
        files[freeToken] = std::move(temp);
        freeTokens.pop_front();

        return freeToken;
    }

    __declspec(dllexport) size_t GetFileCount(FileToken _token) {
        if (!CheckToken(_token)) return 0;
        return files[_token].endCD.numOfCDRecordsOnThisDisk;
    }

    __declspec(dllexport) FileSpecification* GetListOfFiles(FileToken _token) {
        if (!CheckToken(_token)) return nullptr;

        try {
            auto result = new FileSpecification[files[_token].endCD.numOfCDRecordsOnThisDisk];

            for (uint16_t i = 0; i < files[_token].endCD.numOfCDRecordsOnThisDisk; i++) {
                memcpy(result[i].fileName, files[_token].CD[i].fileName, files[_token].CD[i].lenOfFileName + 1);
            }

            return result;
        }
        catch (...) {
            return nullptr;
        }
    }

    __declspec(dllexport) bool ExportFile(FileToken _token, const char* filename, const wchar_t* path) {
        if (!CheckToken(_token)) return false;
        for (uint16_t i = 0; i < files[_token].endCD.numOfCDRecordsOnThisDisk; i++) {
            if (!strcmp((const char*)files[_token].CD[i].fileName, filename)) {
                if (fseek(files[_token].file, files[_token].CD[i].offsetOfLocalHeader, SEEK_SET)) return false;

                LocalFileHeader localFileHeader = { 0 };
                if (fread(&localFileHeader, _sizeof(LocalFileHeader, 2), 1, files[_token].file) != 1) return false;

                try {
                    if (localFileHeader.lenOfFileName) {
                        localFileHeader.fileName = new uint8_t[localFileHeader.lenOfFileName + 1]{ 0 };
                        if (fread(localFileHeader.fileName, localFileHeader.lenOfFileName, 1, files[_token].file) != 1) {
                            Cleanup(&localFileHeader);
                            return false;
                        }
                    }

                    if (localFileHeader.lenOfExtraField) {
                        localFileHeader.extraField = new uint8_t[localFileHeader.lenOfExtraField + 1]{ 0 };
                        if (fread(localFileHeader.extraField, localFileHeader.lenOfExtraField, 1, files[_token].file) != 1) {
                            Cleanup(&localFileHeader);
                            return false;
                        }
                    }
                }
                catch (...) {
                    Cleanup(&localFileHeader);
                    return false;
                }
                if (localFileHeader.compressionMethod == 8) {
                    // Разбираем дефляты... Опять. 

                    if ((localFileHeader.generalPurposeBitFlag >> 3) & 1) {
                        // Если установлен 3 бит в generalPurposeBitFlag - значит мы имеем дело с DataDesription!
                        // Не прочитав DataDescription мы не узнаем сжатый и распакованный размер файла :)

                        uint32_t findSignature;
                        long startPos = ftell(files[_token].file);

                        // Начинаем искать DataDescription...
                        while (1) {
                            if (fread(&findSignature, 4, 1, files[_token].file) != 1) {
                                Cleanup(&localFileHeader);
                                return false;
                            }

                            if (findSignature == 0x04034B50) {
                                Cleanup(&localFileHeader);
                                return false;
                            }

                            if (findSignature == 0x08074B50) {
                                fseek(files[_token].file, -4, SEEK_CUR);
                                break;
                            }

                            fseek(files[_token].file, -3, SEEK_CUR);
                        }

                        DataDescriptor dd;
                        memset(&dd, 0, sizeof(DataDescriptor));

                        if (fread(&dd, sizeof(dd), 1, files[_token].file) != 1) {
                            Cleanup(&localFileHeader);
                            return false;
                        };

                        localFileHeader.compressSize = dd.compressSize;
                        localFileHeader.unCompressSize = dd.unCompressSize;
                        localFileHeader.CRC32 = dd.CRC32;

                        fseek(files[_token].file, startPos, SEEK_SET);
                    }

                    auto data = new BYTE[localFileHeader.compressSize];
                    if (fread(data, localFileHeader.compressSize, 1, files[_token].file) != 1) {
                        Cleanup(&localFileHeader);
                        return false;
                    };

                    BitArray bits = data;
                    bool _newBlock = true;
                    uint16_t byte = 0, copySize = 0;
                    uint8_t bitsCount = 0, _blockType = 0;

                    uint32_t index = 0;
                    uint8_t* resultBuf = new uint8_t[localFileHeader.unCompressSize + 1]{ 0 };
                    if (!resultBuf) {
                        Cleanup(&localFileHeader);
                        return false;
                    }
                    memset(resultBuf, 0, localFileHeader.unCompressSize);
                    for (uint32_t i = 0; i < localFileHeader.compressSize * 8; i++) {
                        if (_newBlock) {
                            i++;
                            _blockType = (bits[i] | (bits[i + 1] << 1));
                            i++;
                            _newBlock = false;
                            continue;
                        }

                        switch (_blockType) {
                            // Разбираем статический дефлят
                        case 0b01: {
                            decodeStatic(bits, i, byte, copySize, bitsCount, _newBlock, index, resultBuf);
                            break;
                        }
                                 // Динамический дефлят
                        case 0b10: {
                            decodeDynamic(bits, i, byte, copySize, bitsCount, _newBlock, index, resultBuf, localFileHeader);
                            break;
                        }
                        case 0b11: {
                            printf("Incorrect type!\n");
                            Cleanup(&localFileHeader);
                            return false;
                        }
                        }
                    }
                    FILE* out = _wfopen(path, L"wb");
                    if (!out) {
                        Cleanup(&localFileHeader);
                        return false;
                    }

                    if (fwrite(resultBuf, localFileHeader.unCompressSize, 1, out) != 1) {
                        fclose(out);
                        Cleanup(&localFileHeader);
                        return false;
                    }

                    fclose(out);
                }
                else if (localFileHeader.compressionMethod == 0) {
                    // Случай без сжатия
                    auto fileName = std::make_unique<wchar_t[]>(localFileHeader.lenOfFileName + 1);
                    for (uint16_t i = 0; i < localFileHeader.lenOfFileName; i++)
                        fileName[i] = localFileHeader.fileName[i];

                    std::wstring resultPath = std::wstring(path) + fileName.get();
                    FILE* out = _wfopen(resultPath.c_str(), L"wb");
                    if (!out) {
                        Cleanup(&localFileHeader);
                        return false;
                    }

                    auto buf = std::make_unique<char[]>(localFileHeader.compressSize);
                    if (fread(buf.get(), localFileHeader.compressSize, 1, files[_token].file) != 1) {
                        fclose(out);
                        Cleanup(&localFileHeader);
                        return false;
                    }
                    if (fwrite(buf.get(), localFileHeader.compressSize, 1, out) != 1) {
                        fclose(out);
                        Cleanup(&localFileHeader);
                        return false;
                    }

                    fclose(out);
                }
                else return false;

                return true;
            }
        }

        return false;
    }

    __declspec(dllexport) void CloseFile(FileToken _token) {
        if (!CheckToken(_token)) return;

        Cleanup(&files[_token]);
        freeTokens.push_back(_token);
        return;
    }

}
//int main() {
//    FileToken token = OpenArchive(L"D:\\Проекты\\No-ZIP\\a.zip");
//
//    size_t count = GetFileCount(token);
//
//    FileSpecification* localFiles = GetListOfFiles(token);
//    for (FileToken i = 0; i < count; i++) {
//        ExportFile(token, localFiles[i].fileName, L"testFile.txt");
//    }
//    CloseFile(token);
//    return 0;
//}