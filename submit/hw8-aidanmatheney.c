/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW8
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void abortWithError(char const *errorMessage);
void abortWithErrorFmt(char const *errorMessageFormat, ...);
void abortWithErrorFmtVA(char const *errorMessageFormat, va_list errorMessageFormatArgs);

/**
 * Turn the given macro token into a string literal.
 *
 * @param macroToken The macro token.
 *
 * @returns The string literal.
 */
#define STRINGIFY(macroToken) #macroToken

/**
 * Get the length of the given compile-time array.
 *
 * @param array The array.
 *
 * @returns The length number literal.
 */
#define ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])

/**
 * Get a stack-allocated mutable string from the given string literal.
 *
 * @param stringLiteral The string literal.
 *
 * @returns The `char [length + 1]`-typed stack-allocated string.
*/
#define MUTABLE_STRING(stringLiteral) ((char [ARRAY_LENGTH(stringLiteral)]){ stringLiteral })

void *safeMalloc(size_t size, char const *callerDescription);
void *safeRealloc(void *memory, size_t newSize, char const *callerDescription);

/**
 * Declare (.h file) a generic Result class which holds no success value but can hold a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DECLARE_VOID_RESULT(TResult, TError) \
    struct TResult; \
    typedef struct TResult * TResult; \
    typedef struct TResult const * Const##TResult; \
    \
    TResult TResult##_success(void); \
    TResult TResult##_failure(TError error); \
    void TResult##_destroy(TResult result); \
    \
    bool TResult##_isSuccess(Const##TResult result); \
    TError TResult##_getError(Const##TResult result); \
    TError TResult##_getErrorAndDestroy(TResult result);

/**
 * Define (.c file) a generic Result class which holds no success value but can hold a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DEFINE_VOID_RESULT(TResult, TError) \
    DECLARE_VOID_RESULT(TResult, TError) \
    \
    struct TResult { \
        bool success; \
        TError error; \
    }; \
    \
    TResult TResult##_success(void) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = true; \
        return result; \
    } \
    \
    TResult TResult##_failure(TError const error) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = false; \
        result->error = error; \
        return result; \
    } \
    \
    void TResult##_destroy(TResult const result) { \
        free(result); \
    } \
    \
    bool TResult##_isSuccess(Const##TResult const result) { \
        return result->success; \
    } \
    \
    TError TResult##_getError(Const##TResult const result) { \
        if (result->success) { \
            abortWithErrorFmt("%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getError)); \
        } \
        \
        return result->error; \
    } \
    \
    TError TResult##_getErrorAndDestroy(TResult const result) { \
        TError const error = TResult##_getError(result); \
        TResult##_destroy(result); \
        return error; \
    }

/**
 * Declare (.h file) a generic Result class which can hold either a sucess value or a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DECLARE_RESULT(TResult, TValue, TError) \
    struct TResult; \
    typedef struct TResult * TResult; \
    typedef struct TResult const * Const##TResult; \
    \
    TResult TResult##_success(TValue value); \
    TResult TResult##_failure(TError error); \
    void TResult##_destroy(TResult result); \
    \
    bool TResult##_isSuccess(Const##TResult result); \
    TValue TResult##_getValue(Const##TResult result); \
    TValue TResult##_getValueAndDestroy(TResult result); \
    TError TResult##_getError(Const##TResult result); \
    TError TResult##_getErrorAndDestroy(TResult result);

/**
 * Define (.c file) a generic Result class which can hold either a sucess value or a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DEFINE_RESULT(TResult, TValue, TError) \
    DECLARE_RESULT(TResult, TValue, TError) \
    \
    struct TResult { \
        bool success; \
        TValue value; \
        TError error; \
    }; \
    \
    TResult TResult##_success(TValue const value) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = true; \
        result->value = value; \
        return result; \
    } \
    \
    TResult TResult##_failure(TError const error) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = false; \
        result->error = error; \
        return result; \
    } \
    \
    void TResult##_destroy(TResult const result) { \
        free(result); \
    } \
    \
    bool TResult##_isSuccess(Const##TResult const result) { \
        return result->success; \
    } \
    \
    TValue TResult##_getValue(Const##TResult const result) { \
        if (!result->success) { \
            abortWithErrorFmt("%s: Cannot get result value. Result is failure", STRINGIFY(TResult##_getValue)); \
        } \
        \
        return result->value; \
    } \
    \
    TValue TResult##_getValueAndDestroy(TResult const result) { \
        TValue const value = TResult##_getValue(result); \
        TResult##_destroy(result); \
        return value; \
    } \
    \
    TError TResult##_getError(Const##TResult const result) { \
        if (result->success) { \
            abortWithErrorFmt("%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getError)); \
        } \
        \
        return result->error; \
    } \
    \
    TError TResult##_getErrorAndDestroy(TResult const result) { \
        TError const error = TResult##_getError(result); \
        TResult##_destroy(result); \
        return error; \
    }

void guard(bool expression, char const *errorMessage);
void guardFmt(bool expression, char const *errorMessageFormat, ...);
void guardFmtVA(bool expression, char const *errorMessageFormat, va_list errorMessageFormatArgs);

void guardNotNull(void const *object, char const *paramName, char const *callerName);

#define DECLARE_ENUMERATOR(TEnumerator, TItem) \
    struct TEnumerator; \
    typedef struct TEnumerator * TEnumerator; \
    typedef struct TEnumerator const * Const##TEnumerator; \
    \
    void TEnumerator##_destroy(TEnumerator enumerator); \
    \
    bool TEnumerator##_moveNext(TEnumerator enumerator); \
    TItem TEnumerator##_current(Const##TEnumerator enumerator); \
    void TEnumerator##_reset(TEnumerator enumerator);

/**
 * Declare (.h file) a generic ListEnumerator class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_LIST_ENUMERATOR(TList, TItem) \
    DECLARE_ENUMERATOR(TList##Enumerator, TItem) \
    \
    TList##Enumerator TList##Enumerator##_create(Const##TList list, int direction);

/**
 * Define (.c file) a generic ListEnumerator class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_LIST_ENUMERATOR(TList, TItem) \
    DECLARE_LIST_ENUMERATOR(TList, TItem) \
    \
    struct TList##Enumerator { \
        Const##TList list; \
        int direction; \
        int currentIndex; \
    }; \
    \
    static void TList##Enumerator##_guardCurrentIndexInRange(Const##TList##Enumerator enumerator, char const *callerName); \
    \
    TList##Enumerator TList##Enumerator##_create(Const##TList const list, int const direction) { \
        guardNotNull(list, "list", STRINGIFY(TList##Enumerator##_create)); \
        \
        TList##Enumerator const enumerator = safeMalloc(sizeof *enumerator, STRINGIFY(TList##Enumerator##_create)); \
        enumerator->list = list; \
        enumerator->direction = direction; \
        enumerator->currentIndex = direction == 1 ? -1 : (int)TList##_count(list); \
        return enumerator; \
    } \
    \
    void TList##Enumerator##_destroy(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_destroy)); \
        free(enumerator); \
    } \
    \
    bool TList##Enumerator##_moveNext(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_moveNext)); \
        \
        enumerator->currentIndex += enumerator->direction; \
        return enumerator->currentIndex >= 0 && enumerator->currentIndex < (int)TList##_count(enumerator->list); \
    } \
    \
    TItem TList##Enumerator##_current(Const##TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_current)); \
        TList##Enumerator##_guardCurrentIndexInRange(enumerator, STRINGIFY(TList##Enumerator##_current)); \
        return TList##_get(enumerator->list, (size_t)enumerator->currentIndex); \
    } \
    \
    void TList##Enumerator##_reset(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_reset)); \
        enumerator->currentIndex = enumerator->direction == 1 ? -1 : (int)TList##_count(enumerator->list); \
    } \
    \
    static void TList##Enumerator##_guardCurrentIndexInRange(Const##TList##Enumerator const enumerator, char const * const callerName) { \
        if (enumerator->currentIndex < 0 || enumerator->currentIndex >= (int)TList##_count(enumerator->list)) { \
            abortWithErrorFmt( \
                "%s: Current index (%d) is out of range (list count: %zu)", \
                callerName, \
                enumerator->currentIndex, \
                TList##_count(enumerator->list) \
            ); \
        } \
    }

/**
 * Declare (.h file) a typedef to a function pointer for a function with the specified args that returns void.
 *
 * @param TAction The name of the new type.
 * @param ... The function's parameter types.
 */
#define DECLARE_ACTION(TAction, ...) typedef void (*TAction)(__VA_ARGS__);

/**
 * Declare (.h file) a typedef to a function pointer for a function with the specified args and return type.
 *
 * @param TFunc The name of the new type.
 * @param TResult The function's return type.
 * @param ... The function's parameter types.
 */
#define DECLARE_FUNC(TFunc, TResult, ...) typedef TResult (*TFunc)(__VA_ARGS__);

/**
 * Declare (.h file) a generic List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_LIST(TList, TItem) \
    struct TList; \
    typedef struct TList * TList; \
    typedef struct TList const * Const##TList; \
    \
    DECLARE_ACTION(TList##ForEachCallback, void *, size_t, TItem) \
    DECLARE_FUNC(TList##FindCallback, bool, void *, size_t, TItem) \
    DECLARE_RESULT(TList##FindItemResult, TItem, void *) \
    \
    DECLARE_LIST_ENUMERATOR(TList, TItem) \
    \
    TList TList##_create(void); \
    TList TList##_fromItems(TItem const *items, size_t count); \
    TList TList##_fromList(Const##TList list); \
    void TList##_destroy(TList list); \
    \
    TItem const *TList##_items(Const##TList list); \
    size_t TList##_count(Const##TList list); \
    bool TList##_empty(Const##TList list); \
    \
    TItem TList##_get(Const##TList list, size_t index); \
    TItem *TList##_getPtr(TList list, size_t index); \
    TItem const *TList##_constGetPtr(Const##TList list, size_t index); \
    \
    void TList##_add(TList list, TItem item); \
    void TList##_addMany(TList list, TItem const *items, size_t count); \
    void TList##_insert(TList list, size_t index, TItem item); \
    void TList##_insertMany(TList list, size_t index, TItem const *items, size_t count); \
    void TList##_set(TList list, size_t index, TItem item); \
    \
    void TList##_removeAt(TList list, size_t index); \
    void TList##_removeManyAt(TList list, size_t startIndex, size_t count); \
    void TList##_clear(TList list); \
    \
    void TList##_forEach(Const##TList list, void *state, TList##ForEachCallback callback); \
    void TList##_forEachReverse(Const##TList list, void *state, TList##ForEachCallback callback); \
    bool TList##_has(Const##TList list, TItem item); \
    size_t TList##_indexOf(Const##TList list, TItem item); \
    size_t TList##_lastIndexOf(Const##TList list, TItem item); \
    bool TList##_findHas(Const##TList list, void *state, TList##FindCallback callback); \
    TList##FindItemResult TList##_find(Const##TList list, void *state, TList##FindCallback callback); \
    size_t TList##_findIndex(Const##TList list, void *state, TList##FindCallback callback); \
    TList##FindItemResult TList##_findLast(Const##TList list, void *state, TList##FindCallback callback); \
    size_t TList##_findLastIndex(Const##TList list, void *state, TList##FindCallback callback); \
    \
    TList##Enumerator TList##_enumerate(Const##TList list); \
    TList##Enumerator TList##_enumerateReverse(Const##TList list); \
    \
    void TList##_fillArray(Const##TList list, TItem *array, size_t startIndex, size_t count);

/**
 * Define (.c file) a generic List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_LIST(TList, TItem) \
    DECLARE_LIST(TList, TItem) \
    \
    static void TList##_ensureCapacity(TList list, size_t targetCapacity); \
    static void TList##_guardIndexInRange(Const##TList list, size_t index, char const *callerName); \
    static void TList##_guardIndexInInsertRange(Const##TList list, size_t index, char const *callerName); \
    static void TList##_guardStartIndexAndCountInRange(Const##TList list, size_t startIndex, size_t count, char const *callerName); \
    \
    DEFINE_RESULT(TList##FindItemResult, TItem, void *) \
    \
    DEFINE_LIST_ENUMERATOR(TList, TItem) \
    \
    struct TList { \
        TItem *items; \
        size_t count; \
        size_t capacity; \
    }; \
    \
    TList TList##_create(void) { \
        TList const list = safeMalloc(sizeof *list, STRINGIFY(TList##_create)); \
        list->items = NULL; \
        list->count = 0; \
        list->capacity = 0; \
        return list; \
    } \
    \
    TList TList##_fromItems(TItem const * const items, size_t const count) { \
        guardNotNull(items, "items", STRINGIFY(TList##_fromItems)); \
        \
        TList const list = TList##_create(); \
        TList##_addMany(list, items, count); \
        return list; \
    } \
    \
    TList TList##_fromList(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_fromList)); \
        return TList##_fromItems(list->items, list->count); \
    } \
    \
    void TList##_destroy(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_destroy)); \
        \
        free(list->items); \
        free(list); \
    } \
    \
    TItem const *TList##_items(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_items)); \
        return list->items; \
    } \
    \
    size_t TList##_count(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_count)); \
        return list->count; \
    } \
    \
    bool TList##_empty(Const##TList list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_empty)); \
        return list->count == 0; \
    } \
    \
    TItem TList##_get(Const##TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_get)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_get)); \
        return list->items[index]; \
    } \
    \
    TItem *TList##_getPtr(TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_getPtr)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_getPtr)); \
        return &list->items[index]; \
    } \
    \
    TItem const *TList##_constGetPtr(Const##TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constGetPtr)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_constGetPtr)); \
        return &list->items[index]; \
    } \
    \
    void TList##_add(TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_add)); \
        \
        TList##_ensureCapacity(list, list->count + 1); \
        list->items[list->count] = item; \
        list->count += 1; \
    } \
    \
    void TList##_addMany(TList const list, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_addMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_addMany)); \
        \
        TList##_ensureCapacity(list, list->count + count); \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            list->items[list->count + i] = item; \
        } \
        list->count += count; \
    } \
    \
    void TList##_insert(TList const list, size_t const index, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insert)); \
        TList##_guardIndexInInsertRange(list, index, STRINGIFY(TList##_insert)); \
        \
        TList##_ensureCapacity(list, list->count + 1); \
        for (int i = (int)list->count - 1; i >= (int)index; i -= 1) { \
            /* Shift each item at an index >= the target index one to the right */ \
            list->items[i + 1] = list->items[i]; \
        } \
        list->items[index] = item; \
        list->count += 1; \
    } \
    \
    void TList##_insertMany(TList const list, size_t const index, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insertMany)); \
        TList##_guardIndexInInsertRange(list, index, STRINGIFY(TList##_insertMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_insertMany)); \
        \
        TList##_ensureCapacity(list, list->count + count); \
        for (int i = (int)list->count - 1; i >= (int)index; i -= 1) { \
            /* Shift each item at an index >= the target index count to the right */ \
            list->items[(size_t)i + count] = list->items[i]; \
        } \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            list->items[index + i] = item; \
        } \
        list->count += count; \
    } \
    \
    void TList##_set(TList const list, size_t const index, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_set)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_set)); \
        list->items[index] = item; \
    } \
    \
    void TList##_removeAt(TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_removeAt)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_removeAt)); \
        \
        for (size_t i = index; i < list->count; i += 1) { \
            /* Shift each item at an index > the target index one to the left */ \
            list->items[i] = list->items[i + 1]; \
        } \
        list->count -= 1; \
    } \
    \
    void TList##_removeManyAt(TList const list, size_t const startIndex, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_removeManyAt)); \
        TList##_guardStartIndexAndCountInRange(list, startIndex, count, STRINGIFY(TList##_removeManyAt)); \
        \
        for (size_t i = startIndex; i < list->count; i += 1) { \
            /* Shift each item at an index > the start index count to the left */ \
            list->items[i] = list->items[i + count]; \
        } \
        list->count -= count; \
    } \
    \
    void TList##_clear(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_clear)); \
        list->count = 0; \
    } \
    \
    void TList##_forEach(Const##TList const list, void * const state, TList##ForEachCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_forEach)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            callback(state, i, item); \
        } \
    } \
    \
    void TList##_forEachReverse(Const##TList const list, void * const state, TList##ForEachCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_forEachReverse)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            callback(state, (size_t)i, item); \
        } \
    } \
    \
    bool TList##_has(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_has)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return true; \
            } \
        } \
        \
        return false; \
    } \
    \
    size_t TList##_indexOf(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_indexOf)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    size_t TList##_lastIndexOf(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_lastIndexOf)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return (size_t)i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    bool TList##_findHas(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findHas)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return true; \
            } \
        } \
        \
        return false; \
    } \
    \
    TList##FindItemResult TList##_find(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_find)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return TList##FindItemResult_success(item); \
            } \
        } \
        \
        return TList##FindItemResult_failure(NULL); \
    } \
    \
    size_t TList##_findIndex(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findIndex)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    TList##FindItemResult TList##_findLast(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findLast)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, (size_t)i, item); \
            if (found) { \
                return TList##FindItemResult_success(item); \
            } \
        } \
        \
        return TList##FindItemResult_failure(NULL); \
    } \
    \
    size_t TList##_findLastIndex(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findLastIndex)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, (size_t)i, item); \
            if (found) { \
                return (size_t)i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    TList##Enumerator TList##_enumerate(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_enumerate)); \
        return TList##Enumerator_create(list, 1); \
    } \
    \
    TList##Enumerator TList##_enumerateReverse(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_enumerateReverse)); \
        return TList##Enumerator_create(list, -1); \
    } \
    \
    void TList##_fillArray(Const##TList const list, TItem * const array, size_t const startIndex, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_fillArray)); \
        guardNotNull(array, "array", STRINGIFY(TList##_fillArray)); \
        TList##_guardStartIndexAndCountInRange(list, startIndex, count, STRINGIFY(TList##_fillArray)); \
        \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = list->items[i + startIndex]; \
            array[i] = item; \
        } \
    } \
    \
    static void TList##_ensureCapacity(TList const list, size_t const requiredCapacity) { \
        assert(list != NULL); \
        \
        if (requiredCapacity <= list->capacity) { \
            return; \
        } \
        \
        size_t newCapacity = list->capacity == 0 ? 4 : (list->capacity * 2); \
        while (newCapacity < requiredCapacity) { \
            newCapacity *= 2; \
        } \
        \
        list->items = safeRealloc(list->items, sizeof *list->items * newCapacity, STRINGIFY(TList##_ensureCapacity)); \
        list->capacity = newCapacity; \
    } \
    \
    static void TList##_guardIndexInRange(Const##TList const list, size_t const index, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        guardFmt( \
            index < list->count, \
            "%s: Index (%zu) must be in range (count: %zu)", \
            callerName, \
            index, \
            list->count \
        ); \
    } \
    \
    static void TList##_guardIndexInInsertRange(Const##TList const list, size_t const index, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        guardFmt( \
            index <= list->count, \
            "%s: Index (%zu) must be in range (count: %zu) or the next available index", \
            callerName, \
            index, \
            list->count \
        ); \
    } \
    \
    static void TList##_guardStartIndexAndCountInRange(Const##TList const list, size_t const startIndex, size_t const count, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        TList##_guardIndexInRange(list, startIndex, callerName); \
        \
        size_t const endIndex = startIndex + count - 1; \
        guardFmt( \
            endIndex < list->count, \
            "%s: End index (%zu) must be within range (count: %zu)", \
            callerName, \
            endIndex, \
            list->count \
        ); \
    }

/**
 * Declare (.h file) a generic CircularLinkedList class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_CIRCULAR_LINKED_LIST(TList, TItem) \
    struct TList##Node; \
    typedef struct TList##Node * TList##Node; \
    typedef struct TList##Node const * Const##TList##Node; \
    \
    struct TList; \
    typedef struct TList * TList; \
    typedef struct TList const * Const##TList; \
    \
    TList TList##_create(void); \
    TList TList##_fromItems(TItem const *items, size_t count); \
    void TList##_destroy(TList list); \
    \
    TList##Node TList##_head(TList list); \
    Const##TList##Node TList##_constHead(Const##TList list); \
    bool TList##_empty(Const##TList list); \
    TItem TList##_item(Const##TList list, Const##TList##Node node); \
    TItem *TList##_itemPtr(TList list, TList##Node node); \
    TItem const *TList##_constItemPtr(Const##TList list, Const##TList##Node node); \
    TList##Node TList##_previous(TList list, TList##Node node); \
    Const##TList##Node TList##_constPrevious(Const##TList list, Const##TList##Node node); \
    TList##Node TList##_next(TList list, TList##Node node); \
    Const##TList##Node TList##_constNext(Const##TList list, Const##TList##Node node); \
    \
    TList##Node TList##_add(TList list, TItem item); \
    TList##Node TList##_addMany(TList list, TItem const *items, size_t count); \
    TList##Node TList##_insertBefore(TList list, TList##Node node, TItem item); \
    TList##Node TList##_insertManyBefore(TList list, TList##Node node, TItem const *items, size_t count); \
    TList##Node TList##_insertAfter(TList list, TList##Node node, TItem item); \
    TList##Node TList##_insertManyAfter(TList list, TList##Node node, TItem const *items, size_t count); \
    void TList##_set(TList list, TList##Node node, TItem item); \
    \
    TList##Node TList##_remove(TList list, TList##Node node); \
    void TList##_clear(TList list);

/**
 * Define (.c file) a CircularLinkedList List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_CIRCULAR_LINKED_LIST(TList, TItem) \
    DECLARE_CIRCULAR_LINKED_LIST(TList, TItem) \
    \
    struct TList##Node { \
        TItem item; \
        TList##Node previous; \
        TList##Node next; \
    }; \
    \
    struct TList { \
        TList##Node head; \
    }; \
    \
    TList TList##_create(void) { \
        TList const list = safeMalloc(sizeof *list, STRINGIFY(TList##_create)); \
        list->head = NULL; \
        return list; \
    } \
    \
    TList TList##_fromItems(TItem const * const items, size_t const count) { \
        guardNotNull(items, "items", STRINGIFY(TList##_fromItems)); \
        \
        TList const list = TList##_create(); \
        TList##_addMany(list, items, count); \
        return list; \
    } \
    \
    void TList##_destroy(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_destroy)); \
        \
        TList##_clear(list); \
        free(list); \
    } \
    \
    TList##Node TList##_head(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_head)); \
        return list->head; \
    } \
    \
    Const##TList##Node TList##_constHead(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constHead)); \
        return list->head; \
    } \
    \
    bool TList##_empty(Const##TList list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constHead)); \
        return list->head == NULL; \
    } \
    \
    TItem TList##_item(Const##TList const list, Const##TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_item)); \
        guardNotNull(node, "node", STRINGIFY(TList##_item)); \
        return node->item; \
    } \
    \
    TItem *TList##_itemPtr(TList const list, TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_itemPtr)); \
        guardNotNull(node, "node", STRINGIFY(TList##_itemPtr)); \
        return &node->item; \
    } \
    \
    TItem const *TList##_constItemPtr(Const##TList const list, Const##TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constItemPtr)); \
        guardNotNull(node, "node", STRINGIFY(TList##_constItemPtr)); \
        return &node->item; \
    } \
    \
    TList##Node TList##_previous(TList const list, TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_previous)); \
        guardNotNull(node, "node", STRINGIFY(TList##_previous)); \
        return node->previous; \
    } \
    \
    Const##TList##Node TList##_constPrevious(Const##TList const list, Const##TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constPrevious)); \
        guardNotNull(node, "node", STRINGIFY(TList##_constPrevious)); \
        return node->previous; \
    } \
    \
    TList##Node TList##_next(TList const list, TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_next)); \
        guardNotNull(node, "node", STRINGIFY(TList##_next)); \
        return node->next; \
    } \
    \
    Const##TList##Node TList##_constNext(Const##TList const list, Const##TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constNext)); \
        guardNotNull(node, "node", STRINGIFY(TList##_constNext)); \
        return node->next; \
    } \
    \
    TList##Node TList##_add(TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_add)); \
        \
        if (list->head != NULL) { \
            return TList##_insertBefore(list, list->head, item); \
        } \
        \
        TList##Node const head = safeMalloc(sizeof *head, STRINGIFY(TList##_add)); \
        head->item = item; \
        \
        list->head = head; \
        \
        head->previous = head; \
        head->next = head; \
        \
        return head; \
    } \
    \
    TList##Node TList##_addMany(TList const list, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_addMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_addMany)); \
        \
        TList##Node currentNode = NULL; \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            currentNode = TList##_add(list, item); \
        } \
        return currentNode; \
    } \
    \
    TList##Node TList##_insertBefore(TList const list, TList##Node const node, TItem const item) { \
        guardNotNull(node, "node", STRINGIFY(TList##_insertBefore)); \
        return TList##_insertAfter(list, node->previous, item); \
    } \
    \
    TList##Node TList##_insertManyBefore(TList const list, TList##Node const node, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insertManyBefore)); \
        guardNotNull(node, "node", STRINGIFY(TList##_insertManyBefore)); \
        guardNotNull(items, "items", STRINGIFY(TList##_insertManyBefore)); \
        \
        TList##Node currentNode = node; \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            currentNode = TList##_insertBefore(list, currentNode, item); \
        } \
        return currentNode; \
    } \
    \
    TList##Node TList##_insertAfter(TList const list, TList##Node const node, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insertAfter)); \
        guardNotNull(node, "node", STRINGIFY(TList##_insertAfter)); \
        \
        TList##Node const newNextNode = safeMalloc(sizeof *newNextNode, STRINGIFY(TList##_insertAfter)); \
        newNextNode->item = item; \
        \
        TList##Node const oldNextNode = node->next; \
        \
        node->next = newNextNode; \
        oldNextNode->previous = newNextNode; \
        \
        newNextNode->previous = node; \
        newNextNode->next = oldNextNode; \
        \
        return newNextNode; \
    } \
    \
    TList##Node TList##_insertManyAfter(TList const list, TList##Node const node, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insertManyAfter)); \
        guardNotNull(node, "node", STRINGIFY(TList##_insertManyAfter)); \
        guardNotNull(items, "items", STRINGIFY(TList##_insertManyAfter)); \
        \
        TList##Node currentNode = node; \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            currentNode = TList##_insertAfter(list, currentNode, item); \
        } \
        return currentNode; \
    } \
    \
    void TList##_set(TList const list, TList##Node const node, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_set)); \
        guardNotNull(node, "node", STRINGIFY(TList##_set)); \
        node->item = item; \
    } \
    \
    TList##Node TList##_remove(TList const list, TList##Node const node) { \
        guardNotNull(list, "list", STRINGIFY(TList##_remove)); \
        guardNotNull(node, "node", STRINGIFY(TList##_remove)); \
        \
        TList##Node nextNode = NULL; \
        if (node == node->next) { \
            list->head = NULL; \
        } else { \
            TList##Node const previousNode = node->previous; \
            nextNode = node->next; \
            \
            previousNode->next = nextNode; \
            nextNode->previous = previousNode; \
            \
            if (node == list->head) { \
                list->head = nextNode; \
            } \
        } \
        \
        free(node); \
        return nextNode; \
    } \
    \
    void TList##_clear(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_clear)); \
        \
        TList##Node const head = list->head; \
        if (head == NULL) { \
            return; \
        } \
        \
        TList##Node currentNode = head; \
        while (true) { \
            TList##Node const nextNode = currentNode->next; \
            free(currentNode); \
            \
            if (nextNode == head) { \
                break; \
            } \
            currentNode = nextNode; \
        } \
        \
        list->head = NULL; \
    }

time_t safeTime(char const *callerDescription);

DECLARE_FUNC(PthreadCreateStartRoutine, void *, void *)

pthread_t safePthreadCreate(
    pthread_attr_t const *attributes,
    PthreadCreateStartRoutine startRoutine,
    void *startRoutineArg,
    char const *callerDescription
);
void *safePthreadJoin(pthread_t threadId, char const *callerDescription);

void safeMutexInit(
    pthread_mutex_t *mutexOutPtr,
    pthread_mutexattr_t const *attributes,
    char const *callerDescription
);
void safeMutexLock(pthread_mutex_t *mutexPtr, char const *callerDescription);
void safeMutexUnlock(pthread_mutex_t *mutexPtr, char const *callerDescription);
void safeMutexDestroy(pthread_mutex_t *mutexPtr, char const *callerDescription);

void safeConditionInit(
    pthread_cond_t *conditionOutPtr,
    pthread_condattr_t const *attributes,
    char const *callerDescription
);
void safeConditionSignal(pthread_cond_t *conditionPtr, char const *callerDescription);
void safeConditionWait(
    pthread_cond_t *conditionPtr,
    pthread_mutex_t *mutexPtr,
    char const *callerDescription
);
void safeConditionDestroy(pthread_cond_t *conditionPtr, char const *callerDescription);

struct StringBuilder;
typedef struct StringBuilder * StringBuilder;
typedef struct StringBuilder const * ConstStringBuilder;

StringBuilder StringBuilder_create(void);
StringBuilder StringBuilder_fromChars(char const *value, size_t count);
StringBuilder StringBuilder_fromString(char const *value);
void StringBuilder_destroy(StringBuilder builder);

char const *StringBuilder_chars(ConstStringBuilder builder);
size_t StringBuilder_length(ConstStringBuilder builder);

void StringBuilder_appendChar(StringBuilder builder, char value);
void StringBuilder_appendChars(StringBuilder builder, char const *value, size_t count);
void StringBuilder_append(StringBuilder builder, char const *value);
void StringBuilder_appendFmt(StringBuilder builder, char const *valueFormat, ...);
void StringBuilder_appendFmtVA(StringBuilder builder, char const *valueFormat, va_list valueFormatArgs);
void StringBuilder_appendLine(StringBuilder builder, char const *value);
void StringBuilder_appendLineFmt(StringBuilder builder, char const *valueFormat, ...);
void StringBuilder_appendLineFmtVA(StringBuilder builder, char const *valueFormat, va_list valueFormatArgs);

void StringBuilder_insertChar(StringBuilder builder, size_t index, char value);
void StringBuilder_insertChars(StringBuilder builder, size_t index, char const *value, size_t count);
void StringBuilder_insert(StringBuilder builder, size_t index, char const *value);
void StringBuilder_insertFmt(StringBuilder builder, size_t index, char const *valueFormat, ...);
void StringBuilder_insertFmtVA(StringBuilder builder, size_t index, char const *valueFormat, va_list valueFormatArgs);

void StringBuilder_removeAt(StringBuilder builder, size_t index);
void StringBuilder_removeManyAt(StringBuilder builder, size_t startIndex, size_t count);

char *StringBuilder_toString(ConstStringBuilder builder);
char *StringBuilder_toStringAndDestroy(StringBuilder builder);

size_t safeSnprintf(
    char *buffer,
    size_t bufferLength,
    char const *callerDescription,
    char const *format,
    ...
);
size_t safeVsnprintf(
    char *buffer,
    size_t bufferLength,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);
size_t safeSprintf(
    char *buffer,
    char const *callerDescription,
    char const *format,
    ...
);
size_t safeVsprintf(
    char *buffer,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

char *formatString(char const *format, ...);
char *formatStringVA(char const *format, va_list formatArgs);

void safeRegcomp(regex_t *regexPtr, char const *pattern, int flags, char const *callerDescription);

void initializeRandom(unsigned int seed);

int randomInt(int minInclusive, int maxExclusive);

DECLARE_LIST(CharList, char)
DECLARE_LIST(StringList, char *)

FILE *safeFopen(char const *filePath, char const *modes, char const *callerDescription);

unsigned int safeFprintf(
    FILE *file,
    char const *callerDescription,
    char const *format,
    ...
);
unsigned int safeVfprintf(
    FILE *file,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

bool safeFgetc(char *charPtr, FILE *file, char const *callerDescription);
bool safeFgets(char *buffer, size_t bufferLength, FILE *file, char const *callerDescription);

char *readFileLine(FILE *file);

char *readAllFileText(char const *filePath);

int safeFscanf(
    FILE *file,
    char const *callerDescription,
    char const *format,
    ...
);
int safeVfscanf(
    FILE *file,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

bool scanFileExact(
    FILE *file,
    unsigned int expectedMatchCount,
    char const *format,
    ...
);
bool scanFileExactVA(
    FILE *file,
    unsigned int expectedMatchCount,
    char const *format,
    va_list formatArgs
);

struct HW8TransactionRecord {
    char const *name;
    char const *filePath;
};

void hw8(struct HW8TransactionRecord const *transactionRecords, size_t transactionRecordCount);

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

/**
 * Abort program execution after printing the specified error message to stderr.
 *
 * @param errorMessage The error message, not terminated by a newline.
 */
void abortWithError(char const * const errorMessage) {
    guardNotNull(errorMessage, "errorMessage", "abortWithError");

    fputs(errorMessage, stderr);
    fputc('\n', stderr);
    abort();
}

/**
 * Abort program execution after formatting and printing the specified error message to stderr.
 *
 * @param errorMessage The error message format (printf), not terminated by a newline.
 * @param ... The error message format arguments (printf).
 */
void abortWithErrorFmt(char const * const errorMessageFormat, ...) {
    va_list errorMessageFormatArgs;
    va_start(errorMessageFormatArgs, errorMessageFormat);
    abortWithErrorFmtVA(errorMessageFormat, errorMessageFormatArgs);
    va_end(errorMessageFormatArgs);
}

/**
 * Abort program execution after formatting and printing the specified error message to stderr.
 *
 * @param errorMessage The error message format (printf), not terminated by a newline.
 * @param errorMessageFormatArgs The error message format arguments (printf).
 */
void abortWithErrorFmtVA(char const * const errorMessageFormat, va_list errorMessageFormatArgs) {
    guardNotNull(errorMessageFormat, "errorMessageFormat", "abortWithErrorFmtVA");

    char * const errorMessage = formatStringVA(errorMessageFormat, errorMessageFormatArgs);
    abortWithError(errorMessage);
    free(errorMessage);
}

/**
 * Open the file using fopen. If the operation fails, abort the program with an error message.
 *
 * @param filePath The file path.
 * @param modes The fopen modes string.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The opened file.
 */
FILE *safeFopen(char const * const filePath, char const * const modes, char const * const callerDescription) {
    guardNotNull(filePath, "filePath", "safeFopen");
    guardNotNull(modes, "modes", "safeFopen");
    guardNotNull(callerDescription, "callerDescription", "safeFopen");

    FILE * const file = fopen(filePath, modes);
    if (file == NULL) {
        int const fopenErrorCode = errno;
        char const * const fopenErrorMessage = strerror(fopenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open file \"%s\" with modes \"%s\" using fopen (error code: %d; error message: \"%s\")",
            callerDescription,
            filePath,
            modes,
            fopenErrorCode,
            fopenErrorMessage
        );
        return NULL;
    }

    return file;
}

/**
 * Print a formatted string to the given file. If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The format (printf).
 * @param ... The format arguments (printf).
 *
 * @returns The number of charactes printed (no string terminator character).
 */
unsigned int safeFprintf(
    FILE * const file,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    unsigned int const printedCharCount = safeVfprintf(file, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return printedCharCount;
}

/**
 * Print a formatted string to the given file. If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param format The format (printf).
 * @param formatArgs The format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of charactes printed (no string terminator character).
 */
unsigned int safeVfprintf(
    FILE * const file,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(file, "file", "safeVfprintf");
    guardNotNull(format, "format", "safeVfprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVfprintf");

    int const printedCharCount = vfprintf(file, format, formatArgs);
    if (printedCharCount < 0) {
        int const vfprintfErrorCode = errno;
        char const * const vfprintfErrorMessage = strerror(vfprintfErrorCode);

        abortWithErrorFmt(
            "%s: Failed to print format \"%s\" to file using vfprintf (error code: %d; error message: \"%s\")",
            callerDescription,
            format,
            vfprintfErrorCode,
            vfprintfErrorMessage
        );
        return (unsigned int)-1;
    }

    return (unsigned int)printedCharCount;
}

/**
 * Read a character from the given file. If the operation fails, abort the program with an error message.
 *
 * @param charPtr The location to store the read character.
 * @param file The file to read from.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns Whether the end-of-file was hit.
 */
bool safeFgetc(
    char * const charPtr,
    FILE * const file,
    char const * const callerDescription
) {
    guardNotNull(charPtr, "charPtr", "safeFgetc");
    guardNotNull(file, "file", "safeFgetc");
    guardNotNull(callerDescription, "callerDescription", "safeFgetc");

    int const fgetcResult = fgetc(file);
    if (fgetcResult == EOF) {
        bool const fgetcError = ferror(file);
        if (fgetcError) {
            int const fgetcErrorCode = errno;
            char const * const fgetcErrorMessage = strerror(fgetcErrorCode);

            abortWithErrorFmt(
                "%s: Failed to read char from file using fgetc (error code: %d; error message: \"%s\")",
                callerDescription,
                fgetcErrorCode,
                fgetcErrorMessage
            );
            return false;
        }

        // EOF
        return false;
    }

    *charPtr = (char)fgetcResult;
    return true;
}

/**
 * Read characters from the given file into the given buffer. Stop as soon as one of the following conditions has been
 * met: (A) `bufferLength - 1` characters have been read, (B) a newline is encountered, or (C) the end of the file is
 * reached. The string read into the buffer will end with a terminating character. If the operation fails, abort the
 * program with an error message.
 *
 * @param buffer The buffer into which to read the string.
 * @param bufferLength The length of the buffer.
 * @param file The file to read from.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns Whether unread characters remain.
 */
bool safeFgets(
    char * const buffer,
    size_t const bufferLength,
    FILE * const file,
    char const * const callerDescription
) {
    guardNotNull(buffer, "buffer", "safeFgets");
    guardNotNull(file, "file", "safeFgets");
    guardNotNull(callerDescription, "callerDescription", "safeFgets");

    char * const fgetsResult = fgets(buffer, (int)bufferLength, file);
    bool const fgetsError = ferror(file);
    if (fgetsError) {
        int const fgetsErrorCode = errno;
        char const * const fgetsErrorMessage = strerror(fgetsErrorCode);

        abortWithErrorFmt(
            "%s: Failed to read %zu chars from file using fgets (error code: %d; error message: \"%s\")",
            callerDescription,
            bufferLength,
            fgetsErrorCode,
            fgetsErrorMessage
        );
        return false;
    }

    if (fgetsResult == NULL || feof(file)) {
        return false;
    }

    return true;
}

/**
 * Read a line from the file. If the current file position is EOF, return null.
 *
 * @param file The file to read from.
 *
 * @returns The line (the caller is responsible for freeing this memory), or null if the current file position is EOF.
 */
char *readFileLine(FILE * const file) {
    guardNotNull(file, "file", "readFileLine");

    StringBuilder const lineBuilder = StringBuilder_create();

    bool lineBeginsAtEof = true;
    char readCharacter;
    while (safeFgetc(&readCharacter, file, "readFileLine")) {
        lineBeginsAtEof = false;

        if (readCharacter == '\n') {
            break;
        }

        StringBuilder_appendChar(lineBuilder, readCharacter);
    }

    if (lineBeginsAtEof) {
        StringBuilder_destroy(lineBuilder);
        return NULL;
    }

    char * const line = StringBuilder_toStringAndDestroy(lineBuilder);
    return line;
}

/**
 * Open a text file, read all the text in the file into a string, and then close the file.
 *
 * @param filePath The path to the file.
 *
 * @returns A string containing all text in the file. The caller is responsible for freeing this memory.
 */
char *readAllFileText(char const * const filePath) {
    guardNotNull(filePath, "filePath", "readAllFileText");

    StringBuilder const fileTextBuilder = StringBuilder_create();

    FILE * const file = safeFopen(filePath, "r", "readAllFileText");
    char fgetsBuffer[100];
    while (safeFgets(fgetsBuffer, 100, file, "readAllFileText")) {
        StringBuilder_append(fileTextBuilder, fgetsBuffer);
    }
    fclose(file);

    char * const fileText = StringBuilder_toStringAndDestroy(fileTextBuilder);

    return fileText;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The format (scanf).
 * @param ... The format arguments (scanf).
 *
 * @returns The number of input items successfully matched and assigned, which can be fewer than provided for, or even
 *          zero in the event of an early matching failure. EOF is returned if the end of input is reached before either
 *          the first successful conversion or a matching failure occurs.
 */
int safeFscanf(
    FILE * const file,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    int const matchCount = safeVfscanf(file, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return matchCount;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param format The format (scanf).
 * @param formatArgs The format arguments (scanf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of input items successfully matched and assigned, which can be fewer than provided for, or even
 *          zero in the event of an early matching failure. EOF is returned if the end of input is reached before either
 *          the first successful conversion or a matching failure occurs.
 */
int safeVfscanf(
    FILE * const file,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(file, "file", "safeVfscanf");
    guardNotNull(format, "format", "safeVfscanf");
    guardNotNull(callerDescription, "callerDescription", "safeVfscanf");

    int const matchCount = vfscanf(file, format, formatArgs);
    bool const vfscanfError = ferror(file);
    if (vfscanfError) {
        int const vfscanfErrorCode = errno;
        char const * const vfscanfErrorMessage = strerror(vfscanfErrorCode);

        abortWithErrorFmt(
            "%s: Failed to read format \"%s\" from file using vfscanf (error code: %d; error message: \"%s\")",
            callerDescription,
            format,
            vfscanfErrorCode,
            vfscanfErrorMessage
        );
        return -1;
    }

    return matchCount;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the number of matched items does not match the expected count or if the operation fails, abort the program with an
 * error message.
 *
 * @param file The file.
 * @param expectedMatchCount The number of items in the format expected to be matched.
 * @param format The format (scanf).
 * @param ... The format arguments (scanf).
 *
 * @returns True if the format was scanned, or false if the end of the file was met.
 */
bool scanFileExact(
    FILE * const file,
    unsigned int const expectedMatchCount,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    bool const scanned = scanFileExactVA(file, expectedMatchCount, format, formatArgs);
    va_end(formatArgs);
    return scanned;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the number of matched items does not match the expected count or if the operation fails, abort the program with an
 * error message.
 *
 * @param file The file.
 * @param expectedMatchCount The number of items in the format expected to be matched.
 * @param format The format (scanf).
 * @param formatArgs The format arguments (scanf).
 *
 * @returns True if the format was scanned, or false if the end of the file was met.
 */
bool scanFileExactVA(
    FILE * const file,
    unsigned int const expectedMatchCount,
    char const * const format,
    va_list formatArgs
) {
    guardNotNull(file, "file", "scanFileExactVA");
    guardNotNull(format, "format", "scanFileExactVA");

    int const matchCount = safeVfscanf(file, format, formatArgs, "scanFileExactVA");
    if (matchCount == EOF) {
        return false;
    }

    if ((unsigned int)matchCount != expectedMatchCount) {
        abortWithErrorFmt(
            "scanFileExactVA: Failed to parse exact format \"%s\" from file"
            " (expected match count: %u; actual match count: %d)",
            format,
            expectedMatchCount,
            matchCount
        );
        return false;
    }

    return true;
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessage The error message.
 */
void guard(bool const expression, char const * const errorMessage) {
    if (errorMessage == NULL) {
        abortWithError("guard: errorMessage must not be null");
        return;
    }

    if (expression) {
        return;
    }

    abortWithError(errorMessage);
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessageFormat The error message format (printf).
 * @param ... The error message format arguments (printf).
 */
void guardFmt(bool const expression, char const * const errorMessageFormat, ...) {
    va_list errorMessageFormatArgs;
    va_start(errorMessageFormatArgs, errorMessageFormat);
    guardFmtVA(expression, errorMessageFormat, errorMessageFormatArgs);
    va_end(errorMessageFormatArgs);
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessageFormat The error message format (printf).
 * @param errorMessageFormatArgs The error message format arguments (printf).
 */
void guardFmtVA(bool const expression, char const * const errorMessageFormat, va_list errorMessageFormatArgs) {
    if (errorMessageFormat == NULL) {
        abortWithError("guardFmtVA: errorMessageFormat must not be null");
        return;
    }

    if (expression) {
        return;
    }

    abortWithErrorFmtVA(errorMessageFormat, errorMessageFormatArgs);
}

/**
 * Ensure that the given object supplied by a parameter is not null. If it is null, abort the program with an error
 * message.
 *
 * @param object The object to verify is not null.
 * @param paramName The name of the parameter supplying the object.
 * @param callerName The name of the calling function.
 */
void guardNotNull(void const * const object, char const * const paramName, char const * const callerName) {
    guard(paramName != NULL, "guardNotNull: paramName must not be null");
    guard(callerName != NULL, "guardNotNull: callerName must not be null");

    guardFmt(object != NULL, "%s: %s must not be null", callerName, paramName);
}

DEFINE_LIST(CharList, char)
DEFINE_LIST(StringList, char *)

/**
 * Allocate memory of the given size using malloc. If the allocation fails, abort the program with an error message.
 *
 * @param size The size of the memory, in bytes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The allocated memory.
 */
void *safeMalloc(size_t const size, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeMalloc");

    void * const memory = malloc(size);
    if (memory == NULL) {
        int const mallocErrorCode = errno;
        char const * const mallocErrorMessage = strerror(mallocErrorCode);

        abortWithErrorFmt(
            "%s: Failed to allocate %zu bytes of memory using malloc (error code: %d; error message: \"%s\")",
            callerDescription,
            size,
            mallocErrorCode,
            mallocErrorMessage
        );
        return NULL;
    }

    return memory;
}

/**
 * Resize the given memory using realloc. If the reallocation fails, abort the program with an error message.
 *
 * @param memory The existing memory, or null.
 * @param newSize The new size of the memory, in bytes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The reallocated memory.
 */
void *safeRealloc(void * const memory, size_t const newSize, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeRealloc");

    void * const newMemory = realloc(memory, newSize);
    if (newMemory == NULL) {
        int const reallocErrorCode = errno;
        char const * const reallocErrorMessage = strerror(reallocErrorCode);

        abortWithErrorFmt(
            "%s: Failed to reallocate memory to %zu bytes using realloc (error code: %d; error message: \"%s\")",
            callerDescription,
            newSize,
            reallocErrorCode,
            reallocErrorMessage
        );
        return NULL;
    }

    return newMemory;
}

static bool randomInitialized = false;

static void ensureRandomInitialized(void);

/**
 * Initialize the random number generator using the given seed value. If this is never called, the random number
 * generator will automatically be initialized with the time it is first used.
 *
 * @param seed A number used to calculate a starting value for the pseudo-random number sequence.
 */
void initializeRandom(unsigned int const seed) {
    srand(seed);
    randomInitialized = true;
}

/**
 * Generate the next random integer from within the given range.
 *
 * @param minInclusive The inclusive lower bound of the random number returned.
 * @param maxExclusive The exclusive upper bound of the random number returned. maxExclusive must be greater than
 *                     minInclusive.
 *
 * @returns The random integer.
 */
int randomInt(int const minInclusive, int const maxExclusive) {
    guardFmt(
        maxExclusive > minInclusive,
        "randomInt: maxExclusive (%d) must be greater than minInclusive (%d)",
        maxExclusive,
        minInclusive
    );

    ensureRandomInitialized();
    return rand() % (maxExclusive - minInclusive) + minInclusive;
}

static void ensureRandomInitialized(void) {
    if (randomInitialized) {
        return;
    }

    initializeRandom((unsigned int)safeTime("ensureRandomInitialized"));
    randomInitialized = true;
}

/**
 * Compile the given regular expression pattern and flags into a regex object using regcomp. If the compilation fails,
 * abort the program with an error message.
 *
 * @param regexPtr The location of the regex.
 * @param pattern The regular expression pattern.
 * @param flags The regular expression flags.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 * the calling function, plus extra information if useful.
 */
void safeRegcomp(regex_t * const regexPtr, char const * const pattern, int const flags, char const * const callerDescription) {
    guardNotNull(regexPtr, "regexPtr", "safeRegcomp");
    guardNotNull(pattern, "pattern", "safeRegcomp");
    guardNotNull(callerDescription, "callerDescription", "safeRegcomp");

    int const regcompErrorCode = regcomp(regexPtr, pattern, flags);
    if (regcompErrorCode != 0) {
        char regcompErrorMessage[100];
        regerror(regcompErrorCode, regexPtr, regcompErrorMessage, 100);

        abortWithErrorFmt(
            "%s: Failed to compile regular expression /%s/%d using regcomp (error code: %d; error message: \"%s\")",
            callerDescription,
            pattern,
            flags,
            regcompErrorCode,
            regcompErrorMessage
        );
    }
}

/**
 * If the given buffer is non-null, format the string into the buffer. If the buffer is null, simply calculate the
 * number of characters that would have been written if the buffer had been sufficiently large. If the operation fails,
 * abort the program with an error message.
 *
 * @param buffer The buffer into which to write, or null if only the formatted string length is desired.
 * @param bufferLength The length of the buffer, or 0 if the buffer is null.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The number of characters that would have been written if the buffer had been sufficiently large, not
 *          counting the terminating null character.
 */
size_t safeSnprintf(
    char * const buffer,
    size_t const bufferLength,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    size_t const length = safeVsnprintf(buffer, bufferLength, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return length;
}

/**
 * If the given buffer is non-null, format the string into the buffer. If the buffer is null, simply calculate the
 * number of characters that would have been written if the buffer had been sufficiently large. If the operation fails,
 * abort the program with an error message.
 *
 * @param buffer The buffer into which to write, or null if only the formatted string length is desired.
 * @param bufferLength The length of the buffer, or 0 if the buffer is null.
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of characters that would have been written if the buffer had been sufficiently large, not
 *          counting the terminating null character.
 */
size_t safeVsnprintf(
    char * const buffer,
    size_t const bufferLength,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(format, "format", "safeVsnprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVsnprintf");

    int const vsnprintfResult = vsnprintf(buffer, bufferLength, format, formatArgs);
    if (vsnprintfResult < 0) {
        abortWithErrorFmt(
            "%s: Failed to format string using vsnprintf (format: \"%s\"; result: %d)",
            callerDescription,
            format,
            vsnprintfResult
        );
        return (size_t)-1;
    }

    return (size_t)vsnprintfResult;
}

/**
 * Format the string into the buffer. If the operation fails, abort the program with an error message.
 *
 * @param buffer The buffer into which to write.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The number of characters written.
 */
size_t safeSprintf(
    char * const buffer,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    size_t const length = safeVsprintf(buffer, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return length;
}

/**
 * Format the string into the buffer. If the operation fails, abort the program with an error message.
 *
 * @param buffer The buffer into which to write.
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of characters written.
 */
size_t safeVsprintf(
    char * const buffer,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(buffer, "buffer", "safeVsprintf");
    guardNotNull(format, "format", "safeVsprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVsprintf");

    int const vsprintfResult = vsprintf(buffer, format, formatArgs);
    if (vsprintfResult < 0) {
        abortWithErrorFmt(
            "%s: Failed to format string using vsprintf (format: \"%s\"; result: %d)",
            callerDescription,
            format,
            vsprintfResult
        );
        return (size_t)-1;
    }

    return (size_t)vsprintfResult;
}

/**
 * Create a string using the specified format and format args.
 *
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The formatted string. The caller is responsible for freeing the memory.
 */
char *formatString(char const * const format, ...) {
    va_list formatArgs;
    va_start(formatArgs, format);
    char * const formattedString = formatStringVA(format, formatArgs);
    va_end(formatArgs);
    return formattedString;
}

/**
 * Create a string using the specified format and format args.
 *
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 *
 * @returns The formatted string. The caller is responsible for freeing the memory.
 */
char *formatStringVA(char const * const format, va_list formatArgs) {
    guardNotNull(format, "format", "formatStringVA");

    va_list formatArgsForVsprintf;
    va_copy(formatArgsForVsprintf, formatArgs);

    size_t const formattedStringLength = safeVsnprintf(NULL, 0, format, formatArgs, "formatStringVA");
    char * const formattedString = safeMalloc(sizeof *formattedString * (formattedStringLength + 1), "formatStringVA");

    safeVsprintf(formattedString, format, formatArgsForVsprintf, "formatStringVA");
    va_end(formatArgsForVsprintf);

    return formattedString;
}

/**
 * Represents a mutable string of characters with convenience methods for string manipulation.
 */
struct StringBuilder {
    CharList chars;
};

/**
 * Create an empty StringBuilder.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_create(void) {
    StringBuilder const builder = safeMalloc(sizeof *builder, "StringBuilder_create");
    builder->chars = CharList_create();
    return builder;
}

/**
 * Create a StringBuilder initialized with the given characters.
 *
 * @param value The characters.
 * @param count The number of characters.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_fromChars(char const * const value, size_t const count) {
    guardNotNull(value, "value", "StringBuilder_fromChars");

    StringBuilder const builder = safeMalloc(sizeof *builder, "StringBuilder_fromChars");
    builder->chars = CharList_fromItems(value, count);
    return builder;
}

/**
 * Create a StringBuilder initialized with the given string.
 *
 * @param value The string.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_fromString(char const * const value) {
    guardNotNull(value, "value", "StringBuilder_fromString");
    return StringBuilder_fromChars(value, strlen(value));
}

/**
 * Free the memory associated with the StringBuilder.
 *
 * @param builder The StringBuilder instance.
 */
void StringBuilder_destroy(StringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_destroy");

    CharList_destroy(builder->chars);
    free(builder);
}

/**
 * Get the characters that compose the current value.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns The current value as a character array. This array is not null-terminated.
 */
char const *StringBuilder_chars(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_chars");
    return CharList_items(builder->chars);
}

/**
 * Get the length of the current value.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns The string length of the current value.
 */
size_t StringBuilder_length(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_length");
    return CharList_count(builder->chars);
}

/**
 * Append the given character to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The character.
 */
void StringBuilder_appendChar(StringBuilder const builder, char const value) {
    guardNotNull(builder, "builder", "StringBuilder_appendChar");
    CharList_add(builder->chars, value);
}

/**
 * Append the given characters to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The characters.
 * @param count The number of characters.
 */
void StringBuilder_appendChars(StringBuilder const builder, char const * const value, size_t const count) {
    guardNotNull(builder, "builder", "StringBuilder_appendChars");
    guardNotNull(value, "value", "StringBuilder_appendChars");

    CharList_addMany(builder->chars, value, count);
}

/**
 * Append the given string to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The string.
 */
void StringBuilder_append(StringBuilder const builder, char const * const value) {
    guardNotNull(value, "value", "StringBuilder_append");
    StringBuilder_appendChars(builder, value, strlen(value));
}

/**
 * Append the string specified by the given format and format args to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_appendFmt(StringBuilder const builder, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_appendFmtVA(builder, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Append the string specified by the given format and format args to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_appendFmtVA(StringBuilder const builder, char const * const valueFormat, va_list valueFormatArgs) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_appendFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_append(builder, value);
    free(value);
}

/**
 * Append the given string, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The string.
 */
void StringBuilder_appendLine(StringBuilder const builder, char const * const value) {
    guardNotNull(builder, "builder", "StringBuilder_appendLine");
    guardNotNull(value, "value", "StringBuilder_appendLine");

    CharList_addMany(builder->chars, value, strlen(value));
    CharList_add(builder->chars, '\n');
}

/**
 * Append the string specified by the given format and format args, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_appendLineFmt(StringBuilder const builder, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_appendLineFmtVA(builder, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Append the string specified by the given format and format args, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_appendLineFmtVA(
    StringBuilder const builder,
    char const * const valueFormat,
    va_list valueFormatArgs
) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_appendLineFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_appendLine(builder, value);
    free(value);
}

/**
 * Insert the given character into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The character.
 */
void StringBuilder_insertChar(StringBuilder const builder, size_t const index, char const value) {
    guardNotNull(builder, "builder", "StringBuilder_insertChar");
    CharList_insert(builder->chars, index, value);
}

/**
 * Insert the given characters into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The characters.
 * @param count The number of characters.
 */
void StringBuilder_insertChars(
    StringBuilder const builder,
    size_t const index,
    char const * const value,
    size_t const count
) {
    guardNotNull(builder, "builder", "StringBuilder_insertChars");
    guardNotNull(value, "value", "StringBuilder_insertChars");

    CharList_insertMany(builder->chars, index, value, count);
}

/**
 * Insert the given string into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The string.
 */
void StringBuilder_insert(StringBuilder const builder, size_t const index, char const * const value) {
    guardNotNull(value, "value", "StringBuilder_insert");
    StringBuilder_insertChars(builder, index, value, strlen(value));
}

/**
 * Insert the string specified by the given format and format args into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_insertFmt(StringBuilder const builder, size_t const index, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_insertFmtVA(builder, index, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Insert the string specified by the given format and format args into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_insertFmtVA(
    StringBuilder const builder,
    size_t const index,
    char const * const valueFormat,
    va_list valueFormatArgs
) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_insertFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_insert(builder, index, value);
    free(value);
}

/**
 * Remove the character at the given index from the current value.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 */
void StringBuilder_removeAt(StringBuilder const builder, size_t const index) {
    guardNotNull(builder, "builder", "StringBuilder_removeAt");
    CharList_removeAt(builder->chars, index);
}

/**
 * Remove a series of characters starting at the given index from the current value.
 *
 * @param builder The StringBuilder instance.
 * @param startIndex The index at which to begin removal.
 * @param count The number of characters to remove.
 */
void StringBuilder_removeManyAt(StringBuilder const builder, size_t const startIndex, size_t const count) {
    guardNotNull(builder, "builder", "StringBuilder_removeManyAt");
    CharList_removeManyAt(builder->chars, startIndex, count);
}

/**
 * Convert the current value to a string.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns A newly allocated string containing the value. The caller is responsible for freeing this memory.
 */
char *StringBuilder_toString(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_toString");

    size_t const length = CharList_count(builder->chars);
    char * const value = safeMalloc(sizeof *value * (length + 1), "StringBuilder_toString");
    CharList_fillArray(builder->chars, value, 0, length);
    value[length] = '\0';
    return value;
}

/**
 * Convert the current value to a string, then destroy the StringBuilder.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns A newly allocated string containing the value. The caller is responsible for freeing this memory.
 */
char *StringBuilder_toStringAndDestroy(StringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_toStringAndDestroy");

    char * const valueString = StringBuilder_toString(builder);
    StringBuilder_destroy(builder);
    return valueString;
}

/**
 * Create a new thread. If the operation fails, abort the program with an error message.
 *
 * @param attributes The attributes with which to create the thread, or null to use the default attributes.
 * @param startRoutine The function to run in the new thread. This function will be called with startRoutineArg as its
 *                     sole argument. If this function returns, the effect is as if there was an implicit call to
 *                     pthread_exit() using the return value of startRoutine as the exit status.
 * @param startRoutineArg The argument to pass to startRoutine.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The ID of the newly created thread.
 */
pthread_t safePthreadCreate(
    pthread_attr_t const * const attributes,
    PthreadCreateStartRoutine const startRoutine,
    void * const startRoutineArg,
    char const * const callerDescription
) {
    guardNotNull(callerDescription, "callerDescription", "safePthreadCreate");

    pthread_t threadId;
    int const pthreadCreateErrorCode = pthread_create(&threadId, attributes, startRoutine, startRoutineArg);
    if (pthreadCreateErrorCode != 0) {
        char const * const pthreadCreateErrorMessage = strerror(pthreadCreateErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create new thread using pthread_create (error code: %d; error message: \"%s\")",
            callerDescription,
            pthreadCreateErrorCode,
            pthreadCreateErrorMessage
        );
        return (pthread_t)-1;
    }

    return threadId;
}

/**
 * Wait for the given thread to terminate. If the operation fails, abort the program with an error message.
 *
 * @param threadId The thread ID.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The thread's return value.
 */
void *safePthreadJoin(pthread_t const threadId, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safePthreadJoin");

    void *threadReturnValue;
    int const pthreadJoinErrorCode = pthread_join(threadId, &threadReturnValue);
    if (pthreadJoinErrorCode != 0) {
        char const * const pthreadJoinErrorMessage = strerror(pthreadJoinErrorCode);

        abortWithErrorFmt(
            "%s: Failed to join threads using pthread_join (error code: %d; error message: \"%s\")",
            callerDescription,
            pthreadJoinErrorCode,
            pthreadJoinErrorMessage
        );
        return NULL;
    }

    return threadReturnValue;
}

/**
 * Initialize the given mutex memory. If the operation fails, abort the program with an error message.
 *
 * @param mutexOutPtr A pointer to the memory where the mutex should be initialized. This pointer must be used directly
 *                    in all mutex-related functions (no copies).
 * @param attributes The attributes with which to create the mutex, or null to use the default attributes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeMutexInit(
    pthread_mutex_t * const mutexOutPtr,
    pthread_mutexattr_t const * const attributes,
    char const * const callerDescription
) {
    guardNotNull(callerDescription, "callerDescription", "safeMutexInit");

    int const mutexInitErrorCode = pthread_mutex_init(mutexOutPtr, attributes);
    if (mutexInitErrorCode != 0) {
        char const * const mutexInitErrorMessage = strerror(mutexInitErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create mutex using pthread_mutex_init (error code: %d; error message: \"%s\")",
            callerDescription,
            mutexInitErrorCode,
            mutexInitErrorMessage
        );
    }
}

/**
 * Lock the given mutex. If the operation fails, abort the program with an error message.
 *
 * @param mutexPtr A pointer to the mutex.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeMutexLock(pthread_mutex_t * const mutexPtr, char const * const callerDescription) {
    guardNotNull(mutexPtr, "mutexPtr", "safeMutexLock");
    guardNotNull(callerDescription, "callerDescription", "safeMutexLock");

    int const mutexLockErrorCode = pthread_mutex_lock(mutexPtr);
    if (mutexLockErrorCode != 0) {
        char const * const mutexLockErrorMessage = strerror(mutexLockErrorCode);

        abortWithErrorFmt(
            "%s: Failed to lock mutex using pthread_mutex_lock (error code: %d; error message: \"%s\")",
            callerDescription,
            mutexLockErrorCode,
            mutexLockErrorMessage
        );
    }
}

/**
 * Unlock the given mutex. If the operation fails, abort the program with an error message.
 *
 * @param mutexPtr A pointer to the mutex.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeMutexUnlock(pthread_mutex_t * const mutexPtr, char const * const callerDescription) {
    guardNotNull(mutexPtr, "mutexPtr", "safeMutexUnlock");
    guardNotNull(callerDescription, "callerDescription", "safeMutexUnlock");

    int const mutexUnlockErrorCode = pthread_mutex_unlock(mutexPtr);
    if (mutexUnlockErrorCode != 0) {
        char const * const mutexUnlockErrorMessage = strerror(mutexUnlockErrorCode);

        abortWithErrorFmt(
            "%s: Failed to unlock mutex using pthread_mutex_unlock (error code: %d; error message: \"%s\")",
            callerDescription,
            mutexUnlockErrorCode,
            mutexUnlockErrorMessage
        );
    }
}

/**
 * Destroy the given mutex. If the operation fails, abort the program with an error message.
 *
 * @param mutexPtr A pointer to the mutex.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeMutexDestroy(pthread_mutex_t * const mutexPtr, char const * const callerDescription) {
    guardNotNull(mutexPtr, "mutexPtr", "safeMutexDestroy");
    guardNotNull(callerDescription, "callerDescription", "safeMutexDestroy");

    int const mutexDestroyErrorCode = pthread_mutex_destroy(mutexPtr);
    if (mutexDestroyErrorCode != 0) {
        char const * const mutexDestroyErrorMessage = strerror(mutexDestroyErrorCode);

        abortWithErrorFmt(
            "%s: Failed to destroy mutex using pthread_mutex_destroy (error code: %d; error message: \"%s\")",
            callerDescription,
            mutexDestroyErrorCode,
            mutexDestroyErrorMessage
        );
    }
}

/**
 * Initialize the given condition memory. If the operation fails, abort the program with an error message.
 *
 * @param conditionOutPtr A pointer to the memory where the condition should be initialized. This pointer must be used
 *                        directly in all condition-related functions (no copies).
 * @param attributes The attributes with which to create the condition, or null to use the default attributes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeConditionInit(
    pthread_cond_t * const conditionOutPtr,
    pthread_condattr_t const * const attributes,
    char const * const callerDescription
) {
    guardNotNull(callerDescription, "callerDescription", "safeConditionInit");

    int const condInitErrorCode = pthread_cond_init(conditionOutPtr, attributes);
    if (condInitErrorCode != 0) {
        char const * const condInitErrorMessage = strerror(condInitErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create condition using pthread_cond_init (error code: %d; error message: \"%s\")",
            callerDescription,
            condInitErrorCode,
            condInitErrorMessage
        );
    }
}

/**
 * Signal the given condition. If the operation fails, abort the program with an error message.
 *
 * @param conditionPtr A pointer to the condition.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeConditionSignal(pthread_cond_t * const conditionPtr, char const * const callerDescription) {
    guardNotNull(conditionPtr, "conditionPtr", "safeConditionSignal");
    guardNotNull(callerDescription, "callerDescription", "safeConditionSignal");

    int const condSignalErrorCode = pthread_cond_signal(conditionPtr);
    if (condSignalErrorCode != 0) {
        char const * const condSignalErrorMessage = strerror(condSignalErrorCode);

        abortWithErrorFmt(
            "%s: Failed to signal condition using pthread_cond_signal (error code: %d; error message: \"%s\")",
            callerDescription,
            condSignalErrorCode,
            condSignalErrorMessage
        );
    }
}

/**
 * Wait for the given condition. If the operation fails, abort the program with an error message.
 *
 * @param conditionPtr A pointer to the condition.
 * @param mutexPtr A pointer to the mutex. The mutex must be locked.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeConditionWait(
    pthread_cond_t * const conditionPtr,
    pthread_mutex_t * const mutexPtr,
    char const * const callerDescription
) {
    guardNotNull(conditionPtr, "conditionPtr", "safeConditionWait");
    guardNotNull(mutexPtr, "mutexPtr", "safeConditionWait");
    guardNotNull(callerDescription, "callerDescription", "safeConditionWait");

    int const condWaitErrorCode = pthread_cond_wait(conditionPtr, mutexPtr);
    if (condWaitErrorCode != 0) {
        char const * const condWaitErrorMessage = strerror(condWaitErrorCode);

        abortWithErrorFmt(
            "%s: Failed to wait for condition using pthread_cond_wait (error code: %d; error message: \"%s\")",
            callerDescription,
            condWaitErrorCode,
            condWaitErrorMessage
        );
    }
}

/**
 * Destroy the given condition. If the operation fails, abort the program with an error message.
 *
 * @param conditionPtr A pointer to the condition.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeConditionDestroy(pthread_cond_t * const conditionPtr, char const * const callerDescription) {
    guardNotNull(conditionPtr, "conditionPtr", "safeConditionDestroy");
    guardNotNull(callerDescription, "callerDescription", "safeConditionDestroy");

    int const condDestroyErrorCode = pthread_cond_destroy(conditionPtr);
    if (condDestroyErrorCode != 0) {
        char const * const condDestroyErrorMessage = strerror(condDestroyErrorCode);

        abortWithErrorFmt(
            "%s: Failed to destroy condition using pthread_cond_destroy (error code: %d; error message: \"%s\")",
            callerDescription,
            condDestroyErrorCode,
            condDestroyErrorMessage
        );
    }
}

/**
 * Get the current time. If the operation fails, abort the program with an error message.
 *
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The current time.
 */
time_t safeTime(char const * const callerDescription) {
    time_t const timeResult = time(NULL);
    if (timeResult == -1) {
        int const timeErrorCode = errno;
        char const * const timeErrorMessage = strerror(timeErrorCode);

        abortWithErrorFmt(
            "%s: Failed to get current time using time (error code: %d; error message: \"%s\")",
            callerDescription,
            timeErrorCode,
            timeErrorMessage
        );
        return -1;
    }

    return timeResult;
}

/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW8
 */

int main(int const argc, char ** const argv) {
    static struct HW8TransactionRecord const transactionRecords[] = {
        {.name = "Vlad", .filePath = "Vlad.in"},
        {.name = "Frank", .filePath = "Frank.in"},
        {.name = "Bigfoot", .filePath = "Bigfoot.in"},
        {.name = "Casper", .filePath = "Casper.in"},
        {.name = "Gomez", .filePath = "Gomez.in"}
    };
    hw8(transactionRecords, ARRAY_LENGTH(transactionRecords));
    return EXIT_SUCCESS;
}

