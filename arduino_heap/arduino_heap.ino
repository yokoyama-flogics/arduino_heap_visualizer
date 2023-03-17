/*
 * Heap and Stack usage analysis example
 * Copyright (c) 2023 Atsushi Yokoyama, Firmlogics
 */

#include "heapmon.h"

static void test1(void) {
  Serial.println("malloc'ing pt(8), pt2(8)");
  void* pt = malloc(8);
  void* pt2 = malloc(8);
  Serial.println((int)pt, HEX);
  Serial.println((int)pt2, HEX);
  heapmon_print_stats();

  Serial.println("freeing pt");
  free(pt);
  heapmon_print_stats();

  Serial.println("String s(3)");
  String s = "aaa";
  Serial.println(s);
  heapmon_print_stats();

  Serial.println("Appending to s");
  s += "bbb";
  Serial.println(s);
  heapmon_print_stats();

  Serial.println("malloc'ing pt3(16), pt(4)");
  void* pt3 = malloc(16);
  pt = malloc(4);
  Serial.println((int)pt3, HEX);
  Serial.println((int)pt, HEX);
  heapmon_print_stats();

  Serial.println("freeing pt2");
  free(pt2);
  heapmon_print_stats();

  Serial.println("freeing pt");
  free(pt);
  heapmon_print_stats();
}

static void test2(void) {
  const int SIZE = 32;
  const int N = 16;
  void* pt_alloc[N];

  for (int i = 0; i < N; i++) {
    pt_alloc[i] = malloc(SIZE);
    Serial.print(i);
    Serial.print(" ");
    Serial.println((int)pt_alloc[i], HEX);
    heapmon_print_stats();
  }

  for (int i = N - 1; i >= 0; i--) {
    free(pt_alloc[i]);
    heapmon_print_stats();
  }

  heapmon_dump(0x1c0, 0x400);
  heapmon_dump(0x800, 0x100);
}

void setup() {
  /*
   * This is optional, but should call this as soon as possible in setup() if
   * want to estimate the peak-SP, or maximum stack usage.
   */
  heapmon_fill_stack();

  Serial.begin(115200);

  /*
   * Should once call this prior to heapmon_print_stats().
   */
  heapmon_print_consts();

  /*
   * This is available if HEAPMON_DEBUG is defined in heapmon.h.
   */
  heapmon_dump(0x800, 0x100);

  /*
   * You can call this anytime to check memory usage (heap and stack).
   */
  heapmon_print_stats();

  test1();
  // test2();
}

void loop() {
}
