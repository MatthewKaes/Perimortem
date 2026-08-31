// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/hash.h"

#include <string.h>

enum {
  block_stride = 2,
};

static const uint64_t constants[5] = {
  UINT64_C(0x15459B19A5ED5DBB), UINT64_C(0xAAA360CB31A60D5D),
  UINT64_C(0x003BDC7DF5E62165), UINT64_C(0x7FCC734CAA4A91DD),
  UINT64_C(0x1FC221792E40C189),
};

static uint64_t avalanche(uint64_t value) {
  value ^= value >> 27;
  value *= constants[2];
  value ^= value >> 33;
  value *= constants[4];
  value ^= value >> 33;
  return value;
}

uint64_t perimortem_hash_u64(uint64_t value) {
  uint64_t result = value * constants[0];
  result ^= (result >> 33) * constants[4];
  return result;
}

uint64_t perimortem_hash_combine(uint64_t seed, uint64_t value) {
  return avalanche(value * constants[0]) ^ seed;
}

uint64_t perimortem_hash_bytes(struct perimortem_bytes bytes) {
  const uint8_t* data = bytes.data;
  uint64_t small = 0;
  switch (bytes.size) {
  case 7:
    small |= (uint64_t)data[6] << 48;
  case 6:
    small |= (uint64_t)data[5] << 40;
  case 5:
    small |= (uint64_t)data[4] << 32;
  case 4:
    small |= (uint64_t)data[3] << 24;
  case 3:
    small |= (uint64_t)data[2] << 16;
  case 2:
    small |= (uint64_t)data[1] << 8;
  case 1:
    small |= (uint64_t)data[0];
    small = constants[0] ^ (small + constants[3]);
    small *= constants[2];
    return avalanche(small ^ (bytes.size * constants[0]));

  case 0:
    return constants[4];
  }

  uint64_t result[block_stride];
  const perimortem_count block_count = bytes.size / sizeof(uint64_t);
  for (perimortem_count index = 0; index < block_stride; ++index) {
    result[index] = constants[index];
  }

  for (perimortem_count processed = 0; processed + block_stride <= block_count;
       processed += block_stride) {
    uint64_t block[block_stride];
    memcpy(block, data, sizeof(block));
    data += sizeof(block);
    for (perimortem_count index = 0; index < block_stride; ++index) {
      result[index] ^= block[index] + constants[index + 3];
      result[index] *= constants[index + 1];
    }
  }

  const perimortem_count remainder =
      bytes.size & (sizeof(uint64_t) * block_stride - 1);
  switch (remainder) {
  case 15:
  case 14:
  case 13:
  case 12:
  case 11:
  case 10:
  case 9: {
    uint64_t tail;
    memcpy(&tail, data + remainder - sizeof(uint64_t), sizeof(tail));
    tail >>= (sizeof(uint64_t) * 2 - remainder) * 8;
    result[1] ^= tail + constants[1];
    result[1] *= constants[2];
    __attribute__((fallthrough));
  }

  case 8: {
    uint64_t tail;
    memcpy(&tail, data, sizeof(tail));
    result[0] ^= tail + constants[3];
    result[0] *= constants[2];
    break;
  }

  case 7:
  case 6:
  case 5:
  case 4:
  case 3:
  case 2:
  case 1: {
    uint64_t tail;
    memcpy(&tail, data + remainder - sizeof(uint64_t), sizeof(tail));
    tail >>= (sizeof(uint64_t) - remainder) * 8;
    result[0] ^= tail + constants[3];
    result[0] *= constants[2];
    break;
  }

  case 0:
    break;
  }

  uint64_t final = result[0];
  for (perimortem_count index = 0; index < block_stride; ++index) {
    final ^= avalanche(result[index] ^ (bytes.size * constants[0]));
  }

  return final;
}
