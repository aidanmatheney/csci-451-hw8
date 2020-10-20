#include "../include/hw8.h"

#include "../include/util/memory.h"
#include "../include/util/thread.h"
#include "../include/util/file.h"
#include "../include/util/random.h"
#include "../include/util/guard.h"
#include "../include/util/error.h"
#include "../include/util/regex.h"

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <regex.h>
#include <assert.h>

static bool initialized = false;
static regex_t beginTransactionSectionRegex;
static regex_t transactionRegex;
static regex_t endTransactionSectionRegex;

static void ensureInitialized(void);

struct ProcessTransactionsThreadStartArg {
    struct HW8TransactionRecord const *transactionRecordPtr;
    float *balancePtr;

    pthread_mutex_t *balanceMutexPtr;
};

static void *processTransactionsThreadStart(void *argAsVoidPtr);

/**
 * Run CSCI 451 HW8. This uses the given transaction records to model multithreaded deposit and withdrawal transactions
 * on an account balance. A separate thread is launched to process each transaction record. The threads will pause in
 * between each transaction section to simulate a random order of occurrence.
 *
 * @param transactionRecords The transaction records to process.
 * @param transactionRecordCount The number of transaction records.
 */
void hw8(struct HW8TransactionRecord const * const transactionRecords, size_t const transactionRecordCount) {
    ensureInitialized();

    guardNotNull(transactionRecords, "transactionRecords", "hw8");

    float balance = 0;

    pthread_mutex_t balanceMutex;
    safeMutexInit(&balanceMutex, NULL, "hw8");

    struct ProcessTransactionsThreadStartArg * const threadStartArgs = (
        safeMalloc(sizeof *threadStartArgs * transactionRecordCount, "hw8")
    );
    pthread_t * const threadIds = safeMalloc(sizeof *threadIds * transactionRecordCount, "hw8");
    for (size_t i = 0; i < transactionRecordCount; i += 1) {
        struct HW8TransactionRecord const * const transactionRecordPtr = &transactionRecords[i];
        struct ProcessTransactionsThreadStartArg * const threadStartArgPtr = &threadStartArgs[i];

        threadStartArgPtr->transactionRecordPtr = transactionRecordPtr;
        threadStartArgPtr->balancePtr = &balance;
        threadStartArgPtr->balanceMutexPtr = &balanceMutex;

        threadIds[i] = safePthreadCreate(
            NULL,
            processTransactionsThreadStart,
            threadStartArgPtr,
            "hw8"
        );
    }

    for (size_t i = 0; i < transactionRecordCount; i += 1) {
        pthread_t const threadId = threadIds[i];
        safePthreadJoin(threadId, "hw8");
    }

    free(threadStartArgs);
    free(threadIds);

    safeMutexDestroy(&balanceMutex, "hw8");

    printf("Final account balance is $%.2f\n", (double)balance);
}

static void ensureInitialized(void) {
    if (initialized) {
        return;
    }

    safeRegcomp(&beginTransactionSectionRegex, "^R$", REG_EXTENDED | REG_NOSUB, "hw8 ensureInitialized");
    // ^[+-][0-9]+\.?[0-9]*|[0-9]*\.[0-9]+$
    safeRegcomp(&transactionRegex, "^[+-][0-9]+\\.?[0-9]*|[0-9]*\\.[0-9]+$", REG_EXTENDED | REG_NOSUB, "hw8 ensureInitialized");
    safeRegcomp(&endTransactionSectionRegex, "^W$", REG_EXTENDED | REG_NOSUB, "hw8 ensureInitialized");

    initialized = true;
}

static void *processTransactionsThreadStart(void * const argAsVoidPtr) {
    assert(argAsVoidPtr != NULL);
    struct ProcessTransactionsThreadStartArg * const argPtr = argAsVoidPtr;

    FILE * const transactionFile = (
        safeFopen(argPtr->transactionRecordPtr->filePath, "r", "hw8 processTransactionsThreadStart")
    );

    bool isFirstTransactionSection = true;
    while (true) {
        char * const beginTransactionSectionLine = readFileLine(transactionFile);
        if (beginTransactionSectionLine == NULL) {
            break;
        }
        if (regexec(&beginTransactionSectionRegex, beginTransactionSectionLine, 0, NULL, 0) == REG_NOMATCH) {
            abortWithErrorFmt(
                "hw8 processTransactionsThreadStart: %s thread failed to parse BeginTransactionSection symbol from \"%s\" (line: \"%s\")",
                argPtr->transactionRecordPtr->name,
                argPtr->transactionRecordPtr->filePath,
                beginTransactionSectionLine
            );
            break;
        }
        free(beginTransactionSectionLine);

        if (!isFirstTransactionSection) {
            // Simulate delay between transaction sections
            nanosleep(&(struct timespec){
                .tv_sec = randomInt(0, 2),
                .tv_nsec = randomInt(0, 1000 * 1000 * 1000)
            }, NULL);
        }

        safeMutexLock(argPtr->balanceMutexPtr, "hw8 processTransactionsThreadStart");

        float balance = *argPtr->balancePtr;

        while (true) {
            char * const line = readFileLine(transactionFile);
            if (line == NULL) {
                abortWithErrorFmt(
                    "hw8 processTransactionsThreadStart: %s thread reached EOF before EndTransactionSection symbol was parsed from \"%s\"",
                    argPtr->transactionRecordPtr->name,
                    argPtr->transactionRecordPtr->filePath
                );
                break;
            }
            if (regexec(&endTransactionSectionRegex, line, 0, NULL, 0) == 0) {
                free(line);
                break;
            }
            if (regexec(&transactionRegex, line, 0, NULL, 0) == REG_NOMATCH) {
                abortWithErrorFmt(
                    "hw8 processTransactionsThreadStart: %s thread failed to parse Deposit, Withdraw, or EndTransactionSection symbol from \"%s\" (line: \"%s\")",
                    argPtr->transactionRecordPtr->name,
                    argPtr->transactionRecordPtr->filePath,
                    line
                );
                break;
            }

            float const transactionAmount = strtof(line, NULL);
            free(line);

            balance += transactionAmount;
        }

        *argPtr->balancePtr = balance;
        printf("Account balance after thread %s is $%.2f\n", argPtr->transactionRecordPtr->name, (double)balance);

        safeMutexUnlock(argPtr->balanceMutexPtr, "hw8 processTransactionsThreadStart");

        isFirstTransactionSection = false;
    }

    fclose(transactionFile);

    return NULL;
}
