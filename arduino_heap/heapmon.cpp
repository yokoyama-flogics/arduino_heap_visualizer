/*
 * Copyright (c) 2023 Atsushi Yokoyama, Firmlogics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PLEASE NOTICE that this design depends on malloc() and free()
 * implementation.  Referred implementation is of avr-libc-1.8.1.
 * Please also refer https://www.nongnu.org/avr-libc/ .
 */

#include "heapmon.h"

static bool estmated_peak_SP_available = false;
static char* max_brkval = nullptr;

static void* estimated_peak_SP(void) {
  if (!estmated_peak_SP_available)
    return 0;

  /*
   * At the moment, max_brkval must be set referring __brkval.
   */
  char* bv = max_brkval;
  if (bv == nullptr)
    bv = &__heap_start;

  for (char* pt = bv; pt <= (char*)SP; pt++) {
    if (*pt != BYTE_FILL_STACK)
      return pt - 1;
  }

  return (char*)-1;
}

void heapmon_fill_stack(void) {
  noInterrupts();
  char* bv = __brkval;
  if (bv == nullptr)
    bv = &__heap_start;

  for (char* pt = bv; pt <= (char*)SP; pt++) {
    *pt = BYTE_FILL_STACK;
  }
  interrupts();

  estmated_peak_SP_available = true;
}

void heapmon_print_consts(void) {
  Serial.println();  // prevent any garbage in front of the header text
  Serial.print((__FlashStringHelper*)HEAPMON_HEADER);
  Serial.print(F("data_start: "));
  Serial.print((int)&__data_start, HEX);
  Serial.print(F(" bss_start: "));
  Serial.print((int)&__bss_start, HEX);
  Serial.print(F(" heap_start: "));
  Serial.print((int)&__heap_start, HEX);
  Serial.print(F(" heap_end: "));
  Serial.println((int)&__heap_end, HEX);
}

void heapmon_check_brkval(void) {
  if (__brkval > max_brkval)
    max_brkval = __brkval;
}

void heapmon_print_stats(void) {
  heapmon_check_brkval();

  Serial.print((__FlashStringHelper*)HEAPMON_HEADER);
  Serial.print(F("SP: "));
  Serial.print((int)SP, HEX);
  Serial.print(F(" estpeakSP: "));
  Serial.print((int)estimated_peak_SP(), HEX);
  Serial.print(F(" brkval: "));
  Serial.print((int)__brkval, HEX);
  Serial.print(F(" flp: "));
  Serial.print((int)__flp, HEX);

  struct __freelist* pt = __flp;
  while (pt) {
    Serial.print(F(" "));
    Serial.print((int)pt->sz, HEX);
    Serial.print(F(" "));
    Serial.print((int)pt->nx, HEX);
    pt = pt->nx;
  }
  Serial.println();
}

#ifdef HEAPMON_DEBUG
void heapmon_dump(void* start, int len) {
  char s[10];

  Serial.print(F("     "));
  for (int i = 0; i < 16; i++) {
    snprintf_P(s, sizeof(s), PSTR(" +%x"), i);
    Serial.print(s);
  }
  Serial.println();

  unsigned char* addr = (unsigned int)start & 0xfff0;
  len += (int)start - (int)addr;

  for (; len > 0; addr++, len--) {
    if (((int)addr & 0xf) == 0) {
      snprintf_P(s, sizeof(s), PSTR("%04x: "), addr);
      Serial.print(s);
    }

    if (addr < start)
      Serial.print(F("   "));
    else {
      snprintf_P(s, sizeof(s), PSTR("%02x "), *addr);
      Serial.print(s);
    }
    if (((int)addr & 0xf) == 0xf)
      Serial.println();
  }

  if (((int)addr & 0xf) != 0)
    Serial.println();
}
#endif
