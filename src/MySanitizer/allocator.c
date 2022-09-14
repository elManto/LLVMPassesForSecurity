#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"


void __init_all() {
    learnsan_init();
}

void* __learnsan_malloc(size_t size) {
    struct chunk_begin* p = malloc(sizeof(struct chunk_struct) + size);
    if (!p) 
        return NULL;

    learnsan_unpoison(p, sizeof(struct chunk_struct) + size);
    
    p->requested_size = size;
    p->aligned_orig = NULL; 
    p->next = p->prev = NULL;



    learnsan_poison(p->redzone, REDZONE_SIZE, ASAN_HEAP_LEFT_RZ);
    if (size & (ALLOC_ALIGN_SIZE - 1))
        learnsan_poison((char*)&p[1] + size, (size & ~(ALLOC_ALIGN_SIZE - 1) + 8 - size + REDZONE_SIZE),
                        ASAN_HEAP_RIGHT_RZ);
    else
        learnsan_poison((char*)&p[1] + size, REDZONE_SIZE, ASAN_HEAP_RIGHT_RZ);
    
    memset(&p[1], 0xff, size);

    return &p[1];
    
}



void __learnsan_free(void* ptr) {
    if (!ptr)
        return;
    
    struct chunk_begin* p = ptr;
    p -= 1;

    size_t n = p->requested_size;
    if (p->aligned_orig)
        free(p->aligned_orig);
    else
        free(p);

    if (n & (ALLOC_ALIGN_SIZE - 1))
        n = (n & !(ALLOC_ALIGN_SIZE - 1)) + ALLOC_ALIGN_SIZE;

    learnsan_poison(ptr, n, ASAN_HEAP_FREED);
}

int __learnsan_load(void* ptr, unsigned int size) {
    if (size == 1)
        return learnsan_load1(ptr);
    else if (size == 4)
        return learnsan_load4(ptr);
    else if (size == 8)
        return learnsan_load8(ptr);
    else
        return 0;
}

int __learnsan_store(void* ptr, unsigned int size){
    if (size == 1)
        return learnsan_store1(ptr);
    else if (size == 4)
        return learnsan_store4(ptr);
    else if (size == 8)
        return learnsan_store8(ptr);
    else
        return 0;
}
