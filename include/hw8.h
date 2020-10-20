#pragma once

#include <stdlib.h>

struct HW8TransactionRecord {
    char const *name;
    char const *filePath;
};

void hw8(struct HW8TransactionRecord const *transactionRecords, size_t transactionRecordCount);
