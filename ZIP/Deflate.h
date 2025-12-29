#pragma once
#include <stdint.h>
#include "BitStream.h"
#include "structOfZIP.h"

bool decodeStatic(BitArray& bits, uint32_t& i, uint16_t& byte, uint16_t& copySize, uint8_t& bitsCount, bool& _newBlock,
    uint32_t& index, uint8_t* resultBuf);
bool decodeDynamic(BitArray& bits, uint32_t& i, uint16_t& byte, uint16_t& copySize, uint8_t& bitsCount, bool& _newBlock,
    uint32_t& index, uint8_t* resultBuf, LocalFileHeader& localFileHeader);
