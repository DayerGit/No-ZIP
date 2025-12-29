#pragma once
#include <stdint.h>

int codeLengthsForCodeLengthsCompare(const void* arg1, const void* arg2);
int tableOfCodeLenghtsCompare(const void* arg1, const void* arg2);

struct CodeLengthsForCodeLengths {
    int16_t numInOrder = 0, numOfBits = 0, numOfCode = 0;

    bool operator==(const CodeLengthsForCodeLengths& other) const {
        return numOfBits == other.numOfBits &&
            numOfCode == other.numOfCode;
    }

    bool operator!=(const CodeLengthsForCodeLengths& other) const {
        return !(*this == other);
    }
};

struct TableOfCodeLenghts {
    int32_t symbol = 0, numOfBits = 0, numOfCode = 0;

    bool operator==(const TableOfCodeLenghts& other) const {
        return numOfBits == other.numOfBits &&
            numOfCode == other.numOfCode;
    }

    bool operator!=(const TableOfCodeLenghts& other) const {
        return !(*this == other);
    }
};