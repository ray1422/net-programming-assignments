#pragma once

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ((type *)((void *)ptr - offsetof(type, member)))

#define list_next(ptr, type) ((type *)container_of(ptr->list_instance.next, type, list_instance))
struct list_instance {
    struct list_instance *prev, *next;
};
