#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "decoder.h"
#include "utf8.h"

bool bcs_read_bool(buffer_t *buffer, bool *value) {
    uint8_t tmp_value = 0;
    if (!buffer_read_u8(buffer, &tmp_value)) {
        return false;
    }
    if (tmp_value > 1) {
        // Invalid boolean value
        return false;
    }
    *value = tmp_value;
    return true;
}

bool bcs_read_option_tag(buffer_t *buffer, bool *value) {
    return bcs_read_bool(buffer, value);
}

bool bcs_read_u8(buffer_t *buffer, uint8_t *value) {
    return buffer_read_u8(buffer, value);
}

bool bcs_read_u16(buffer_t *buffer, uint16_t *value) {
    return buffer_read_u16(buffer, value, LE);
}

bool bcs_read_u32(buffer_t *buffer, uint32_t *value) {
    return buffer_read_u32(buffer, value, LE);
}

bool bcs_read_u64(buffer_t *buffer, uint64_t *value) {
    return buffer_read_u64(buffer, value, LE);
}

bool bcs_read_u128(buffer_t *buffer, uint128_t *value) {
    if (!buffer_read_u64(buffer, &value->low, LE)) {
        return false;
    }
    return buffer_read_u64(buffer, &value->high, LE);
}

bool bcs_read_i8(buffer_t *buffer, int8_t *value) {
    uint8_t tmp = 0;
    if (!buffer_read_u8(buffer, &tmp)) {
        return false;
    }
    *value = (int8_t) tmp;
    return true;
}

bool bcs_read_i16(buffer_t *buffer, int16_t *value) {
    uint16_t tmp = 0;
    if (!buffer_read_u16(buffer, &tmp, LE)) {
        return false;
    }
    *value = (int16_t) tmp;
    return true;
}

bool bcs_read_i32(buffer_t *buffer, int32_t *value) {
    uint32_t tmp = 0;
    if (!buffer_read_u32(buffer, &tmp, LE)) {
        return false;
    }
    *value = (int32_t) tmp;
    return true;
}

bool bcs_read_i64(buffer_t *buffer, int64_t *value) {
    uint64_t tmp = 0;
    if (!buffer_read_u64(buffer, &tmp, LE)) {
        return false;
    }
    *value = (int64_t) tmp;
    return true;
}

bool bcs_read_i128(buffer_t *buffer, int128_t *value) {
    if (!buffer_read_u64(buffer, &value->low, LE)) {
        return false;
    }
    return bcs_read_i64(buffer, &value->high);
}

bool bcs_read_f32(buffer_t *buffer, float *value) {
    (void) buffer;
    (void) value;
    // Not implemented
    return false;
}

bool bcs_read_f64(buffer_t *buffer, double *value) {
    (void) buffer;
    (void) value;
    // Not implemented
    return false;
}

bool bcs_read_u32_from_uleb128(buffer_t *buffer, uint32_t *value) {
    uint64_t tmp_value = 0;
    uint8_t tmp_byte = 0;
    int digit;
    for (int shift = 0; shift < 32; shift += 7) {
        if (!buffer_read_u8(buffer, &tmp_byte)) {
            return false;
        }
        digit = tmp_byte & 0x7F;
        tmp_value |= (uint64_t) digit << shift;
        if (tmp_value > UINT32_MAX) {
            // Overflow while parsing uleb128-encoded uint32 value
            return false;
        }
        if (digit == tmp_byte) {
            if (shift > 0 && digit == 0) {
                // Invalid uleb128 number (unexpected zero digit)
                return false;
            }
            *value = (uint32_t) tmp_value;
            return true;
        }
    }
    // Overflow while parsing uleb128-encoded uint32 value
    return false;
}

bool bcs_read_variant_index(buffer_t *buffer, uint32_t *out) {
    return bcs_read_u32_from_uleb128(buffer, out);
}

bool bcs_read_length(buffer_t *buffer, size_t *out_len) {
    uint32_t tmp_length = 0;
    if (!bcs_read_u32_from_uleb128(buffer, &tmp_length)) {
        return false;
    }
    *out_len = (size_t) tmp_length;

    if (tmp_length > MAX_SEQUENCE_LENGTH) {
        // Length is too large
        return false;
    }
    return true;
}

bool bcs_read_char(buffer_t *buffer, uint8_t *out) {
    (void) buffer;
    (void) out;
    // Not implemented
    return false;
}

bool bcs_read_string(buffer_t *buffer, unsigned char *out, size_t out_size, size_t *out_len) {
    size_t str_len = 0;
    if (!bcs_read_length(buffer, &str_len)) {
        return false;
    }
    if (!buffer_can_read(buffer, str_len)) {
        return false;
    }

    const uint8_t *str_start = buffer->ptr + buffer->offset;
    int ascii_len = try_utf8_to_ascii(str_start, str_len, out, out_size, NULL);
    if (ascii_len < 0) {
        return false;
    }
    *out_len = ascii_len;

    return buffer_seek_cur(buffer, str_len);
}

bool bcs_read_fixed_bytes(buffer_t *buffer, uint8_t *out, size_t size) {
    if (!buffer_can_read(buffer, size)) {
        return false;
    }

    memmove(out, buffer->ptr + buffer->offset, size);
    return buffer_seek_cur(buffer, size);
}

bool bcs_read_ptr_to_fixed_bytes(buffer_t *buffer, uint8_t **out, size_t size) {
    if (!buffer_can_read(buffer, size)) {
        return false;
    }

    *out = (uint8_t *) buffer->ptr + buffer->offset;
    return buffer_seek_cur(buffer, size);
}

bool bcs_read_dynamic_bytes(buffer_t *buffer, uint8_t *out, size_t out_size, size_t *out_len) {
    *out_len = 0;
    if (!bcs_read_length(buffer, out_len)) {
        return false;
    }
    if (*out_len > out_size) {
        return false;
    }
    return bcs_read_fixed_bytes(buffer, out, *out_len);
}

bool bcs_read_type_tag_fixed(buffer_t *buffer, type_tag_t *ty_val) {
    switch (ty_val->type_tag) {
        case TYPE_TAG_BOOL:
            ty_val->size = sizeof(bool);
            return bcs_read_bool(buffer, (bool *) &ty_val->value);
        case TYPE_TAG_U8:
            ty_val->size = sizeof(uint8_t);
            return bcs_read_u8(buffer, (uint8_t *) &ty_val->value);
        case TYPE_TAG_U64:
            ty_val->size = sizeof(uint64_t);
            return bcs_read_u64(buffer, (uint64_t *) &ty_val->value);
        case TYPE_TAG_U128:
            ty_val->size = sizeof(uint128_t);
            return bcs_read_u128(buffer, (uint128_t *) &ty_val->value);
        case TYPE_TAG_ADDRESS:
        case TYPE_TAG_SIGNER:
            ty_val->size = ADDRESS_LEN;
            return bcs_read_fixed_bytes(buffer, (uint8_t *) &ty_val->value, ADDRESS_LEN);
        default:
            return false;
    }
}

/* TODO: optimize memory handling before use
bool bcs_read_type_tag_vector(buffer_t *buffer, type_tag_t *vector_val) {
    if (!bcs_read_u32_from_uleb128(buffer, (uint32_t *) vector_val->size)) {
        return false;
    }
    vector_val->value = (type_tag_t *) malloc(sizeof(type_tag_t) * vector_val->size);
    for (size_t i = 0; i < vector_val->size; i++) {
        if (!bcs_read_u32_from_uleb128(buffer, (uint32_t *) (*(type_tag_t *) vector_val->value).type_tag)) {
            return false;
        }
        if ((*(type_tag_t *) vector_val->value).type_tag < TYPE_TAG_VECTOR) {
            if (!bcs_read_type_tag_fixed(buffer, &(*(type_tag_t *) vector_val->value))) {
                return false;
            }
        } else if ((*(type_tag_t *) vector_val->value).type_tag == TYPE_TAG_VECTOR) {
            if (!bcs_read_type_tag_vector(buffer, &(*(type_tag_t *) vector_val->value))) {
                return false;
            }
        } else if ((*(type_tag_t *) vector_val->value).type_tag == TYPE_TAG_STRUCT) {
            (*(type_tag_t *) vector_val->value).size = sizeof(type_tag_struct_t);
            (*(type_tag_t *) vector_val->value).value = (type_tag_struct_t *) malloc(sizeof(type_tag_struct_t));
            type_tag_struct_init((*(type_tag_t *) vector_val->value).value);

            if (!bcs_read_type_tag_struct(buffer, (*(type_tag_t *) vector_val->value).value)) {
                return false;
            }
        }
    }
    return true;
}
*/

/* TODO: optimize memory handling before use
bool bcs_read_type_tag_struct(buffer_t *buffer, type_tag_struct_t *ty_struct) {
    if (!bcs_read_fixed_bytes(buffer, (uint8_t *) &ty_struct->address, ADDRESS_LEN)) {
        return false;
    }
    if (!bcs_read_u32_from_uleb128(buffer, (uint32_t *) &ty_struct->module_name.len)) {
        return false;
    }
    ty_struct->module_name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * ty_struct->module_name.len);
    if (!bcs_read_fixed_bytes(buffer, (uint8_t *) &ty_struct->module_name.bytes, ty_struct->module_name.len)) {
        return false;
    }
    if (!bcs_read_u32_from_uleb128(buffer, (uint32_t *) &ty_struct->name.len)) {
        return false;
    }
    ty_struct->name.bytes = (uint8_t *) malloc(sizeof(uint8_t) * ty_struct->name.len);
    if (!bcs_read_fixed_bytes(buffer, (uint8_t *) &ty_struct->name.bytes, ty_struct->name.len)) {
        return false;
    }
    if (!bcs_read_u32_from_uleb128(buffer, (uint32_t *) &ty_struct->type_args_size)) {
        return false;
    }
    if (ty_struct->type_args_size > 0) {
        ty_struct->type_args = (type_tag_t *) malloc(sizeof(type_tag_t) * ty_struct->type_args_size);
        for (size_t i = 0; i < ty_struct->type_args_size; i++) {
            ty_struct->type_args[i].type_tag = 0;
            ty_struct->type_args[i].size = 0;
            ty_struct->type_args[i].value = NULL;

            if (!bcs_read_u32_from_uleb128(buffer, (uint32_t *) &ty_struct->type_args[i].type_tag)) {
                return false;
            }
            if (ty_struct->type_args[i].type_tag < TYPE_TAG_VECTOR) {
                if (!bcs_read_type_tag_fixed(buffer, (type_tag_t *) &ty_struct->type_args[i].value)) {
                    return false;
                }
            } else if (ty_struct->type_args[i].type_tag == TYPE_TAG_VECTOR) {
                if (!bcs_read_type_tag_vector(buffer, &ty_struct->type_args[i])) {
                    return false;
                }
            } else if (ty_struct->type_args[i].type_tag == TYPE_TAG_STRUCT) {
                ty_struct->type_args[i].size = sizeof(type_tag_struct_t);
                ty_struct->type_args[i].value = (type_tag_struct_t *) malloc(sizeof(type_tag_struct_t));
                type_tag_struct_init(ty_struct->type_args[i].value);

                if (!bcs_read_type_tag_struct(buffer, ty_struct->type_args[i].value)) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    return true;
}
*/
