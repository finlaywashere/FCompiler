#include <stdint.h>
#include <stddef.h>

#include <elf.h>

struct instruction{
	char name[16];
	uint64_t params[4];
	uint64_t types[4];
	uint64_t line;
	uint64_t count;
};
typedef struct instruction instruction_t;

void make_valid_header(elf_header_t* header);
uint64_t elf_length(uint64_t data_length);
void create_elf(uint8_t* buffer, uint8_t* data, uint64_t data_length);
uint64_t get_instruction_length(instruction_t* inst);
uint64_t get_register_size(char* reg);
uint64_t get_register_num(char* reg);
void write_instruction(instruction_t* inst, uint8_t* buffer);
void parse_instructions(instruction_t* inst, char* buffer, uint64_t len);
uint64_t count_instructions(char* buffer, uint64_t len);

