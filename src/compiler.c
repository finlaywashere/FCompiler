#include <elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SEGMENTS 2
#define PROGRAMS 1

const char* names = ".text\0.shstrtab\0";

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
	header->segment_header_name_index = 1;

	program_header_t* programs = (program_header_t*) malloc(sizeof(program_header_t)*PROGRAMS);
	
	segment_header_t* segments = (segment_header_t*) malloc(sizeof(segment_header_t)*SEGMENTS);
	
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
	
	segments[0].type = SEGMENT_PROGRAM;
	segments[0].flags = SEGMENT_FLAG_EXEC | SEGMENT_FLAG_ALLOC;
	segments[0].mem_address = file_offset;
	segments[0].file_offset = file_offset;
	segments[0].file_size = data_length;

	segments[1].type = SEGMENT_STRINGS;
	segments[1].flags = SEGMENT_FLAG_STRINGS;
	segments[1].file_offset = file_offset-str_len;
	segments[1].file_size = str_len;
	
	memcpy(buffer,header,sizeof(elf_header_t));
	memcpy((uint8_t*) (((uint64_t)buffer)+0x40),programs,sizeof(program_header_t)*PROGRAMS);
	memcpy((uint8_t*) (((uint64_t)buffer)+0x40+0x38*PROGRAMS),segments,sizeof(segment_header_t)*SEGMENTS);
	memcpy((uint8_t*) (((uint64_t)buffer)+file_offset-str_len),names,str_len);
	memcpy((uint8_t*) (((uint64_t)buffer)+file_offset),data,data_length);

	free(header);
	free(programs);
	free(segments);
}

int main(){
	FILE* bin = fopen("test.bin", "r");
	fseek(bin, 0L, SEEK_END);
	uint64_t len = ftell(bin);
	rewind(bin);
	uint8_t* buffer = malloc(len); // Allocate bytes for data
	fread(buffer,len,1,bin);
	fclose(bin);
	
	uint64_t elen = elf_length(len);
	uint8_t* elf_buffer = malloc(elen);
	create_elf(elf_buffer,buffer,len);

	FILE* fd = fopen("test.elf","w+");
	fwrite(elf_buffer,elen,1,fd);
	fclose(fd);
}
