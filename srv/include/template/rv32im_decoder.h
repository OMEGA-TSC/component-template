#ifndef RV32IM_DECODE_H
#define RV32IM_DECODE_H

#include <stdint.h>
#include <stddef.h>

int rv32im_decode(uint32_t instr, char* out, size_t out_len);

#endif