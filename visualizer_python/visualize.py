# Copyright (c) 2023 Atsushi Yokoyama, Firmlogics
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import os
import sys

import serial
from termcolor import colored

PORT = "/dev/cu.usbserial-DA0147I3"
SERIAL_SPEED = 115200
HEAPMON_HEADER = "#># HEAPMON"
COLOR_NORMAL = (None, None, [])
FORCE_COLOR = True
DEBUG = False

MEM_SIZE = 2048
DUMP_WIDTH = 64

data_start = None
bss_start = None
heap_start = None
heap_end = None


def match_head(s, head):
    """
    Test if the string 's' begins with the string 'head'.
    """
    return s[: len(head)] == head


def pos_char(s, pos, fill="."):
    """
    Return a character at 'pos' in 's'.
    If out of range, return " ".
    """
    if pos >= len(s):
        return fill
    else:
        return s[pos]


def visualize(sp, estpeak_sp, brkval, flp):
    MEM_OFFSET = 0x100

    mem = [(" ", COLOR_NORMAL)] * MEM_SIZE

    for pos, addr in enumerate(range(data_start, bss_start)):
        c = pos_char("DATA", pos)
        mem[addr - MEM_OFFSET] = (c, (None, "on_dark_grey", ["bold"]))

    for pos, addr in enumerate(range(bss_start, heap_start)):
        c = pos_char("BSS", pos)
        mem[addr - MEM_OFFSET] = (c, (None, "on_blue", ["bold"]))

    if brkval == 0:
        brkval = heap_start

    for pos, addr in enumerate(range(heap_start, brkval)):
        c = "."
        mem[addr - MEM_OFFSET] = (c, (None, "on_yellow", ["bold"]))

    for f in flp:
        f_len = f[1] + 2  # 2 bytes for sz and 2 bytes for nx also
        for pos, addr in enumerate(range(f[0], f[0] + f_len)):
            c = " "
            mem[addr - MEM_OFFSET] = (c, (None, "on_yellow", ["bold"]))

    if estpeak_sp:
        for pos, addr in enumerate(range(estpeak_sp, sp)):
            c = " "
            mem[addr - MEM_OFFSET] = (c, (None, "on_cyan", ["bold"]))

    for pos, addr in enumerate(range(sp, MEM_SIZE + MEM_OFFSET)):
        c = pos_char("STACK", pos)
        mem[addr - MEM_OFFSET] = (c, (None, "on_cyan", ["bold"]))

    try:
        for addr in range(0, MEM_SIZE):
            if addr % DUMP_WIDTH == 0:
                sys.stdout.write("{:04x}: ".format(addr + MEM_OFFSET))

            c = mem[addr]
            sys.stdout.write(colored(c[0], c[1][0], c[1][1], attrs=c[1][2]))
            if addr % DUMP_WIDTH == DUMP_WIDTH - 1:
                sys.stdout.write("\n")

        sys.stdout.write(colored(".", None, "on_dark_grey", ["bold"]))
        sys.stdout.write(": data, ")
        sys.stdout.write(colored(".", None, "on_blue", ["bold"]))
        sys.stdout.write(": BSS, ")
        sys.stdout.write(colored(".", None, "on_yellow", ["bold"]))
        sys.stdout.write(": heap, ")
        sys.stdout.write(colored(" ", None, "on_yellow", ["bold"]))
        sys.stdout.write(": heap (free), ")
        sys.stdout.write(colored(" ", None, "on_cyan", ["bold"]))
        sys.stdout.write(": stack (used so far), ")
        sys.stdout.write(colored(".", None, "on_cyan", ["bold"]))
        sys.stdout.write(": stack")
        sys.stdout.write("\n")

    except BrokenPipeError:
        sys.exit(0)


def dec_hex(s):
    val = int(s, 16)
    return val


def expect_and_dec(w, name):
    """
    Find expected keyword 'name' and return the associated value.
    """
    if not match_head(w[0], name):
        raise Exception("Expected '{}'.".format(name))
    val = dec_hex(w[1])
    if DEBUG:
        print("{}: {}".format(name, hex(val)))
    return val


def decode_consts(w):
    """
    Decode a line like:
    ['data_start:', '100', 'bss_start:', '188', 'heap_start:', '235',
        'heap_end:', '0']
    """
    global data_start, bss_start, heap_start, heap_end

    data_start = expect_and_dec(w[0:], "data_start")
    bss_start = expect_and_dec(w[2:], "bss_start")
    heap_start = expect_and_dec(w[4:], "heap_start")
    heap_end = expect_and_dec(w[6:], "heap_end")


def decode_sp_and_heap(w):
    """
    Decode a line like:
    ['SP:', '8ED', 'estpeakSP:', '89D', 'brkval:', '252',
        'flp:', '235', '8', '0']
    """
    sp = expect_and_dec(w[0:], "SP")

    estpeak_sp = expect_and_dec(w[2:], "estpeakSP")
    if estpeak_sp > 0xFFFF:
        estpeak_sp = None  # couldn't find estimated peak SP

    brkval = expect_and_dec(w[4:], "brkval")

    pt = expect_and_dec(w[6:], "flp")
    flp = []
    ct = 8
    while pt != 0:
        len = dec_hex(w[ct])
        flp.append((pt, len))
        pt = dec_hex(w[ct + 1])
        ct += 2
    if DEBUG:
        print(flp)

    visualize(sp, estpeak_sp, brkval, flp)


def decode_heapmon(s):
    """
    Decode a message from heapmon.cpp.
    """
    words = s.split()
    if DEBUG:
        print(words)
    if match_head(words[0], "data_start"):
        decode_consts(words)
    elif match_head(words[0], "SP:"):
        decode_sp_and_heap(words)
    else:
        raise Exception("Unknown format '{}'.".format(s.strip()))


def main():
    if FORCE_COLOR:
        os.environ["FORCE_COLOR"] = "1"

    lenh = len(HEAPMON_HEADER)
    ser = serial.Serial(PORT, SERIAL_SPEED)

    while True:
        try:
            line = ser.readline().decode("utf-8")
        except KeyboardInterrupt:
            print("Interrupted.")
            sys.exit(0)

        if line[:lenh] == HEAPMON_HEADER:
            decode_heapmon(line[lenh:])
        else:
            try:
                print(line.rstrip())
            except BrokenPipeError:
                sys.exit(0)


if __name__ == "__main__":
    main()
