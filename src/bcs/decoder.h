#pragma once

#include "buffer.h"

#include "types.h"

bool bcs_read_bool(buffer_t *buffer, bool *value);
bool bcs_read_option_tag(buffer_t *buffer, bool *value);

bool bcs_read_u8(buffer_t *buffer, uint8_t *value);
bool bcs_read_u16(buffer_t *buffer, uint16_t *value);
bool bcs_read_u32(buffer_t *buffer, uint32_t *value);
bool bcs_read_u64(buffer_t *buffer, uint64_t *value);
bool bcs_read_u128(buffer_t *buffer, uint128_t *value);

bool bcs_read_i8(buffer_t *buffer, int8_t *value);
bool bcs_read_i16(buffer_t *buffer, int16_t *value);
bool bcs_read_i32(buffer_t *buffer, int32_t *value);
bool bcs_read_i64(buffer_t *buffer, int64_t *value);
bool bcs_read_i128(buffer_t *buffer, int128_t *value);

bool bcs_read_f32(buffer_t *buffer, float *value);
bool bcs_read_f64(buffer_t *buffer, double *value);

bool bcs_read_u32_from_uleb128(buffer_t *buffer, uint32_t *value);
bool bcs_read_variant_index(buffer_t *buffer, uint32_t *out);

bool bcs_read_char(buffer_t *buffer, uint8_t *out);
bool bcs_read_bytes(buffer_t *buffer, uint8_t *out, size_t out_len);
bool bcs_read_string(buffer_t *buffer, unsigned char *out, size_t out_size, size_t *out_len);
bool bcs_read_fixed_bytes(buffer_t *buffer, uint8_t *out, size_t size);
bool bcs_read_ptr_to_fixed_bytes(buffer_t *buffer, uint8_t **out, size_t size);
bool bcs_read_dynamic_bytes(buffer_t *buffer, uint8_t *out, size_t out_size, size_t *out_len);

bool bcs_read_type_tag_fixed(buffer_t *buffer, type_tag_t *ty_val);

// TODO: optimize memory handling before use
// bool bcs_read_type_tag_vector(buffer_t *buffer, type_tag_t *vector_val);
// TODO: optimize memory handling before use
// bool bcs_read_type_tag_struct(buffer_t *buffer, type_tag_struct_t *ty_struct);
