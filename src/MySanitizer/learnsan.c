#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

#define HIGH_SHADOW_ADDR ((void*)0x02008fff7000ULL)
#define LOW_SHADOW_ADDR ((void*)0x00007fff8000ULL)
#define GAP_SHADOW_ADDR ((void*)0x00008fff7000)

#define HIGH_SHADOW_SIZE (0xdfff0000fffULL)
#define LOW_SHADOW_SIZE (0xfffefffULL)
#define GAP_SHADOW_SIZE (0x1ffffffffff)

#define SHADOW_OFFSET (0x7fff8000ULL)
#define SHADOW_SCALE 3

#include "learnsan.h"
#define MEM_TO_SHADOW(mem) (((mem) >> SHADOW_SCALE) + (SHADOW_OFFSET))

void learnsan_init() {
   if (mmap(HIGH_SHADOW_ADDR, HIGH_SHADOW_SIZE, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_FIXED | MAP_NORESERVE | MAP_ANON, -1,
              0) == MAP_FAILED) {
        perror("Failed to mmap HIGH_SHADOW memory\n");
    }

    if(mmap(LOW_SHADOW_ADDR, LOW_SHADOW_SIZE, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_FIXED | MAP_NORESERVE | MAP_ANON, -1,
              0) == MAP_FAILED) {

        perror("Failed to mmap LOW_SHADOW memory\n");
    }

    if(mmap(GAP_SHADOW_ADDR, GAP_SHADOW_SIZE, PROT_NONE,
              MAP_PRIVATE | MAP_FIXED | MAP_NORESERVE | MAP_ANON, -1,
              0) == MAP_FAILED) {
        perror("Failed to mmap GAP_SHADOW memory\n");

    }
}

int learnsan_poison(void* ptr, size_t size, uint8_t poison_byte) {

    uintptr_t ptr_int = (uintptr_t) ptr;
    uintptr_t ptr_int_end = ptr_int + size;
    uintptr_t ptr_int_end_aligned = ptr_int_end & ~7;

    //fprintf(stderr, "poisoning mem at addr %lx of size %zu with byte %d\n", ptr_int, size, poison_byte);
    if (ptr_int & 7) {
        
        unsigned long ptr_int_aligned = (ptr_int & ~7) + 8;
        size_t first_size = ptr_int_aligned - ptr_int;

        if (size < first_size)
            return 0;

        uintptr_t mem = ptr_int;
        uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
        //fprintf(stderr, "\tshadow addr %p\n", shadow_addr);
        *shadow_addr = 8 - first_size;
        ptr_int = ptr_int_aligned;
    }

    while ( ptr_int < ptr_int_end_aligned) {
        uintptr_t mem = ptr_int;
        uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
        //fprintf(stderr, "\tshadow addr %p\n", shadow_addr);
        *shadow_addr = poison_byte;
        ptr_int += 8;
    }
    return 1;
}


int learnsan_unpoison(void* ptr, size_t size) {
    uintptr_t ptr_int = (uintptr_t) ptr;
    uintptr_t ptr_int_end = ptr_int + size;

    while (ptr_int < ptr_int_end) {
        uintptr_t mem = ptr_int;
        uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
        *shadow_addr = 0;
        ptr_int += 8;
    }
    return 1;
}


int learnsan_load1(void* ptr) {
    uintptr_t mem = (uintptr_t)ptr;
    uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
    int8_t content = *shadow_addr;
    //fprintf(stderr, "++++ Read from shadow addr %p (%lu) with content %u\n", shadow_addr, mem, content);
    return (content != 0 && ((int)((mem & 7) + 1) > content));
}



int learnsan_load4(void* ptr) {
    uintptr_t mem = (uintptr_t)ptr;
    uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
    int8_t content = *shadow_addr;
    //fprintf(stderr, "++++ Read from shadow addr %p (%lu) with content %u\n", shadow_addr, mem, content);
    return (content != 0 && ((int)((mem & 7) + 4) > content));
}

int learnsan_load8(void* ptr) {
    uintptr_t mem = (uintptr_t)ptr;
    uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
    return *shadow_addr != 0;
}

int learnsan_store1(void* ptr) {
    uintptr_t mem = (uintptr_t)ptr;
    uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
    int8_t content = *shadow_addr;
    //fprintf(stderr, "++++ Write from shadow addr %p (%lu) with content %u\n", shadow_addr, mem, content);
    int res = (content != 0 && ((int)((mem & 7) + 1) > content));
    return res;
}

int learnsan_store4(void* ptr) {
    uintptr_t mem = (uintptr_t)ptr;
    uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
    int8_t content = *shadow_addr;
    //fprintf(stderr, "++++ Write from shadow addr %p (%lu) with content %u\n", shadow_addr, mem, content);
    int res = (content != 0 && ((int)((mem & 7) + 4) > content));
    return res;
}

int learnsan_store8(void* ptr) {
    uintptr_t mem = (uintptr_t)ptr;
    uint8_t* shadow_addr = (uint8_t*) MEM_TO_SHADOW(mem);
    return *shadow_addr != 0;
}
