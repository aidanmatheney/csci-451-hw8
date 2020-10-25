#pragma once

#include "../macro.h"
#include "../memory.h"
#include "../guard.h"

#include <stdlib.h>
#include <stdbool.h>

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
