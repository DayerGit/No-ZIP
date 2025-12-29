#include "CodeLenghts.h"

int codeLengthsForCodeLengthsCompare(const void* arg1, const void* arg2) {
    const CodeLengthsForCodeLengths* one = reinterpret_cast<const CodeLengthsForCodeLengths*>(arg1);
    const CodeLengthsForCodeLengths* two = reinterpret_cast<const CodeLengthsForCodeLengths*>(arg2);

    if (one->numOfBits < two->numOfBits) return -1;
    if (one->numOfBits > two->numOfBits) return 1;

    if (one->numInOrder < two->numInOrder) return -1;
    if (one->numInOrder > two->numInOrder) return 1;

    return 0;
}

int tableOfCodeLenghtsCompare(const void* arg1, const void* arg2) {
    const TableOfCodeLenghts* one = reinterpret_cast<const TableOfCodeLenghts*>(arg1);
    const TableOfCodeLenghts* two = reinterpret_cast<const TableOfCodeLenghts*>(arg2);

    if (one->numOfBits < two->numOfBits) return -1;
    if (one->numOfBits > two->numOfBits) return 1;

    if (one->symbol < two->symbol) return -1;
    if (one->symbol > two->symbol) return 1;

    return 0;
}