#ifndef __HEAPMON_H__

#define HEAPMON_DEBUG

#include <Arduino.h>
#include <stddef.h>
#include <avr/common.h>

struct __freelist {
  size_t sz;
  struct __freelist* nx;
};

extern char __data_start;
extern char __bss_start;
extern char __heap_start;
extern char __heap_end;
extern char* __brkval;
extern struct __freelist* __flp;

static const char BYTE_FILL_STACK = 0x55;
static const char HEAPMON_HEADER[] PROGMEM = "#># HEAPMON ";

void heapmon_fill_stack(void);
void heapmon_print_consts(void);
void heapmon_check_brkval(void);
void heapmon_print_stats(void);
void heapmon_dump(void* start, int len);

#endif /* !defined(__HEAPMON_H__) */
