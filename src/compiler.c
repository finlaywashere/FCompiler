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

	header->type = PROGRAM_TYPE_EXEC;
	header->instruction_set = INSTRUCTION_X86_64;
	header->elf_version_2 = 1; // This ELF version must also be 1
}

int main(){
	const char* file = "test.elf";
	elf_header_t *header = (elf_header_t*) malloc(sizeof(elf_header_t));
	make_valid_header(header);
	
	FILE* fd = fopen(file,"w+");
	fwrite(header,sizeof(elf_header_t),1,fd);
}
