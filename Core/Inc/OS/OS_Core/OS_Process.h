/*
 * OS_Process.h
 *
 *  Created on: Apr 15, 2025
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_CORE_OS_PROCESS_H_
#define INC_OS_OS_CORE_OS_PROCESS_H_

#include <stdarg.h>
#include "OS/OS_Core/OS_Common.h"

/**********************************************
 * PUBLIC TYPES
 *********************************************/


/* ELF ID header
 ---------------------------------------------------*/
typedef struct{
	uint8_t magic[4];	//Always 0x7F, 'E', 'L', 'F'
	uint8_t class;		//This byte is set to either 1 or 2 to signify 32- or 64-bit format, respectively.
	uint8_t data;		//This byte is set to either 1 or 2 to signify little or big endianness, respectively.
	uint8_t version;	//Always 1
	uint8_t os_abi;		//0x00 System V, 0x01 HP-UX, 0x02 NetBSD, 0x03 Linux, 0x04 GNU Hurd, 0x06 Solaris, 0x07 AIX (Monterey), 0x08 IRIX, 0x09 FreeBSD, 0x0A Tru64, 0x0B Novell Modesto, 0x0C OpenBSD, 0x0D OpenVMS, 0x0E NonStop Kernel, 0x0F AROS, 0x10 FenixOS, 0x11 Nuxi CloudABI, 0x12 Stratus Technologies OpenVOS
	uint8_t abi_version;
	uint8_t pad[7];		//Padding. Unused
} __packed os_elf_id_t;

/* ELF header
 ---------------------------------------------------*/
typedef struct{
	os_elf_id_t e_ident;
	uint16_t	e_type;
	uint16_t	e_machine;
	uint32_t	e_version;		//Set to 1 for the original version of ELF.
	uint32_t	e_entry;		//This is the memory address of the entry point from where the process starts executing.
	uint32_t	e_phoff;		//Points to the start of the program header table.
	uint32_t	e_shoff;		//Points to the start of the section header table.
	uint32_t 	e_flags;		//Interpretation of this field depends on the target architecture.
	uint16_t 	e_ehsize;		//Contains the size of this header, normally 64 Bytes for 64-bit and 52 Bytes for 32-bit format.
	uint16_t	e_phentsize;	//Contains the size of a program header table entry.
	uint16_t	e_phnum;		//Contains the number of entries in the program header table.
	uint16_t	e_shentsize;	//Contains the size of a section header table entry.
	uint16_t	e_shnum;		//Contains the number of entries in the section header table.
	uint16_t	e_shstrndx;		//Contains index of the section header table entry that contains the section names.h

} __packed os_elf_header_t;

/* ELF program header
 ---------------------------------------------------*/
typedef struct{
	uint32_t 	p_type;		//0x00000000 PT_NULL Program header table entry unused., 0x00000001 PT_LOAD Loadable segment. 0x00000002 PT_DYNAMIC Dynamic linking information. 0x00000003 PT_INTERP Interpreter information. 0x00000004 PT_NOTE Auxiliary information. 0x00000005 PT_SHLIB Reserved. 0x00000006 PT_PHDR Segment containing program header table itself. 0x00000007 PT_TLS Thread-Local Storage template.
	uint32_t  	p_offset;	//Offset of the segment in the file image.
	uint32_t	p_vaddr;	//Virtual address of the segment in memory.
	uint32_t	p_paddr;	//On systems where physical address is relevant, reserved for segment's physical address.
	uint32_t	p_filesz;	//Size in bytes of the segment in the file image. May be 0.
	uint32_t 	p_memsz;	//Size in bytes of the segment in memory. May be 0.
	uint32_t 	p_flags;	//Segment-dependent flags (position for 32-bit structure).
	uint32_t	p_align;	//0 and 1 specify no alignment. Otherwise should be a positive, integral power of 2, with p_vaddr equating p_offset modulus p_align
} __packed os_elf_programHeader_t;

/* ELF section header
 ---------------------------------------------------*/
typedef struct{
	uint32_t sh_name;		//An offset to a string in the .shstrtab section that represents the name of this section.
	uint32_t sh_type;		//0x0 SHT_NULL Section header table entry unused, 0x1 SHT_PROGBITS Program data, 0x2 SHT_SYMTAB Symbol table 0x3 SHT_STRTAB String table, 0x4 SHT_RELA Relocation entries with addends, 0x5 SHT_HASH Symbol hash table, 0x6 SHT_DYNAMIC Dynamic linking information, 0x7 SHT_NOTE Notes, 0x8 SHT_NOBITS Program space with no data (bss), 0x9 SHT_REL Relocation entries, no addends, 0x0A SHT_SHLIB Reserved, 0x0B SHT_DYNSYM Dynamic linker symbol table, 0x0E SHT_INIT_ARRAY	Array of constructors, 0x0F SHT_FINI_ARRAY Array of destructors, 0x10 SHT_PREINIT_ARRAY	Array of pre-constructors, 0x11 SHT_GROUP Section group, 0x12 SHT_SYMTAB_SHNDX Extended section indices, 0x13 SHT_NUM Number of defined types., 0x60000000 SHT_LOOS Start OS-specific.
	uint32_t sh_flags;		//0x1 SHF_WRITE Writable, 0x2 SHF_ALLOC Occupies memory during execution, 0x4 SHF_EXECINSTR Executable, 0x10 SHF_MERGE Might be merged, 0x20 SHF_STRINGS Contains null-terminated strings, 0x40 SHF_INFO_LINK 'sh_info' contains SHT index, 0x80 SHF_LINK_ORDER Preserve order after combining, 0x100 SHF_OS_NONCONFORMING Non-standard OS specific handling required, 0x200 SHF_GROUP Section is member of a group, 0x400 SHF_TLS Section hold thread-local data, 0x0FF00000 SHF_MASKOS OS-specific, 0xF0000000 SHF_MASKPROC Processor-specific, 0x4000000 SHF_ORDERED	Special ordering requirement (Solaris), 0x8000000 SHF_EXCLUDE Section is excluded unless referenced or allocated (Solaris)
	uint32_t sh_addr;		//Virtual address of the section in memory, for sections that are loaded.
	uint32_t sh_offset; 	//Offset of the section in the file image.
	uint32_t sh_size;		//Size in bytes of the section in the file image. May be 0.
	uint32_t sh_link;		//Contains the section index of an associated section. This field is used for several purposes, depending on the type of section.
	uint32_t sh_info;		//Contains extra information about the section. This field is used for several purposes, depending on the type of section.
	uint32_t sh_addralign;	//Contains the required alignment of the section. This field must be a power of two.
	uint32_t sh_entsize;	//Contains the size, in bytes, of each entry, for sections that contain fixed-size entries. Otherwise, this field contains zero.
} __packed os_elf_sectionHeader_t;

/* Process information
 ---------------------------------------------------*/
typedef struct os_process_ {
	int (*entry_fn)(int, char**);
	uint8_t* segments;
	void* thread_list;
	char* p_name;
	uint32_t gotBaseAddr;
	os_elf_header_t elf_H;
	uint16_t PID;
} os_process_t;


/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS Process get by PID
 *
 * @brief This function searches for a process with a giben PID
 *
 * @param uint16_t pid : [in] PID to search
 *
 * @return os_process_t* : reference to found process
 *
 **********************************************************************/
os_process_t* os_process_getByPID(uint16_t pid);


/***********************************************************************
 * OS Create process
 *
 * @brief This function creates a process using its ELF file
 *
 * @param char* file   : [in] File's name
 * @param void* argc   : [in] Argument number to be passed to the task
 * @param char* argv[] : [in] Array of strings to be passed to the task
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_process_create(char* file, int argc, char* argv[]);


/***********************************************************************
 * Kill a process
 *
 * @brief Kill a process by releasing its
 *
 * @param uint16_t pid : [in] PID
 *
 * @return os_err_e : An error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_process_kill(os_process_t* proc);


#endif /* INC_OS_OS_CORE_OS_PROCESS_H_ */
