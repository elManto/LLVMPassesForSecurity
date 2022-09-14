#include "learnsan.h"


#define REDZONE_SIZE 128
#define ALLOC_ALIGN_SIZE (_Alignof(max_align_t))

struct chunk_begin {
    size_t requested_size;
    void* aligned_orig;
    struct chunk_begin* next;
    struct chunk_begin* prev;
    char redzone[REDZONE_SIZE];
};

struct chunk_struct {
    struct chunk_begin begin;
    char redzone[REDZONE_SIZE];
    size_t prev_size_padding;
};

void __init_all();

void* __learnsan_malloc(size_t size);
void __learnsan_free(void* ptr);
int __learnsan_load(void* ptr, unsigned int size);
int __learnsan_store(void* ptr, unsigned int size);
