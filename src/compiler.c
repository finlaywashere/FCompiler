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
	uint64_t length = 0x40 + 0x40 * SEGMENTS + 0x38 * PROGRAMS;
	
	length += 0x1000 - length;

	length += + data_length + len;
	return length;
}
void create_elf(uint8_t* buffer, uint8_t* data, uint64_t data_length, instruction_t *inst){
	elf_header_t *header = (elf_header_t*) malloc(sizeof(elf_header_t));
	make_valid_header(header);
	
	uint64_t file_offset = elf_length(data_length)-data_length;
	uint64_t str_len = file_offset - (0x40 + 0x40 * SEGMENTS + 0x38 * PROGRAMS);

	header->program_header = 0x40; // Right after ELF header
	header->num_program_headers = PROGRAMS;
	header->segment_header = 0x40 + 0x38 * PROGRAMS; // Right after program headers
	header->num_segment_headers = SEGMENTS;
	header->entry_point = inst->origin;
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
	programs[0].virtual_address = inst->origin;
	programs[0].file_size = data_length;
	programs[0].memory_size = data_length;
	programs[0].flags = 0x5;
	programs[0].align = 0x1000;
	
	segments[1].type = SEGMENT_PROGRAM;
	segments[1].flags = SEGMENT_FLAG_EXEC | SEGMENT_FLAG_ALLOC;
	segments[1].mem_address = inst->origin;
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
	if(!memcmp(&inst->name,"mov",3)){
		if(inst->types[0] < 4 && (inst->types[1] == 4 || inst->types[1] == 5)){ // If it is type 4 then it is a value, if it is type 5 then it is a symbol that will be substituted in as a value
			switch(inst->types[0]){
				case 0:
					return 2;
				case 1:
					return 4; // Factor in prefix length
				case 2:
					return 5;
				case 3:
					return 10; // Factor in prefix length
			}
		}else if(inst->types[0] < 4 && inst->types[1] < 4){
			switch(inst->types[0]){
				case 0:
					return 2;
				case 1:
					return 3; // Factor in prefix length
				case 2:
					return 2;
				case 3:
					return 3; // Factor in prefix length
			}
		}else if(inst->types[0] < 4 && (inst->types[1] == 7 || inst->types[1] == 8)){ // If it is type 7 then it is an address, if it is type 8 then it is a symbol that will be substituted in as an address
			switch(inst->types[0]){
				case 0:
					return 7;
				case 1:
					return 8; // Factor in prefix length
				case 2:
					return 7;
				case 3:
					return 8; // Factor in prefix length
			}
		}
	}
	if(!memcmp(&inst->name,"pop",3) || !memcmp(&inst->name,"push",4)) return 1;
	if(!memcmp(&inst->name,"call",4)){
			switch(inst->types[0]){
				case 3:
					return 2;
				case 4:
					return 5;
				case 5:
					return 5;
			}
	}
	if(!memcmp(&inst->name,"inc",3) || !memcmp(&inst->name,"dec",3)){
		switch(inst->types[0]){
			case 2:
				return 2;
			case 1:
				return 3;
			case 3:
				return 3;
			case 0:
				return 2;
		}
	}
	if(!memcmp(&inst->name,"ret",3)) return 1;
	return 0;
}
/*
 Used to get the size of a register, now gets the data type of a string
 0 = 8 bit
 1 = 16 bit
 2 = 32 bit
 3 = 64 bit
 4 = value (uint64_t)
 5 = symbol
 6 = defining symbol
 7 = address
 8 = symbol address
*/
uint64_t get_register_size(char* reg){
	if(!memcmp(reg,"0x",2))
		return 4;
	char first = reg[0];
	char second = reg[1];
	if(first <= 'Z' && first >= 'A')
		first += 'z'-'Z';
	if(second <= 'Z' && second >= 'A')
		second += 'z'-'Z';
	if(first == 'e')
		return 2;
	if(second == 'l' || second == 'h')
		return 0;
	if(second == 'x')
		return 1;
	if(first == 'r'){
		if(second >= 'a') return 3; // rax, rbx, rcx, etc
		char third = reg[2];
		if(third <= 'Z' && third >= 'A')
			third += 'z'-'Z';
		if(third == 'l') return 0;
		if(third == 'x') return 1;
		if(third == 'd') return 2;
		if(third >= '0' && third <= '5'){
			char fourth = reg[3];
			if(fourth <= 'Z' && fourth >= 'A')
				fourth += 'z'-'Z';
			if(fourth == 'l') return 0;
			if(fourth == 'x') return 1;
			if(fourth == 'd') return 2;
			return 3; // Its 64 bit if theres no suffix
		}else{
			return 3; // r8, or r9 with no suffix
		}
	}
	if(first >= '0' && first <= '9') return 4; // It starts with a number/0x/0b
	if(first == '['){
		if(second >= '0' && second <= '9') return 7; // It starts with [ then a number so its an address to read from
		return 8; // Its a symbol in address brackets
	}
	return 5; // Its probably a symbol
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
		if(next != 'h'){
			if(identifier == 'a') return 0;
			if(identifier == 'c') return 1;
			if(identifier == 'd') return 2;
			if(identifier == 'b') return 3;
		}else{
			if(identifier == 'a') return 4;
			if(identifier == 'c') return 5;
			if(identifier == 'd') return 6;
			if(identifier == 'b') return 7;
		}
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
void write_int_to_buffer(uint64_t i, uint8_t* buffer, uint64_t type){
	buffer[0] = (uint8_t) i;
	if(type >= 1)
		buffer[1] = (uint8_t) (i >> 8);
	if(type >= 2){
		buffer[2] = (uint8_t) (i >> 16);
		buffer[3] = (uint8_t) (i >> 24);
	}
	if(type == 3){
		buffer[4] = (uint8_t) (i >> 32);
		buffer[5] = (uint8_t) (i >> 40);
		buffer[6] = (uint8_t) (i >> 48);
		buffer[7] = (uint8_t) (i >> 56);
	}
}
void write_instruction(instruction_t* inst, uint8_t* buffer, uint64_t address){
	if(!memcmp(&inst->name,"ret",3)){
		buffer[0] = 0xC3;
	}
	if(!memcmp(&inst->name,"mov",3)){
		if(inst->count < 2){
			printf("Error in syntax!");
			exit(1);
		}
		uint64_t max_size = 0;
		for(uint64_t i = 0; i < PARAMS; i++){
			if(inst->types[i] > max_size && inst->types[i] <= 3)
				max_size = inst->types[i];
		}
		uint64_t start = 0;
		if(max_size == 3){
			buffer[0] = 0x48; // 64 bit prefix
			start = 1;
		}
		if(max_size == 1){
			buffer[0] = 0x66; // 16 bit prefix
			start = 1;
		}
		uint64_t type = 0; // 0 = data, 1 = register, 2 = memory
		uint64_t value = inst->params[1];
		if(inst->types[0] < 4 && inst->types[1] == 4){
			uint64_t opcode = 0xb8;
			if(inst->types[0] == 0)
				opcode = 0xb0;
			buffer[start] = opcode + inst->params[0];
			type = 0;
		}else if(inst->types[0] < 4 && inst->types[1] < 4){
			uint64_t opcode = 0x89;
			if(inst->types[0] == 0)
				opcode = 0x88;
			buffer[start] = opcode;
			start++;
			buffer[start] = 0xc0 + (inst->params[1] * 8) + inst->params[0];
			type = 1;
		}else if(inst->types[0] < 4 && inst->types[1] == 7){
			uint64_t opcode = 0x8b;
			if(inst->types[0] == 0)
				opcode = 0x8a;
			buffer[start] = opcode;
			start++;
			buffer[start] = 0x04 + 8 * inst->params[0];
			start++;
			buffer[start] = 0x25;
			type = 2;
		}else if(inst->types[0] == 7 && inst->types[1] < 4){
			uint64_t opcode = 0x89;
			if(inst->types[0])
				opcode = 0x88;
			buffer[start] = opcode;
			start++;
			buffer[start] = 0x04 + 8 * inst->params[0];
			start++;
			buffer[start] = 0x25;
		}
		start++;
		if(type == 0){
			write_int_to_buffer(value,&buffer[start],inst->types[0]);
		}else if(type == 1){
			buffer[start] = 0xc0 + (inst->params[1] * 8) + inst->params[0];
		}else if(type == 2){
			write_int_to_buffer(value,&buffer[start],2);
		}
	}
	if(!memcmp(&inst->name,"pop",3)){
		buffer[0] = 0x58 + inst->params[0];
	}
	if(!memcmp(&inst->name,"push",4)){
		buffer[0] = 0x50 + inst->params[0];
	}
	if(!memcmp(&inst->name,"inc",3)){
		uint64_t start = 1;
		if(inst->types[0] == 2){
			buffer[0] = 0xff;
		}
		if(inst->types[0] == 1){
			buffer[0] = 0x66;
			buffer[1] = 0xff;
			start = 2;
		}
		if(inst->types[0] == 3){
			buffer[0] = 0x48;
			buffer[1] = 0xff;
			start = 2;
		}
		if(inst->types[0] == 0){
			buffer[0] = 0xfe;
		}
		buffer[start] = 0xc0 + inst->params[0];
	}
	if(!memcmp(&inst->name,"dec",3)){
		uint64_t start = 1;
		if(inst->types[0] == 2){
			buffer[0] = 0xff;
		}
		if(inst->types[0] == 0){
			buffer[0] = 0xfe;
		}
		if(inst->types[0] == 1){
			buffer[0] = 0x66;
			buffer[1] = 0xff;
			start = 2;
		}
		if(inst->types[0] == 3){
			buffer[0] = 0x48;
			buffer[1] = 0xff;
			start = 2;
		}
		buffer[start] = 0xc8 + inst->params[0];
	}
	if(!memcmp(&inst->name,"call",4)){
		if(inst->types[0] == 3){
			// Register
			buffer[0] = 0xff;
			buffer[1] = 0xd0 + inst->params[0];
		}else if(inst->types[0] >= 4){
			// Address
			buffer[0] = 0xe8;
			uint64_t return_pointer = address + 5;
			uint64_t new_pointer = inst->params[0] - return_pointer; // The new pointer to the call
			buffer[1] = (uint8_t) new_pointer;
			buffer[2] = (uint8_t) (new_pointer >> 8);
			buffer[3] = (uint8_t) (new_pointer >> 16);
			buffer[4] = (uint8_t) (new_pointer >> 24);
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
void group_symbols(instruction_t* inst, uint64_t len, char* buffer){
	uint64_t curr_id = 1;
	for(uint64_t i = 0; i < len; i++){
		for(uint64_t i4 = 0; i4 < PARAMS; i4++){
			if(inst[i].types[i4] != 6) continue;
			inst[i].params[i4] = curr_id;
			char* str = &buffer[inst[i].s_start[i4]];
			uint64_t s_len = inst[i].s_len[i4];
			// Loop through each and mark all of the same symbols
			for(uint64_t i2 = 0; i2 < len; i2++){
				for(uint64_t i3 = 0; i3 < PARAMS; i3++){
					if(inst[i2].types[i3] != 5 && inst[i2].types[i3] != 8) continue;
					if(s_len != inst[i2].s_len[i3]) continue;
					if(!memcmp(str,&buffer[inst[i2].s_start[i3]],s_len)){
						inst[i2].params[i3] = curr_id;
					}
				}
			}
			curr_id++;
		}
	}
	inst->symbols = curr_id;
}
void find_symbols_and_replace(instruction_t* inst, uint64_t len){
	for(uint64_t id = 1; id < inst->symbols; id++){
		for(uint64_t i = 0; i < len; i++){
			for(uint64_t p = 0; p < PARAMS; p++){
				if(inst[i].types[p] != 6) continue;
				uint64_t address = inst[i].address;
				uint64_t symbol = inst[i].params[p];
				for(uint64_t i2 = 0; i2 < len; i2++){
					for(uint64_t p2 = 0; p2 < len; p2++){
						if(inst[i2].types[p2] != 5 && inst[i2].types[p2] != 8) continue;
						if(inst[i2].params[p2] != symbol) continue;
						if(inst[i2].types[p2] == 5)
							inst[i2].types[p2] = 4; // Set to raw value
						if(inst[i2].types[p2] == 8)
							inst[i2].types[p2] = 7; // Set to address
						inst[i2].params[p2] = address; // Set address
					}
				}
			}
		}
	}
}
void parse_instructions(instruction_t* inst, char* buffer, uint64_t len, uint64_t count){
	uint64_t inst_index = 0;
	uint64_t index = 0;
	for(uint64_t i = 0; i < len; i++){
		if(buffer[i] == '\n' || i == len - 1){
			memset(&inst[inst_index],0,sizeof(instruction_t));
			uint64_t tmp_len = i - index;
			char* str = &buffer[index];
			uint64_t i_index = 0;
			uint64_t index2 = 0;
			for(uint64_t i2 = 0; i2 < tmp_len; i2++){
				if((str[i2] == ' ' && i_index == 0) || str[i2] == ',' || str[i2] == '\t' || i2 == tmp_len-1){
					if(i_index == 0 && str[i2] != ':'){
						memcpy(&inst[inst_index].name,str,i2);
					}else{
						// Remove leading tabs and spaces
						uint64_t new_index = index2;
						for(; new_index < i; new_index++){
							if(str[new_index] != ' ' && str[new_index] != '\t') break;
						}
						char* str2 = &str[new_index];
						uint64_t tmp_type = get_register_size(str2);
						if(tmp_type < 4){
							// Register
							inst[inst_index].params[i_index-1] = get_register_num(str2);
							inst[inst_index].types[i_index-1] = tmp_type;
						}else if(tmp_type == 4){
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
						}else if(tmp_type == 5 || tmp_type == 8){
							inst[inst_index].types[i_index-1] = tmp_type; // Symbol
							inst[inst_index].s_start[i_index-1] = new_index+index;
							inst[inst_index].s_len[i_index-1] = i2-new_index+1;
						}else if(tmp_type == 7){
							// Its an address to read/write from/to
							inst[inst_index].types[i_index-1] = 7; // Raw value
                            uint64_t val = 0;
                            // Value
                            if(!memcmp(str2,"[0x",2)){
                                // Hex
                                val = strtol(&str2[3],NULL,16);
                            }else if(!memcmp(str2,"[0b",2)){
                                // Binary
                                val = strtol(&str2[3],NULL,2);
                            }else{
                                // Decimal
                                val = strtol(&str2[1],NULL,10);
                            }
                            inst[inst_index].params[i_index-1] = val;
						}
						if(str[i2] == ':'){
							// label
							memcpy(&inst[inst_index].name,"label",5);
							inst[inst_index].types[0] = 6; // set first type to symbol
							inst[inst_index].s_start[0] = new_index+index;
							inst[inst_index].s_len[0] = i2-new_index;
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
	inst->origin = 0x401000;
	for(uint64_t i = 0; i < count; i++){
		if(!memcmp(&inst[i].name,"org",3)){
			inst->origin = inst[i].params[0]; // first instruction stores origin
		}
	}
	uint64_t address = inst->origin;
	for(uint64_t i = 0; i < count; i++){
		inst[i].address = address;
		address += get_instruction_length(&inst[i]);
	}
	group_symbols(inst, count, buffer);
	find_symbols_and_replace(inst,count);
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
	parse_instructions(instructions, buffer, len, inst_count);

	uint64_t bin_len = 0;
	for(uint64_t i = 0; i < inst_count; i++){
		bin_len += get_instruction_length(&instructions[i]);
	}
	uint8_t* bin_buffer = malloc(bin_len);
	memset(bin_buffer,0,bin_len);

	uint64_t index = instructions->origin;

	for(uint64_t i = 0; i < inst_count; i++){
		write_instruction(&instructions[i], &bin_buffer[index-instructions->origin], index);
		index += get_instruction_length(&instructions[i]);
	}

	uint64_t elen = elf_length(len);
	uint8_t* elf_buffer = malloc(elen);
	create_elf(elf_buffer,bin_buffer,bin_len,instructions);

	FILE* fd = fopen("test.elf","w+");
	fwrite(elf_buffer,elen,1,fd);
	fclose(fd);
}
