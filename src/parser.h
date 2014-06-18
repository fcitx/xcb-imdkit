#ifndef IMDKIT_PARSE_H
#define IMDKIT_PARSE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t *bytearray;
#define READ_FUNC(TYPE) \
void TYPE##_read(TYPE* p, uint8_t** data, size_t *len, bool swap); \
uint8_t* TYPE##_write(TYPE* p, uint8_t* data, bool swap);

READ_FUNC(uint8_t)
READ_FUNC(uint16_t)
READ_FUNC(uint32_t)
void bytearray_read(bytearray* p, uint32_t arraylen, uint8_t** data, size_t *len, bool swap);
uint8_t* bytearray_write(bytearray* p, uint32_t arraylen, uint8_t* data, bool swap);

uintptr_t align_to_4(uintptr_t ptr, uintptr_t len, size_t* remain);
uintptr_t align_to_2(uintptr_t ptr, uintptr_t len, size_t* remain);

#endif // IMDKIT_PARSE_H
