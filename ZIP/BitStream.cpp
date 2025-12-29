#include "BitStream.h"

uint8_t readBits(BitArray& bits, uint32_t* i, int count) {
    uint8_t readByte = 0;
    for (int j = *i; j < *i + count; j++) {
        readByte = readByte | (bits[j] << j - *i);
    }
    *i += count;
    return readByte;
}