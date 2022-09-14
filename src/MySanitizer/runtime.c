#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>

#include "allocator.h"
#include "learnsan.h"

#define CALLSTACK_MAX 1000

char* callstack[CALLSTACK_MAX];
unsigned top = 0;

__attribute__((constructor, no_sanitize("address", "memory"))) void init() {
   // Here you can place initialization code 
    __init_all();
}

void push_call(char* fcn_name) {
    size_t s = strlen(fcn_name);
    char* dst = (char*) malloc(s + 1);
    strcpy(dst, fcn_name);
    callstack[top] = dst;
    if (top < CALLSTACK_MAX)
        top++;
}

void pop_call() {
    free(callstack[top]);
    callstack[top] = NULL;
    if (top >= 1)
        top--;
}

void crash_and_report() {
    fprintf(stderr, "learnsanitizer detected an heap memory corruption\n");    
    fprintf(stderr, "callstack depth %u\n", top);
    for (int i = 0; i < top; i++) {
        fprintf(stderr, "%s\n", callstack[i]);
    }
    abort();
}


// Hooking

extern void *__hook_malloc(size_t size) {

    void* p = __learnsan_malloc(size);
    return p;

}

extern void __hook_free(void* ptr) {
    __learnsan_free(ptr);
	return;
}


extern void __hook_store(long addr, unsigned int size) {

    void* ptr = (void*) addr;
    int res = __learnsan_store(ptr, size);
    if (res != 0) {
        crash_and_report();
    }

}

extern void __hook_load(long addr, unsigned int size) {

    void* ptr = (void*)addr;
    int res = __learnsan_load(ptr, size);
    if (res != 0) {
        crash_and_report();
    }
    
}


extern void __hook_entry(char* name) {
    //fprintf(stderr, "==> %s\n", name);
    push_call(name);
}


extern void __hook_exit(char* name) {
    //fprintf(stderr, "<== %s\n", name);
    pop_call();
}

