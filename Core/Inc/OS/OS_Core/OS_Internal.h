/*
 * OS_Internal.h
 *
 *  Created on: Oct 23, 2021
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_INTERNAL_H_
#define INC_OS_OS_INTERNAL_H_

#include "OS/OS_Core/OS_Common.h"
#include "OS/OS_Core/OS_Tasks.h"
#include "OS/OS_Core/OS_MsgQ.h"

/**********************************************
 * OS PRIVATE TYPES
 *********************************************/

/* Task list cell
 ---------------------------------------------------*/
typedef struct os_list_cell_{
	struct os_list_cell_* next;			//Points to the next stack on the list
	struct os_list_cell_* prev;			//Points to the previous stack on the list
	void*  		 		 element;		//Pointer to the element
	uint32_t			 order;			//To store the order of waiting
} os_list_cell_t;

/* Head object
 ---------------------------------------------------*/
typedef struct os_list_head_{
	os_list_cell_t 	 	 head;			//Head structure
	os_list_cell_t*		 first;			//Pointer to the first element
	os_list_cell_t*		 last;			//Pointer to the first element
	uint32_t 			 listSize;		//Size of list
} os_list_head_t;

/* Enum to add to a list
 ---------------------------------------------------*/
typedef enum{
	OS_LIST_FIRST,
	OS_LIST_LAST,
	__OS_LIST_INVALID,
}os_list_pos_e;

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

/* Helper structure to map the different segments
 ---------------------------------------------------*/
typedef struct{
	uint32_t original_addr;
	uint32_t remapped_addr;
	uint32_t size;
} os_elf_mapping_el_t;


/* Structure containing the information to execute the program
 ---------------------------------------------------*/
typedef struct{
	void* entryPoint;
	uint32_t gotBase;
}os_elf_prog_t;

/**********************************************
 * OS PRIVATE FUNCTIONS
 *********************************************/

//////////////////////////////////////////////// Priority //////////////////////////////////////////////////


/***********************************************************************
 * OS Object Update Prio
 *
 * @brief This function updates the maximum priority of an object
 *
 * @param os_handle_t h : [in] The reference to the object
 *
 * @return bool : (1) = priority changed; (0) = Nothing changed
 *
 **********************************************************************/
void os_obj_updatePrio(os_handle_t h);


//////////////////////////////////////////////// TASKS //////////////////////////////////////////////////


/***********************************************************************
 * OS Task Init
 *
 * @brief This function initializes the default tasks
 *
 * @param char* main_name				: [in] main task name. A null name creates a nameless task.
 * @param int8_t   main_task_priority   : [in] main stack base priority (0 to 127)
 * @param uint32_t interrput_stack_size : [in] size of the interrupt stack
 * @param uint32_t idle_stack_size 		: [in] size of the idle stack
 *
 * @return os_err_e : Error code (0 = OK)
 *
 **********************************************************************/
os_err_e os_task_init(char* main_name, int8_t main_task_priority, uint32_t interrput_stack_size, uint32_t idle_stack_size);


/***********************************************************************
 * OS Task Must Yeild
 *
 * @brief This function checks the priority of all tasks and decides if the current task should yield
 *
 * @return bool : 1 = yeild
 *
 **********************************************************************/
bool os_task_must_yeild();


//////////////////////////////////////////////// HANDLE LISTS //////////////////////////////////////////////////

/***********************************************************************
 * OS List Init
 *
 * @brief This function initializes a list
 *
 * @return os_list_head_t* : reference to the head of the list
 **********************************************************************/
os_list_head_t* os_list_init();


/***********************************************************************
 * OS List Search
 *
 * @brief This function searches for an element in a list
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 * @param void* el	  		   : [in] element to search
 *
 * @return os_list_cell_t* : reference to the cell containing the element or null if not found
 **********************************************************************/
os_list_cell_t* os_list_search(os_list_head_t* head, void* el);


/***********************************************************************
 * OS Handle List Search by name
 *
 * @brief This function searches for a handle in a list using its name and type. Must be used in handle lists
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 * @param os_obj_type_e type   : [in] type of the object
 * @param char* name		   : [in] name of the object
 *
 * @return os_list_cell_t* : reference to the cell containing the handle or null if not found
 **********************************************************************/
os_list_cell_t* os_handle_list_searchByName(os_list_head_t* head, os_obj_type_e type, char const * name);


/***********************************************************************
 * OS List add
 *
 * @brief This function adds an element in a list
 *
 * @param os_list_head_t* head  : [in] reference to the head of the list
 * @param void* el	   		    : [in] reference to the element
 * @param os_list_add_pos_e pos : [in] flag to indicate whether to add at the first or last position
 *
 * @return os_err_e : error code (0 = OK)
 **********************************************************************/
os_err_e os_list_add(os_list_head_t* head, void* el, os_list_pos_e pos);


/***********************************************************************
 * OS List Pop
 *
 * @brief This function pops an element from a list
 *
 * @param os_list_head_t* head  : [ in] reference to the head of the list
 * @param os_list_add_pos_e pos : [ in] flag to indicate whether to add at the first or last position
 * @param os_err_e* err			: [out] OS error code, NULL to ignore
 *
 * @return void* : address of the element popped or NULL if error
 **********************************************************************/
void* os_list_pop(os_list_head_t* head, os_list_pos_e pos, os_err_e* err);


/***********************************************************************
 * OS List remove
 *
 * @brief This function removes an element from a list
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 * @param void* el	   		   : [in]  reference to the element
 *
 * @return os_err_e : error code (0 = OK)
 **********************************************************************/
os_err_e os_list_remove(os_list_head_t* head, void* el);


/***********************************************************************
 * OS List clear
 *
 * @brief This function clears a list, freeing all its cells and head
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 *
 **********************************************************************/
void os_list_clear(os_list_head_t* head);


/***********************************************************************
 * OS Handle List sort
 *
 * @brief This function sorts a task list, sorting from higher priority to lower.
 * If priorities are equal the first task that entered the list will be put first.
 * Must be used on task lists
 *
 * @param os_list_head_t* head : [in] reference to the head of the list
 *
 **********************************************************************/
void os_task_list_sort(os_list_head_t* head);


/***********************************************************************
 * OS Handle list get object to update
 *
 * @brief This function searches for an object in the object list that needs to update. Must be used in a handle list
 *
 * @return os_handle_t handle to the object to update or NULL if nothing to do
 **********************************************************************/
os_handle_t os_handle_list_getObjToUpdate();


/***********************************************************************
 * OS Task list is object free on task
 *
 * @brief This function scans an object's block list to decide whether the object is still free on a given task. Must be used with a task list.
 *
 * @param os_handle_t obj  : [in] object to scan
 * @param os_handle_t task : [in] task to stop the scan
 *
 * @return bool : 1 = object is free ; 0 = object is not free
 **********************************************************************/
bool os_task_list_isObjFreeOnTask(os_handle_t obj, os_handle_t task);


/***********************************************************************
 * OS Handle update and check
 *
 * @brief This function updates the block list of all objects that needs an update.
 * It returns whether the current task must yield or not.
 * Must be used with a handle list
 *
 * @param os_handle_t h : [in] handle to the object to update
 *
 * @return bool : 1 = current task must yield
 **********************************************************************/
bool os_handle_list_updateAndCheck(os_handle_t h);


//////////////////////////////////////////////// ELF //////////////////////////////////////////////////


/***********************************************************************
 * OS ELF load file
 *
 * @brief This function loads an ELF file into memory
 *
 * @param char* name : [ in] File name
 *
 * @return os_elf_prog_t : Necessary information to run a program
 **********************************************************************/
os_elf_prog_t os_elf_loadFile(char* file);


#endif /* INC_OS_OS_INTERNAL_H_ */
