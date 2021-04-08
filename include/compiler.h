#include <stdint.h>
#include <stddef.h>

#include <elf.h>

void make_valid_header(elf_header_t* header);
uint64_t elf_length(uint64_t data_length);
void create_elf(uint8_t* buffer, uint8_t* data, uint64_t data_length);
uint64_t get_instruction_length(char* str);
uint64_t get_register_size(char* reg);
uint64_t get_register_num(char* reg);
void parse_and_write_instruction(char* str, uint8_t* buffer);

