#include <compiler.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SEGMENTS 3
#define PROGRAMS 1

const char* names = "\0.text\0.shstrtab\0";

void make_valid_header(elf_header_t* header){
	//NOTE: This doesn't set any fields that should be filled with zeros, and doesn't set any non static values
	
	header->magic[0] = 0x7F;
	header->magic[1] = 'E';
	header->magic[2] = 'L';
	header->magic[3] = 'F';
	header->bits = 2; // 64 bit
	header->endian = 1; // Little endian
	header->elf_version = 1; // ELF version must be 1
	header->abi = ABI_LINUX;
	header->abi_version = 0x1;
	header->header_size = 0x40;

	header->type = PROGRAM_TYPE_EXEC;
	header->instruction_set = INSTRUCTION_X86_64;
	header->elf_version_2 = 1; // This ELF version must also be 1

	header->program_header_size = 0x38;
	header->segment_header_size = 0x40;
}
uint64_t elf_length(uint64_t data_length){
	uint64_t len = 0;
	for(uint64_t i = 0; i < SEGMENTS; i++){
		len += strlen((char*) (((uint64_t)names)+len))+1;
	}
	uint64_t length = 0x40 + 0x40 * SEGMENTS + 0x38 * PROGRAMS + data_length + len;
	return length;
}
void create_elf(uint8_t* buffer, uint8_t* data, uint64_t data_length){
	elf_header_t *header = (elf_header_t*) malloc(sizeof(elf_header_t));
	make_valid_header(header);
	
	uint64_t file_offset = elf_length(data_length)-data_length;
	uint64_t str_len = file_offset - (0x40 + 0x40 * SEGMENTS + 0x38 * PROGRAMS);

	header->program_header = 0x40; // Right after ELF header
	header->num_program_headers = PROGRAMS;
	header->segment_header = 0x40 + 0x38 * PROGRAMS; // Right after program headers
	header->num_segment_headers = SEGMENTS;
	header->entry_point = file_offset;
	header->segment_header_name_index = SEGMENTS-1;

	program_header_t* programs = (program_header_t*) malloc(sizeof(program_header_t)*PROGRAMS);
	memset(programs,0,sizeof(program_header_t)*PROGRAMS);
	segment_header_t* segments = (segment_header_t*) malloc(sizeof(segment_header_t)*SEGMENTS);
	memset(segments,0,sizeof(segment_header_t)*SEGMENTS);
	uint64_t name_index = 0;
	for(uint64_t i = 0; i < SEGMENTS; i++){
		segments[i].name_offset = name_index;
		name_index += strlen((char*) (((uint64_t) names) + name_index)) + 1;
	}

	programs[0].type = PROGRAM_LOAD;
	programs[0].offset = file_offset;
	programs[0].virtual_address = file_offset;
	programs[0].file_size = data_length;
	programs[0].memory_size = data_length;
	programs[0].flags = 0x5;
	programs[0].align = 0x1000;
	
	segments[1].type = SEGMENT_PROGRAM;
	segments[1].flags = SEGMENT_FLAG_EXEC | SEGMENT_FLAG_ALLOC;
	segments[1].mem_address = file_offset;
	segments[1].file_offset = file_offset;
	segments[1].file_size = data_length;

	segments[2].type = SEGMENT_STRINGS;
	segments[2].flags = SEGMENT_FLAG_STRINGS;
	segments[2].file_offset = file_offset-str_len;
	segments[2].file_size = str_len;
	
	memcpy(buffer,header,sizeof(elf_header_t));
	memcpy((uint8_t*) &buffer[0x40],programs,sizeof(program_header_t)*PROGRAMS);
	memcpy((uint8_t*) &buffer[0x40+0x38*PROGRAMS],segments,sizeof(segment_header_t)*SEGMENTS);
	memcpy((uint8_t*) &buffer[file_offset-str_len],names,str_len);
	memcpy((uint8_t*) &buffer[file_offset],data,data_length);

	free(header);
	free(programs);
	free(segments);
}
uint64_t get_instruction_length(instruction_t* inst){
	//TODO: Find instruction length for string
	if(!memcmp(&inst->name,"mov",3)){
		uint64_t type = inst->types[0];
		if(type == 0)
			return 2;
		if(type == 1)
			return 4; // Factor in prefix length
		if(type == 2)
			return 5;
		if(type == 3)
			return 10; // Factor in prefix length
	}
	if(!memcmp(&inst->name,"ret",3)){
		return 1;
	}
	return 0;
}
// 0 = 8 bit
// 1 = 16 bit
// 2 = 32 bit
// 3 = 64 bit
// 4 = not a register
uint64_t get_register_size(char* reg){
	if(!memcmp(reg,"0x",2))
		return 4;
	char first = reg[0];
	char second = reg[1];
	if(first <= 'Z')
		first += 'z'-'Z';
	if(second <= 'Z')
		second += 'z'-'Z';
	if(first == 'e')
		return 2;
	if(second == 'l')
		return 0;
	if(second == 'x')
		return 1;
	if(first == 'r'){
		if(second >= 'a') return 3; // rax, rbx, rcx, etc
		char third = reg[2];
		if(third <= 'Z')
			third += 'z'-'Z';
		if(third == 'l') return 0;
		if(third == 'x') return 1;
		if(third == 'd') return 2;
		if(third >= '0' && third <= '5'){
			char fourth = reg[3];
			if(fourth <= 'Z')
				fourth += 'z'-'Z';
			if(fourth == 'l') return 0;
			if(fourth == 'x') return 1;
			if(fourth == 'd') return 2;
			return 3; // Its 64 bit if theres no suffix
		}else{
			return 3; // r8, or r9 with no suffix
		}
	}
	return 4; // No register found
}
uint64_t get_register_num(char* reg){
	char identifier = reg[0];
	char next = reg[1];
	if(identifier <= 90)
		identifier += 32;
	if(identifier == 'e' || identifier == 'r'){
		identifier = reg[1];
		if(identifier <= 90)
			identifier += 32;
		next = reg[2];
		if(next <= 90)
			next += 32;
	}
	if(next == 'i'){
		if(identifier == 's') return 6;
		if(identifier == 'd') return 7;
	}else if(next == 'p'){
		if(identifier == 's') return 4;
		if(identifier == 'b') return 5;
	}else{
		if(identifier == 'a') return 0;
		if(identifier == 'c') return 1;
		if(identifier == 'd') return 2;
		if(identifier == 'b') return 3;
		// This could be done with an atoi
		if(identifier == '8') return 8;
		if(identifier == '9') return 9;
		if(identifier == '1' && reg[2] == '0') return 10;
		if(identifier == '1' && reg[2] == '1') return 11;
		if(identifier == '1' && reg[2] == '2') return 12;
		if(identifier == '1' && reg[2] == '3') return 13;
		if(identifier == '1' && reg[2] == '4') return 14;
		if(identifier == '1' && reg[2] == '5') return 15;
	}
	return 0;
}
void write_instruction(instruction_t* inst, uint8_t* buffer){
	if(!memcmp(&inst->name,"ret",3)){
		buffer[0] = 0xC3;
	}
	if(!memcmp(&inst->name,"mov",3)){
		if(inst->count < 2){
			printf("Error in syntax!");
			exit(1);
		}
		if(inst->types[0] < 4 && inst->types[1] == 4){
			uint64_t start = 1;
			
			uint64_t reg = inst->params[0];
			uint64_t tmp_val = inst->params[1];

			if(inst->types[0] == 0)
				buffer[0] = 0xb0 + reg;
			if(inst->types[0] == 1){
				buffer[0] = 0x66; // cs.d prefix
				buffer[1] = 0xb8 + reg;
				start++;
			}
			if(inst->types[0] == 2)
				buffer[0] = 0xb8 + reg;
			if(inst->types[0] == 3){
				buffer[0] = 0x48; // movabs prefix
				buffer[1] = 0xb8 + reg;
				start++;
			}
	
			buffer[start] = (uint8_t) tmp_val;
			if(inst->types[0] >= 1)
				buffer[start+1] = (uint8_t) (tmp_val >> 8);
			if(inst->types[0] >= 2){
				buffer[start+2] = (uint8_t) (tmp_val >> 16);
				buffer[start+3] = (uint8_t) (tmp_val >> 24);
			}
			if(inst->types[0] == 3){
				buffer[start+4] = (uint8_t) (tmp_val >> 32);
				buffer[start+5] = (uint8_t) (tmp_val >> 40);
				buffer[start+6] = (uint8_t) (tmp_val >> 48);
				buffer[start+7] = (uint8_t) (tmp_val >> 56);
			}
		}
	}
	return;
}
uint64_t count_instructions(char* buffer, uint64_t len){
	uint64_t count = 0;
	for(uint64_t i = 0; i < len; i++){
		if(buffer[i] == '\n' || i == len - 1){
			count++;
		}
	}
	return count;
}
void parse_instructions(instruction_t* inst, char* buffer, uint64_t len){
	uint64_t inst_index = 0;
	uint64_t index = 0;
	for(uint64_t i = 0; i < len; i++){
		if(buffer[i] == '\n' || i == len - 1){
			uint64_t tmp_len = i - index;
			char* str = &buffer[index];
			uint64_t i_index = 0;
			uint64_t index2 = 0;
			for(uint64_t i2 = 0; i2 < tmp_len; i2++){
				if((str[i2] == ' ' && i_index == 0) || str[i2] == ',' || i2 == tmp_len-1){
					if(i_index == 0){
						memcpy(&inst[inst_index].name,str,3);
					}else{
						// Remove leading tabs and spaces
						uint64_t new_index = index2;
						for(; new_index < i; new_index++){
							if(str[new_index] != ' ' && str[new_index] != '\t') break;
						}
						char* str2 = (char*)(((uint64_t)str)+new_index);
						uint64_t tmp_type = get_register_size(str2);
						if(tmp_type != 4){
							// Register
							inst[inst_index].params[i_index-1] = get_register_num(str2);
							inst[inst_index].types[i_index-1] = tmp_type;
						}else{
							inst[inst_index].types[i_index-1] = 4; // Raw value
							uint64_t val = 0;
							// Value
							if(!memcmp(str2,"0x",2)){
								// Hex
								val = strtol(&str2[2],NULL,16);
							}else if(!memcmp(str2,"0b",2)){
								// Binary
								val = strtol(&str2[2],NULL,2);
							}else{
								// Decimal
								val = strtol(str2,NULL,10);
							}
							inst[inst_index].params[i_index-1] = val;
						}
					}

					i_index++;
					index2 = i2 + 1;
				}
			}
			inst[inst_index].count = i_index;
			inst[inst_index].line = inst_index+1;
			inst_index++;
			index = i+1;
		}
	}
}
int main(){
	FILE* src = fopen("asm/test_intel.asm", "r");
	fseek(src, 0L, SEEK_END);
	uint64_t len = ftell(src);
	rewind(src);
	char* buffer = malloc(len); // Allocate bytes for data
	fread(buffer,len,1,src);
	fclose(src);
	
	uint64_t inst_count = count_instructions(buffer, len);
	instruction_t* instructions = (instruction_t*) malloc(sizeof(instruction_t)*inst_count);
	parse_instructions(instructions, buffer, len);
	
	uint64_t bin_len = 0;
	for(uint64_t i = 0; i < inst_count; i++){
		bin_len += get_instruction_length(&instructions[i]);
	}
	uint8_t* bin_buffer = malloc(bin_len);
	
	uint64_t index = 0;

	for(uint64_t i = 0; i < inst_count; i++){
		write_instruction(&instructions[i], &bin_buffer[index]);
		index += get_instruction_length(&instructions[i]);
	}

	uint64_t elen = elf_length(len);
	uint8_t* elf_buffer = malloc(elen);
	create_elf(elf_buffer,bin_buffer,bin_len);

	FILE* fd = fopen("test.elf","w+");
	fwrite(elf_buffer,elen,1,fd);
	fclose(fd);
}
