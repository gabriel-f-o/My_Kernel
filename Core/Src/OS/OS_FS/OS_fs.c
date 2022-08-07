/*
 * os_fs.c
 *
 *  Created on: Aug 5, 2022
 *      Author: Gabriel
 */


#include "OS/OS_FS/OS_fs.h"
#include "OS/OS_FS/lfs.h"

#include "OS/OS_Core/OS.h"

/**********************************************
 * EXTERNAL VARIABLES
 *********************************************/

extern os_handle_t fsMutex;

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
OS_FILE* os_fopen(const char* filename, const char* mode){

	/* Check arguments
	 ------------------------------------------------------*/
	if(mode == NULL) return NULL;
	if(filename == NULL) return NULL;
	if(strlen(mode) > 2) return NULL;

	/* Define flags
	 ------------------------------------------------------*/
	int flags = 0;
	switch(mode[0]){
		case 'r' : flags |= LFS_O_RDONLY; break;
		case 'w' : flags |= LFS_O_CREAT | LFS_O_WRONLY | LFS_O_TRUNC ; break;
		case 'a' : flags |= LFS_O_CREAT | LFS_O_WRONLY | LFS_O_APPEND; break;
		default  : return NULL;
	}

	if(mode[1] == '+'){
		flags |= LFS_O_RDWR;
	}
	else if(mode[1] != '\0'){
		return NULL;
	}

	/* Get FS mutex
	 ------------------------------------------------------*/
	if(os_obj_single_wait(fsMutex, OS_WAIT_FOREVER, NULL) == NULL)
		return NULL;

	/* Allocate file pointer
	 ------------------------------------------------------*/
	OS_FILE* f = os_heap_alloc(sizeof(lfs_file_t));
	if(f == NULL)
		return NULL;

	/* Open file
	 ------------------------------------------------------*/
	int err = lfs_file_open(&lfs, (lfs_file_t*)f, filename, flags);
	if(err < 0){
		os_heap_free(f);
		return NULL;
	}

	/* Release mutex
	 ------------------------------------------------------*/
	if(os_mutex_release(fsMutex) != OS_ERR_OK){
		os_heap_free(f);
		return NULL;
	}


	return f;
}


/***********************************************************************
 * OS File close
 *
 * @brief This function closes an opened file
 *
 * @param OS_FILE* fstream	: [in] The file pointer to close
 *
 * @return int : 0 if success
 **********************************************************************/
int os_fclose(OS_FILE* fstream){

	/* Check arguments
	 ------------------------------------------------------*/
	if(fstream == NULL) return OS_ERR_BAD_ARG;

	/* Get FS mutex
	 ------------------------------------------------------*/
	os_err_e err;
	os_obj_single_wait(fsMutex, OS_WAIT_FOREVER, &err);
	if(err != OS_ERR_OK)
		return err;

	/* Close file
	 ------------------------------------------------------*/
	int fserr = lfs_file_close(&lfs, (lfs_file_t*) fstream);

	/* Release mutex
	 ------------------------------------------------------*/
	err = os_mutex_release(fsMutex);

	/* Free file pointer
	 ------------------------------------------------------*/
	os_err_e errh = os_heap_free(fstream);

	/* Return error
	 ------------------------------------------------------*/
	if(err != OS_ERR_OK)
		return err;

	/* Return error
	 ------------------------------------------------------*/
	if(errh != OS_ERR_OK)
		return errh;

	/* Return 0 if OK
	 ------------------------------------------------------*/
	return fserr < 0 ? OS_ERR_FS : 0;
}


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
size_t os_fread(void* ptr, size_t size, size_t nmemb, OS_FILE* fstream){

	/* Check arguments
	 ------------------------------------------------------*/
	if(ptr == NULL) return 0;
	if(size == 0) return 0;
	if(nmemb == 0) return 0;
	if(fstream == NULL) return 0;

	/* Get FS mutex
	 ------------------------------------------------------*/
	if(os_obj_single_wait(fsMutex, OS_WAIT_FOREVER, NULL) == NULL)
		return 0;

	/* read file
	 ------------------------------------------------------*/
	int fserr = lfs_file_read(&lfs, (lfs_file_t*)fstream, ptr, size * nmemb);

	/* Release mutex
	 ------------------------------------------------------*/
	os_err_e err = os_mutex_release(fsMutex);
	if(err != OS_ERR_OK)
		return 0;

	return (fserr < 0) ? 0 : ((size_t)fserr / (size_t)size);
}


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
size_t os_fwrite(const void * ptr, size_t size, size_t count, OS_FILE * fstream){

	/* Check arguments
	 ------------------------------------------------------*/
	if(ptr == NULL) return 0;
	if(size == 0) return 0;
	if(count == 0) return 0;
	if(fstream == NULL) return 0;

	/* Get FS mutex
	 ------------------------------------------------------*/
	if(os_obj_single_wait(fsMutex, OS_WAIT_FOREVER, NULL) == NULL)
		return 0;

	/* write file
	 ------------------------------------------------------*/
	int fserr = lfs_file_write(&lfs, (lfs_file_t*)fstream, ptr, size * count);

	/* Release mutex
	 ------------------------------------------------------*/
	os_err_e err = os_mutex_release(fsMutex);
	if(err != OS_ERR_OK)
		return 0;

	return fserr < 0 ? 0 : ((size_t)fserr / (size_t)size);
}


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
int os_fseek(OS_FILE *fstream, int32_t offset, os_fs_seek_e whence){

	/* Check arguments
	 ------------------------------------------------------*/
	if(fstream == NULL) return OS_ERR_BAD_ARG;
	if(whence == 0) return OS_ERR_BAD_ARG;
	if(whence >= __OS_FS_SEEK_MAX) return OS_ERR_BAD_ARG;

	/* Get FS mutex
	 ------------------------------------------------------*/
	os_err_e err;
	os_obj_single_wait(fsMutex, OS_WAIT_FOREVER, &err);
	if(err != OS_ERR_OK)
		return err;

	/* Translate whence enum
	 ------------------------------------------------------*/
	int lfs_whence = -1;
	switch(whence){
		case OS_FS_SEEK_CUR : lfs_whence = LFS_SEEK_CUR; break;
		case OS_FS_SEEK_END : lfs_whence = LFS_SEEK_END; break;
		case OS_FS_SEEK_SET : lfs_whence = LFS_SEEK_SET; break;
		case __OS_FS_SEEK_MAX :
		default : break;
	}

	/* Seek position
	 ------------------------------------------------------*/
	lfs_soff_t fserr = lfs_file_seek(&lfs, (lfs_file_t*) fstream, (lfs_soff_t)offset, lfs_whence);

	/* Release mutex
	 ------------------------------------------------------*/
	err = os_mutex_release(fsMutex);
	if(err != OS_ERR_OK)
		return err;

	/* Return
	 ------------------------------------------------------*/
	return fserr < 0 ? OS_ERR_FS : 0;
}
