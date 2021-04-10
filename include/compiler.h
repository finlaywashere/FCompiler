#include <stdint.h>
#include <stddef.h>

#include <elf.h>

#define PARAMS 4

struct symbol{
	char name[64];
	uint64_t id;
};
typedef struct symbol symbol_t;

struct instruction{
	char name[16];
	uint64_t params[PARAMS];
	uint64_t types[PARAMS];
	uint64_t s_start[PARAMS];
	uint64_t s_len[PARAMS];
	uint64_t line;
	uint64_t count;
	uint64_t address;
	uint64_t origin;
	uint64_t symbols;
};
typedef struct instruction instruction_t;

void make_valid_header(elf_header_t* header);
uint64_t elf_length(uint64_t data_length);
void create_elf(uint8_t* buffer, uint8_t* data, uint64_t data_length, instruction_t* inst);
uint64_t get_instruction_length(instruction_t* inst);
uint64_t get_register_size(char* reg);
uint64_t get_register_num(char* reg);
void write_instruction(instruction_t* inst, uint8_t* buffer, uint64_t address);
void parse_instructions(instruction_t* inst, char* buffer, uint64_t len, uint64_t count);
uint64_t count_instructions(char* buffer, uint64_t len);
void group_symbols(instruction_t* inst, uint64_t len, char* buffer);
void find_symbols_and_replace(instruction_t* inst, uint64_t len);

