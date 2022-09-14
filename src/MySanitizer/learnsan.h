#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>

#define HIGH_SHADOW_ADDR ((void*)0x02008fff7000ULL)
#define LOW_SHADOW_ADDR ((void*)0x00007fff8000ULL)
#define GAP_SHADOW_ADDR ((void*)0x00008fff7000)

#define HIGH_SHADOW_SIZE (0xdfff0000fffULL)
#define LOW_SHADOW_SIZE (0xfffefffULL)
#define GAP_SHADOW_SIZE (0x1ffffffffff)

#define SHADOW_OFFSET (0x7fff8000ULL)
#define SHADOW_SCALE 3



/* shadow map byte values */
#define ASAN_VALID 0x00
#define ASAN_PARTIAL1 0x01
#define ASAN_PARTIAL2 0x02
#define ASAN_PARTIAL3 0x03
#define ASAN_PARTIAL4 0x04
#define ASAN_PARTIAL5 0x05
#define ASAN_PARTIAL6 0x06
#define ASAN_PARTIAL7 0x07
#define ASAN_ARRAY_COOKIE 0xac
#define ASAN_STACK_RZ 0xf0
#define ASAN_STACK_LEFT_RZ 0xf1
#define ASAN_STACK_MID_RZ 0xf2
#define ASAN_STACK_RIGHT_RZ 0xf3
#define ASAN_STACK_FREED 0xf5
#define ASAN_STACK_OOSCOPE 0xf8
#define ASAN_GLOBAL_RZ 0xf9
#define ASAN_HEAP_RZ 0xe9
#define ASAN_USER 0xf7
#define ASAN_HEAP_LEFT_RZ 0xfa
#define ASAN_HEAP_RIGHT_RZ 0xfb
#define ASAN_HEAP_FREED 0xfd



void learnsan_init();

int learnsan_poison(void* ptr, size_t s, uint8_t poison_byte);
int learnsan_unpoison(void* ptr, size_t s);

int learnsan_load1(void* ptr);
int learnsan_load4(void* ptr);
int learnsan_load8(void* ptr);

int learnsan_store1(void* ptr);
int learnsan_store4(void* ptr);
int learnsan_store8(void* ptr);
