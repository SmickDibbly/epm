#include "./bfio.h"
#include <stdlib.h>
#include <stdbool.h>

char *read_ASCII_string(FILE *in_fp, char *str_p, uint8_t num_chars) {
    if (str_p == NULL) {
        str_p = calloc(num_chars + 1, sizeof(char));
    }

    for (uint32_t i = 0; i < num_chars; i++) {
        str_p[i] = (char)read_ASCII_char(in_fp);
    }

    str_p[num_chars] = '\0';

    return str_p;
}

void write_ASCII_string(char *str_p, FILE *out_fp, uint8_t num_bytes) {
    for (uint32_t i = 0; i < num_bytes; i++) {
        write_byte(str_p[i], out_fp);
    }
}

uint32_t read_VLQ_max4_be(FILE *in_fp) {
    uint32_t value;
    byte_t c;

    if ((value = read_byte(in_fp)) & 0x80) {
        value &= 0x0000007f; // strip high bit
        do {
            value = (value << 7) + ((c = (byte_t)read_byte(in_fp)) & 0x7f);
        } while (c & 0x80);
    }
    return (value);
}

void write_VLQ_max4_be(uint32_t value, FILE *out_fp) {
    uint32_t buffer = value & 0x7f;
    while ((value >>= 7) > 0) {
        buffer <<= 8;
        buffer |= 0x80;
        buffer += (value & 0x7f);
    }
    while (true) {
        fputc(buffer, out_fp);
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }
}

uint32_t read_VLQ_max4_le(FILE *in_fp) {
    (void)in_fp;
    return 0;
}


uint32_t read_uint(FILE *in_fp, uint8_t num_bytes, uint8_t endianness) {
    (void)in_fp;
    (void)num_bytes;
    (void)endianness;
    return 0;
}

uint16_t read_uint_2_be(FILE *in_fp) {
    return (uint16_t)(0
                      | (read_byte(in_fp) << 8)
                      | (read_byte(in_fp)));
}

uint16_t read_uint_2_le(FILE *in_fp) {
    unsigned char b[2];
    uint16_t rec = 0;
    
    size_t ignore = fread((char*) b, 1, 2, in_fp);
    (void)ignore;
    rec |= (uint16_t)(((uint32_t) b[0]) << 0);
    rec |= (uint16_t)(((uint32_t) b[1]) << 8);

    return rec;

    
    /*
    return (uint16_t)(0
                      | (read_byte(in_fp))
                      | (read_byte(in_fp) << 8));
    */
}

uint32_t read_uint_4_be(FILE *in_fp) {
    return
        (uint32_t)0
        | (read_byte(in_fp) << 24)
        | (read_byte(in_fp) << 16)
        | (read_byte(in_fp) << 8)
        | (read_byte(in_fp) << 0);
}

void write_uint_4_be(uint32_t num, FILE *out_fp) {
    write_byte((num >> 24) & 0xFF, out_fp);
    write_byte((num >> 16) & 0xFF, out_fp);
    write_byte((num >> 8) & 0xFF, out_fp);
    write_byte((num >> 0) & 0xFF, out_fp);
}

/*
uint32_t read_uint_4_le_OLD(FILE *in_fp) {
    return
        (uint32_t)0
        | (read_byte(in_fp) << 0)
        | (read_byte(in_fp) << 8)
        | (read_byte(in_fp) << 16)
        | (read_byte(in_fp) << 24);
}
*/

uint32_t read_uint_4_le(FILE *in_fp) {
    unsigned char b[4];
    uint32_t rec = 0;
    
    size_t ignore = fread((char*) b, 1, 4, in_fp);
    (void)ignore;
    rec |= (uint32_t)((uint32_t) b[0]) << 0;
    rec |= (uint32_t)((uint32_t) b[1]) << 8;
    rec |= (uint32_t)((uint32_t) b[2]) << 16;
    rec |= (uint32_t)((uint32_t) b[3]) << 24;

    return rec;
}

void write_uint_4_le(uint32_t num, FILE *out_fp) {
    write_byte((num >> 0) & 0xFF, out_fp);
    write_byte((num >> 8) & 0xFF, out_fp);
    write_byte((num >> 16) & 0xFF, out_fp);
    write_byte((num >> 24) & 0xFF, out_fp);
}
