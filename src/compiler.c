#include <elf.h>
#include <stdlib.h>
#include <stdio.h>

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
	header->section_header_size = 0x40;
}

int main(){
	elf_header_t *header = (elf_header_t*) malloc(sizeof(elf_header_t));
	make_valid_header(header);
	
	uint64_t text = 0x40+0x38+0x40+0x38+0x38+0x10;

	header->program_header = 0x40;
	header->num_program_headers = 1;
	header->section_header = 0x40 + 0x38;
	header->num_section_headers = 3;
	header->section_header_name_index = 2;
	header->entry_point = text;

	program_header_t *prog_header = (program_header_t*) malloc(sizeof(program_header_t));
	prog_header->type = PROGRAM_LOAD;
	prog_header->offset = text; // Right after entry
	prog_header->virtual_address = text;
	prog_header->file_size = 6; // 6 bytes
	prog_header->memory_size = 6; // 6 bytes
	prog_header->flags = 0x5;
	prog_header->align = 0x1000;

	segment_header_t *null_header = (segment_header_t*) malloc(sizeof(segment_header_t));

	segment_header_t *seg_header = (segment_header_t*) malloc(sizeof(segment_header_t));
	seg_header->type = SEGMENT_PROGRAM;
	seg_header->flags = SEGMENT_FLAG_EXEC | SEGMENT_FLAG_ALLOC;
	seg_header->mem_address = 0x0;
	seg_header->name_offset = 1;
	seg_header->file_offset = 0x40+0x38+0x40+0x38+0x38+0x10;
	seg_header->file_size = 0x6;

	segment_header_t *strtab = (segment_header_t*) malloc(sizeof(segment_header_t));
	strtab->name_offset = 7;
	strtab->type = SEGMENT_STRINGS;
	strtab->flags = SEGMENT_FLAG_STRINGS;
	strtab->file_offset = 0x40+0x38+0x40+0x38+0x38+0x6+0x10;
	strtab->mem_address = 0x0;
	strtab->file_size = 17;
	
	FILE* bin = fopen("test.bin", "r");
	uint8_t* buffer = malloc(23); // Allocate 6 bytes for data
	fread(buffer,23,1,bin);
	fclose(bin);

	FILE* fd = fopen("test.elf","w+");
	fwrite(header,sizeof(elf_header_t),1,fd);
	fwrite(prog_header,sizeof(program_header_t),1,fd);
	fwrite(null_header,sizeof(segment_header_t),1,fd);
	fwrite(seg_header,sizeof(segment_header_t),1,fd);
	fwrite(strtab,sizeof(segment_header_t),1,fd);
	fwrite(buffer,23,1,fd);
	fclose(fd);
}
