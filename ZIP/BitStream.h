#pragma once
#include <stdint.h>

#pragma pack(push, 1)

struct BYTE {
    union {
        struct {
            bool flag0 : 1;
            bool flag1 : 1;
            bool flag2 : 1;
            bool flag3 : 1;
            bool flag4 : 1;
            bool flag5 : 1;
            bool flag6 : 1;
            bool flag7 : 1;
        };
        uint8_t byte;
    };

    void operator=(int value) {
        byte = value;
    }

    bool operator[](int index) {
        return (byte >> index) & 1;
    }
};

struct BitArray {
    BYTE* array;

    BitArray(BYTE* arr) {
        operator=(arr);
    }

    void operator=(BYTE* arr) {
        array = arr;
    }

    bool operator[](int index) {
        return array[index / 8][index % 8];
    }
};

#pragma pack(push)

uint8_t readBits(BitArray& bits, uint32_t* i, int count);