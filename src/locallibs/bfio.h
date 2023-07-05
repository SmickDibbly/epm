#ifndef BFIO_H
#define BFIO_H

#include <stdint.h>
#include <stdio.h>

typedef uint8_t byte_t;

#define read_byte(IN_FP) (fgetc((IN_FP)))

#define read_ASCII_char(IN_FP) (fgetc((IN_FP)))
extern char *read_ASCII_string(FILE *in_fp, char *str_p, uint8_t num_bytes);
extern uint32_t read_VLQ_max4_be(FILE *in_fp);
extern uint32_t read_VLQ_max4_le(FILE *in_fp);
extern uint32_t read_uint(FILE *in_fp, uint8_t num_bytes, uint8_t endianness);
extern uint16_t read_uint_2_be(FILE *in_fp);
extern uint16_t read_uint_2_le(FILE *in_fp);
extern uint32_t read_uint_4_be(FILE *in_fp);
extern uint32_t read_uint_4_le(FILE *in_fp);


#define write_byte(BYTE, IN_FP) (fputc((BYTE), (IN_FP)))

extern void write_ASCII_string(char *str_p, FILE *out_fp, uint8_t num_bytes);
extern void write_VLQ_max4_be(uint32_t value, FILE *out_fp);
extern void write_VLQ_max4_le(uint32_t value, FILE *out_fp);
extern void write_uint(uint32_t value, FILE *out_fp, uint8_t num_bytes, uint8_t endianness);
extern void write_uint_2_be(uint32_t value, FILE *out_fp);
extern void write_uint_2_le(uint32_t value, FILE *out_fp);
extern void write_uint_4_be(uint32_t value, FILE *out_fp);
extern void write_uint_4_le(uint32_t value, FILE *out_fp);

#endif
