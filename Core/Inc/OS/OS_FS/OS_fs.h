	/*
 * OS_fs.h
 *
 *  Created on: Aug 6, 2022
 *      Author: Gabriel
 */

#ifndef INC_OS_OS_FS_OS_FS_H_
#define INC_OS_OS_FS_OS_FS_H_

#include <stdint.h>
#include <string.h>

/**********************************************
 * PUBLIC TYPES
 *********************************************/

/* Define the OS_FILE type
 ---------------------------------------------------*/
typedef void OS_FILE;

/* Define the seek whence modes
 ---------------------------------------------------*/
typedef enum{
	OS_FS_SEEK_SET = 1,
	OS_FS_SEEK_CUR = 2,
	OS_FS_SEEK_END = 3,
	__OS_FS_SEEK_MAX,
}os_fs_seek_e;

/**********************************************
 * PUBLIC FUNCTIONS
 *********************************************/


/***********************************************************************
 * OS File open
 *
 * @brief This function opens a file using its name
 *
 * @param char* filename : [in] This is the C string containing the name of the file to be opened.
 * @param char* mode 	 : [in] This is the C string containing a file access mode.
 *
 * File modes  :
 * 		- "r"  : Opens a file for reading. The file must exist.
 * 		- "w"  : Creates an empty file for writing. If a file with the same name already exists, its content is erased and the file is considered as a new empty file.
 * 		- "a"  : Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.
 * 		- "r+" : Opens a file to update both reading and writing. The file must exist.
 * 		- "w+" : Creates an empty file for both reading and writing.
 * 		- "a+" : Opens a file for reading and appending.
 *
 * @return OS_FILE* : file pointer or NULL if error
 **********************************************************************/
OS_FILE* os_fopen(const char* filename, const char* mode);


/***********************************************************************
 * OS File close
 *
 * @brief This function closes an opened file
 *
 * @param OS_FILE* fstream	: [in] The file pointer to close
 *
 * @return int : 0 if success
 **********************************************************************/
int os_fclose(OS_FILE* fstream);


/***********************************************************************
 * OS File Read
 *
 * @brief This function reads from an opened file
 *
 * @param void* ptr 		: [out] This is the pointer to a block of memory with a minimum size of size*nmemb bytes.
 * @param size_t size 		: [ in] This is the size in bytes of each element to be read.
 * @param size_t nmemb  	: [ in] This is the number of elements, each one with a size of size bytes.
 * @param OS_FILE* fstream	: [ in] The file pointer to read from
 *
 * @return size_t : number of elements read
 **********************************************************************/
size_t os_fread(void* ptr, size_t size, size_t nmemb, OS_FILE* fstream);


/***********************************************************************
 * OS File write
 *
 * @brief This function write to an opened file
 *
 * @param void* ptr 		: [in] Pointer to the array of elements to be written, converted to a const void*.
 * @param size_t size 		: [in] Size in bytes of each element to be written.
 * @param size_t count  	: [in] Number of elements, each one with a size of size bytes.
 * @param OS_FILE* fstream	: [in] The file pointer to write to
 *
 * @return size_t : number of elements written
 **********************************************************************/
size_t os_fwrite(const void * ptr, size_t size, size_t count, OS_FILE * stream );


/***********************************************************************
 * OS File Seek
 *
 * @brief This function seeks a position in the file
 *
 * @param OS_FILE* fstream		: [in] The file pointer
 * @param int32_t offset 		: [in] This is the number of bytes to offset from whence.
 * @param os_fs_seek_e whence 	: [in] This is the position from where offset is added.
 *
 * @return size_t : 0 if success
 **********************************************************************/
int os_fseek(OS_FILE* fstream, int32_t offset, os_fs_seek_e whence);


#endif /* INC_OS_OS_FS_OS_FS_H_ */
