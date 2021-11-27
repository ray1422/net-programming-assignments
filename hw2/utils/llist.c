#include "llist.h"

#include <stdio.h>
#include <stdlib.h>


typedef struct IntNode {
    int v;
    struct list_instance list_instance;
} IntNode;

static int llist_example() {
    IntNode *n0 = (IntNode *)malloc(sizeof(IntNode));
    n0->v = 10;
    IntNode *n1 = (IntNode *)malloc(sizeof(IntNode));

    n1->v = 1024;

    n0->list_instance.next = &n1->list_instance;
    n1->list_instance.prev = &n0->list_instance;
    IntNode *u = list_next(n0, IntNode);

    printf("%d\n", u->v);
}