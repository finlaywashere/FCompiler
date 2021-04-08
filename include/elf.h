#include <stdint.h>
#include <stddef.h>

#define ABI_LINUX 0x03

#define PROGRAM_TYPE_NONE 0x00
#define PROGRAM_TYPE_REL 0x01
#define PROGRAM_TYPE_EXEC 0x02

#define INSTRUCTION_X86_64 0x3E

#define PROGRAM_NULL 0x00
#define PROGRAM_LOAD 0x01
#define PROGRAM_DYNAMIC 0x02
#define PROGRAM_INTERP 0x03
#define PROGRAM_NOTE 0x04
#define PROGRAM_RESV 0x05
#define PROGRAM_PROGHEADER 0x06

#define SEGMENT_NULL 0x00
#define SEGMENT_PROGRAM 0x01
#define SEGMENT_SYMBOLS 0x02
#define SEGMENT_STRINGS 0x03
#define SEGMENT_RELOC 0x04
#define SEGMENT_HASH 0x05
#define SEGMENT_DYNAMIC 0x06
#define SEGMENT_NOTE 0x07
#define SEGMENT_EMPTY 0x08
#define SEGMENT_DYNAMIC_SYMBOLS 0x0B
#define SEGMENT_CONSTRUCTORS 0x0E
#define SEGMENT_DESTRUCTORS 0x0F

#define SEGMENT_FLAG_WRITE 0x01
#define SEGMENT_FLAG_ALLOC 0x02
#define SEGMENT_FLAG_EXEC 0x04
#define SEGMENT_FLAG_STRINGS 0x20
#define SEGMENT_FLAG_INFO_LINK 0x40
#define SEGMENT_FLAG_LINK_ORDER 0x80

struct elf_header{
	char magic[4];
	uint8_t bits;
	uint8_t endian;
	uint8_t elf_version;
	uint8_t abi;
	uint8_t abi_version;
	uint8_t reserved[7];
	uint16_t type;
	uint16_t instruction_set;
	uint32_t elf_version_2;
	uint64_t entry_point;
	uint64_t program_header;
	uint64_t segment_header;
	uint32_t flags;
	uint16_t header_size;
	uint16_t program_header_size;
	uint16_t num_program_headers;
	uint16_t segment_header_size;
	uint16_t num_segment_headers;
	uint16_t segment_header_name_index;
}__attribute__((packed));
typedef struct elf_header elf_header_t;

struct program_header{
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t virtual_address;
	uint64_t physical_address;
	uint64_t file_size;
	uint64_t memory_size;
	uint64_t align;
}__attribute__((packed));
typedef struct program_header program_header_t;

struct segment_header{
	uint32_t name_offset;
	uint32_t type;
	uint64_t flags;
	uint64_t mem_address;
	uint64_t file_offset;
	uint64_t file_size;
	uint32_t link;
	uint32_t info;
	uint64_t align;
	uint64_t entry_size;
}__attribute__((packed));
typedef struct segment_header segment_header_t;
