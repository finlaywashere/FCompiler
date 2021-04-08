#include <elf.h>
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

	programs[SEGMENTS-2].type = PROGRAM_LOAD;
	programs[SEGMENTS-2].offset = file_offset;
	programs[SEGMENTS-2].virtual_address = file_offset;
	programs[SEGMENTS-2].file_size = data_length;
	programs[SEGMENTS-2].memory_size = data_length;
	programs[SEGMENTS-2].flags = 0x5;
	programs[SEGMENTS-2].align = 0x1000;
	
	segments[SEGMENTS-1].type = SEGMENT_PROGRAM;
	segments[SEGMENTS-1].flags = SEGMENT_FLAG_EXEC | SEGMENT_FLAG_ALLOC;
	segments[SEGMENTS-1].mem_address = file_offset;
	segments[SEGMENTS-1].file_offset = file_offset;
	segments[SEGMENTS-1].file_size = data_length;

	segments[2].type = SEGMENT_STRINGS;
	segments[2].flags = SEGMENT_FLAG_STRINGS;
	segments[2].file_offset = file_offset-str_len;
	segments[2].file_size = str_len;
	
	memcpy(buffer,header,sizeof(elf_header_t));
	memcpy((uint8_t*) (((uint64_t)buffer)+0x40),programs,sizeof(program_header_t)*PROGRAMS);
	memcpy((uint8_t*) (((uint64_t)buffer)+0x40+0x38*PROGRAMS),segments,sizeof(segment_header_t)*SEGMENTS);
	memcpy((uint8_t*) (((uint64_t)buffer)+file_offset-str_len),names,str_len);
	memcpy((uint8_t*) (((uint64_t)buffer)+file_offset),data,data_length);

	free(header);
	free(programs);
	free(segments);
}
uint64_t get_instruction_length(char* str){
	//TODO: Find instruction length for string
	if(!memcmp(str,"mov",3)){
		return 5;
	}
	if(!memcmp(str,"ret",3)){
		return 1;
	}
	return 0;
}
void parse_and_write_instruction(char* str, uint8_t* buffer){
	//TODO: Support more registers
	if(!memcmp(str,"ret",3)){
		buffer[0] = 0xC3;
	}
	if(!memcmp(str,"mov",3)){
		uint64_t reg = 0;
		uint64_t val = 0;
		
		uint64_t count = 0;
		uint64_t index = 0;
		for(uint64_t i = 3; i < strlen(str); i++){
			if(str[i] == ',' || i == strlen(str)-1){
				char* str2 = (char*)(((uint64_t)str)+index);
				if(count == 0){
					if(!memcmp(str2,"eax",3)){
						reg = 0;
					}
				}else if(count == 1){
					val = strtol(str2,NULL,10);
				}
				index = i + 1;
				count++;
			}
		}
		buffer[0] = 0xb8 + reg;
		buffer[1] = (uint8_t) val;
		buffer[2] = (uint8_t) (val >> 8);
		buffer[3] = (uint8_t) (val >> 16);
		buffer[4] = (uint8_t) (val >> 24);
	}
	return;
}

int main(){
	FILE* src = fopen("asm/test_intel.asm", "r");
	fseek(src, 0L, SEEK_END);
	uint64_t len = ftell(src);
	rewind(src);
	char* buffer = malloc(len); // Allocate bytes for data
	fread(buffer,len,1,src);
	fclose(src);
	
	uint64_t bin_len = 0;
	uint64_t index = 0;
	for(uint64_t i = 0; i < len; i++){
		if(buffer[i] == '\n' || i == len - 1){
			uint64_t tmp_len = i - index;
			char* str = malloc(tmp_len);
			memcpy(str,(char*)(((uint64_t)buffer)+index),tmp_len);
			uint64_t inst_len = get_instruction_length(str);
			bin_len += inst_len;
			index = i+1;
		}
	}
	index = 0;
	uint8_t* bin_buffer = malloc(bin_len);
	for(uint64_t i = 0; i < len; i++){
                if(buffer[i] == '\n' || i == len - 1){
                        uint64_t tmp_len = i - index;
                        char* str = malloc(tmp_len);
                        memcpy(str,(char*)(((uint64_t)buffer)+index),tmp_len);
                        parse_and_write_instruction(str,(uint8_t*)(((uint64_t)bin_buffer)+index));
			index = i+1;
                }
        }
	
	uint64_t elen = elf_length(len);
	uint8_t* elf_buffer = malloc(elen);
	create_elf(elf_buffer,bin_buffer,len);

	FILE* fd = fopen("test.elf","w+");
	fwrite(elf_buffer,elen,1,fd);
	fclose(fd);
}
