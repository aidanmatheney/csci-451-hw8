#include "../include/hw8.h"

#include "../include/util/list.h"
#include "../include/util/memory.h"
#include "../include/util/thread.h"
#include "../include/util/file.h"
#include "../include/util/random.h"
#include "../include/util/guard.h"
#include "../include/util/error.h"
#include "../include/util/regex.h"
#include "../include/util/macro.h"

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

struct Page {
    char const *owner;
    bool referenced;
    bool modified;
};
DEFINE_CIRCULAR_LINKED_LIST(Pages, struct Page)
DEFINE_LIST(PageNodeList, PagesNode)

struct ProcessTransactionsThreadStartArg {
    struct HW8TransactionRecord const *transactionRecordPtr;

    float *balancePtr;
    pthread_mutex_t *balanceMutexPtr;

    Pages pages;
    PagesNode initialOwnedPageNode;
    pthread_mutex_t *pagesMutexPtr;
};
static void *processTransactionsThreadStart(void *argAsVoidPtr);

struct PeriodicallyResetPagesReferencedThreadStartArg {
    Pages pages;
    pthread_mutex_t *pagesMutexPtr;

    bool *stopPtr;
};
static void *periodicallyResetPagesReferencedThreadStart(void *argAsVoidPtr);

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

    Pages const pages = Pages_create();
    Pages_add(pages, (struct Page){
        .owner = NULL,
        .referenced = false,
        .modified = false
    });
    pthread_mutex_t pagesMutex;
    safeMutexInit(&pagesMutex, NULL, "hw8");
    safeMutexLock(&pagesMutex, "hw8");

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

        threadStartArgPtr->pages = pages;
        PagesNode const initialOwnedPageNode = Pages_add(pages, (struct Page){
            .owner = transactionRecordPtr->name,
            .referenced = false,
            .modified = false
        });
        threadStartArgPtr->initialOwnedPageNode = initialOwnedPageNode;
        threadStartArgPtr->pagesMutexPtr = &pagesMutex;

        threadIds[i] = safePthreadCreate(
            NULL,
            processTransactionsThreadStart,
            threadStartArgPtr,
            "hw8"
        );
    }

    safeMutexUnlock(&pagesMutex, "hw8");

    bool stopPeriodicallyResettingPagesReferenced = false;
    safePthreadCreate(
        NULL,
        periodicallyResetPagesReferencedThreadStart,
        &(struct PeriodicallyResetPagesReferencedThreadStartArg){
            .pages = pages,
            .pagesMutexPtr = &pagesMutex,
            .stopPtr = &stopPeriodicallyResettingPagesReferenced
        },
        "hw8"
    );

    for (size_t i = 0; i < transactionRecordCount; i += 1) {
        pthread_t const threadId = threadIds[i];
        safePthreadJoin(threadId, "hw8");
    }

    stopPeriodicallyResettingPagesReferenced = true;
    safeMutexLock(&pagesMutex, "hw8");
    Pages_destroy(pages);
    safeMutexUnlock(&pagesMutex, "hw8");

    free(threadStartArgs);
    free(threadIds);

    safeMutexDestroy(&balanceMutex, "hw8");
    safeMutexDestroy(&pagesMutex, "hw8");

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

    PageNodeList const ownedPageNodes = PageNodeList_create();
    PageNodeList_add(ownedPageNodes, argPtr->initialOwnedPageNode);

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

        safeMutexLock(argPtr->pagesMutexPtr, "hw8 processTransactionsThreadStart");

        for (int i = 0; (size_t)i < PageNodeList_count(ownedPageNodes); i += 1) {
            PagesNode const ownedPageNode = PageNodeList_get(ownedPageNodes, (size_t)i);
            if (Pages_item(argPtr->pages, ownedPageNode).owner != argPtr->transactionRecordPtr->name) {
                PageNodeList_removeAt(ownedPageNodes, (size_t)i);
                i -= 1;
            }
        }

        bool const requireAdditionalPage = randomInt(0, 4) == 0;
        if (PageNodeList_empty(ownedPageNodes) || requireAdditionalPage) {
            printf("Page fault in thread %s\n", argPtr->transactionRecordPtr->name);

            PagesNode unownedAdditionalPageNode = NULL;
            PagesNode class0AdditionalPageNode = NULL;
            PagesNode class1AdditionalPageNode = NULL;
            PagesNode class2AdditionalPageNode = NULL;
            PagesNode class3AdditionalPageNode = NULL;

            PagesNode const headPageNode = Pages_head(argPtr->pages);
            PagesNode currentPageNode = headPageNode;
            while (true) {
                struct Page * const currentPagePtr = Pages_itemPtr(argPtr->pages, currentPageNode);

                if (currentPagePtr->owner == NULL) {
                    unownedAdditionalPageNode = currentPageNode;
                    break;
                }

                bool const currentPageReferenced = currentPagePtr->referenced;
                bool const currentPageModified = currentPagePtr->modified;
                if (!currentPageReferenced && !currentPageModified) {
                    class0AdditionalPageNode = currentPageNode;
                } else if (!currentPageReferenced && currentPageModified) {
                    class1AdditionalPageNode = currentPageNode;
                } else if (currentPageReferenced && !currentPageModified) {
                    class2AdditionalPageNode = currentPageNode;
                } else { // currentPageReferenced && currentPageModified
                    class3AdditionalPageNode = currentPageNode;
                }

                PagesNode const nextPageNode = Pages_next(argPtr->pages, currentPageNode);
                if (nextPageNode == headPageNode) {
                    break;
                }
                currentPageNode = nextPageNode;
            }

            PagesNode const additionalPageNode = (
                unownedAdditionalPageNode != NULL ? (
                    unownedAdditionalPageNode
                ) : class0AdditionalPageNode != NULL ? (
                    class0AdditionalPageNode
                ) : class1AdditionalPageNode != NULL ? (
                    class1AdditionalPageNode
                ) : class2AdditionalPageNode != NULL ? (
                    class2AdditionalPageNode
                ) : ( // class3AdditionalPageNode != NULL
                    class3AdditionalPageNode
                )
            );
            struct Page * const additionalPagePtr = Pages_itemPtr(argPtr->pages, additionalPageNode);
            printf(
                "Page being removed: {owner=%s, referenced=%s, modified=%s}\n",
                additionalPagePtr->owner == NULL ? "[UNOWNED]" : additionalPagePtr->owner,
                additionalPagePtr->referenced ? "yes" : "no",
                additionalPagePtr->modified ? "yes" : "no"
            );

            PageNodeList_add(ownedPageNodes, additionalPageNode);
            additionalPagePtr->owner = argPtr->transactionRecordPtr->name;
            additionalPagePtr->referenced = false;
            additionalPagePtr->modified = false;
        }

        for (size_t i = 0; i < PageNodeList_count(ownedPageNodes); i += 1) {
            PagesNode const ownedPageNode = PageNodeList_get(ownedPageNodes, i);
            if (balance < 0) {
                Pages_itemPtr(argPtr->pages, ownedPageNode)->referenced = true;
                Pages_itemPtr(argPtr->pages, ownedPageNode)->modified = true;
            } else if (balance > 0) {
                Pages_itemPtr(argPtr->pages, ownedPageNode)->referenced = true;
            }
        }

        safeMutexUnlock(argPtr->pagesMutexPtr, "hw8 processTransactionsThreadStart");

        *argPtr->balancePtr = balance;
        printf("Account balance after thread %s is $%.2f\n", argPtr->transactionRecordPtr->name, (double)balance);

        safeMutexUnlock(argPtr->balanceMutexPtr, "hw8 processTransactionsThreadStart");

        isFirstTransactionSection = false;
    }

    PageNodeList_destroy(ownedPageNodes);
    fclose(transactionFile);

    return NULL;
}

static void *periodicallyResetPagesReferencedThreadStart(void * const argAsVoidPtr) {
    assert(argAsVoidPtr != NULL);
    struct PeriodicallyResetPagesReferencedThreadStartArg * const argPtr = argAsVoidPtr;

    while (true) {
        nanosleep(&(struct timespec){
            .tv_sec = 1,
            .tv_nsec = 0
        }, NULL);

        if (*argPtr->stopPtr) {
            break;
        }

        safeMutexLock(argPtr->pagesMutexPtr, "hw8 periodicallyResetPagesReferencedThreadStart");

        if (*argPtr->stopPtr) {
            safeMutexUnlock(argPtr->pagesMutexPtr, "hw8 periodicallyResetPagesReferencedThreadStart");
            break;
        }

        PagesNode const headPageNode = Pages_head(argPtr->pages);
        PagesNode currentPageNode = headPageNode;
        while (true) {
            Pages_itemPtr(argPtr->pages, currentPageNode)->referenced = false;

            PagesNode const nextPageNode = Pages_next(argPtr->pages, currentPageNode);
            if (nextPageNode == headPageNode) {
                break;
            }
            currentPageNode = nextPageNode;
        }

        safeMutexUnlock(argPtr->pagesMutexPtr, "hw8 periodicallyResetPagesReferencedThreadStart");
    }

    return NULL;
}
