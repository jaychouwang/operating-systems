/*
	FUSE: Filesystem in Userspace
	Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

	This program can be distributed under the terms of the GNU GPL.
	See the file COPYING.
*/

#define	FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static void cs1550_root(void);
static int cs1550_read_disk(int block, void* buf);
static int cs1550_write_disk(int block, void* buf);
static void cs1550_addFreeBlock(int block);
static int cs1550_getFreeBlock();

//size of a disk block
#define	BLOCK_SIZE 512

//we'll use 8.3 filenames
#define	MAX_FILENAME 8
#define	MAX_EXTENSION 3

//How many files can there be in one directory?
#define MAX_FILES_IN_DIR (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + (MAX_EXTENSION + 1) + sizeof(size_t) + sizeof(long))

//The attribute packed means to not align these things
struct cs1550_directory_entry
{
	int nFiles;	//How many files are in this directory.
				//Needs to be less than MAX_FILES_IN_DIR

	struct cs1550_file_directory
	{
		char fname[MAX_FILENAME + 1];	//filename (plus space for nul)
		char fext[MAX_EXTENSION + 1];	//extension (plus space for nul)
		size_t fsize;					//file size
		long nStartBlock;				//where the first block is on disk
	} __attribute__((packed)) files[MAX_FILES_IN_DIR];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_FILES_IN_DIR * sizeof(struct cs1550_file_directory) - sizeof(int)];
} ;

typedef struct cs1550_root_directory cs1550_root_directory;

#define MAX_DIRS_IN_ROOT (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + sizeof(long))

struct cs1550_root_directory
{
	int nDirectories;	//How many subdirectories are in the root
						//Needs to be less than MAX_DIRS_IN_ROOT
	struct cs1550_directory
	{
		char dname[MAX_FILENAME + 1];	//directory name (plus space for nul)
		long nStartBlock;				//where the directory block is on disk
	} __attribute__((packed)) directories[MAX_DIRS_IN_ROOT];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_DIRS_IN_ROOT * sizeof(struct cs1550_directory) - sizeof(int)];
} ;


typedef struct cs1550_directory_entry cs1550_directory_entry;

//How much data can one block hold?
#define	MAX_DATA_IN_BLOCK (BLOCK_SIZE - sizeof(long))

struct cs1550_disk_block
{
	//The next disk block, if needed. This is the next pointer in the linked 
	//allocation list
	long nNextBlock;

	//And all the rest of the space in the block can be used for actual data
	//storage.
	char data[MAX_DATA_IN_BLOCK];
};

typedef struct cs1550_disk_block cs1550_disk_block;

char allocationTable1[BLOCK_SIZE];
char allocationTable2[BLOCK_SIZE];
cs1550_root_directory* root;

static void cs1550_root(){
	
	char mode[] = "r+b";
	int result;

	FILE* disk = fopen(".disk", mode);

	root = malloc(sizeof(cs1550_root_directory));
	result = fread(root,BLOCK_SIZE,1,disk);
	
	fseek (disk, BLOCK_SIZE, SEEK_SET);
	result = fread(allocationTable1,1,BLOCK_SIZE,disk);
	
	fseek (disk, 2*BLOCK_SIZE, SEEK_SET);
	result = fread(allocationTable2,1,BLOCK_SIZE,disk);
	
	fclose(disk);
	
	if(allocationTable1[0] == 0){
		allocationTable1[0] = 7;
	}
}

static int cs1550_read_disk(int block, void* buf){
	int result;
	char mode[] = "r+b";
	FILE* disk = fopen(".disk", mode);
	fseek (disk, (BLOCK_SIZE*block), SEEK_SET);
	result = fread(buf,BLOCK_SIZE,1,disk);
	fclose(disk);
	return result;

}

static int cs1550_write_disk(int block, void* buf){
	int result;
	char mode[] = "r+b";
	FILE* disk = fopen(".disk", mode);
	fseek (disk, (BLOCK_SIZE*block), SEEK_SET);
	result = fwrite(buf, BLOCK_SIZE, 1, disk);
	fclose(disk);
	return result;
}

static void cs1550_addFreeBlock(int block){
	
	char zeroBytes[BLOCK_SIZE];
	int i;
	int i_index;
	int j_index;
	char MASK = 0xFF;
	char element;
	int flag = 0;
	
	for(i = 0; i < BLOCK_SIZE; i++){
		zeroBytes[i] = 0;
	}
	cs1550_write_disk(block,zeroBytes);
	
	if(block >= (BLOCK_SIZE*8)){
		flag = 1;
		block = block-(BLOCK_SIZE*8);
	}
	
	i_index = block/8;
	j_index = block - (8*i_index);
	
	switch(j_index)
   	{
		case 0:
			MASK = 0xFE;
			break;
	   	case 1:
	   		MASK = 0xFD;
	   	 	break;
	   	case 2:
		  	MASK = 0xFB;
		  	break;
	   	case 3:
		  	MASK = 0xF7;
		  	break;
		case 4:
		  	MASK = 0xEF;
		  	break;
		case 5:
		  	MASK = 0xDF;
		  	break;
		case 6:
		  	MASK = 0xBF;
		  	break;
		  	
		case 7:
		  	MASK = 0x7F;
		  	break;
   }
   
   	if(!flag){
   		element = allocationTable1[i_index];
		allocationTable1[i_index] = element & MASK;
		cs1550_write_disk(1,allocationTable1);
	}
	
	else{
		element = allocationTable2[i_index];
		allocationTable2[i_index] = element & MASK;
		cs1550_write_disk(2,allocationTable2);
	}
	
}

static int cs1550_getFreeBlock(){
	int freeBlock;
	int i,j;
	char element;
	char ONE = 1;
	char MASK = 1;
	int flag = 1;

	for(i = 0; i < BLOCK_SIZE; i++){
		element = allocationTable1[i];
		MASK = 1;
		
		for(j = 0; j < 8; j++){
			flag = (ONE & element);
			if(!flag){
				break;
			}
			
			element = element >> 1;
			MASK = MASK << 1;
		}
	
		if(!flag){
			element = allocationTable1[i];
			allocationTable1[i] = element | MASK;
			freeBlock = (i*8 + j);
			cs1550_write_disk(1,allocationTable1);
			return freeBlock;
		}
	}
	
	for(i = 0; i < BLOCK_SIZE; i++){
		element = allocationTable2[i];
		MASK = 1;
		
		for(j = 0; j < 8; j++){
			flag = (ONE & element);
			if(!flag){
				break;
			}
			
			element = element >> 1;
			MASK = MASK << 1;
		}
	
		if(!flag){
			element = allocationTable2[i];
			allocationTable2[i] = element | MASK;
			freeBlock = (BLOCK_SIZE*8) + (i*8 + j);
			cs1550_write_disk(2,allocationTable2);
			return freeBlock;
		}
	}
	
	return -1;
}

/*
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not. 
 *
 * man -s 2 stat will show the fields of a stat structure
 */
static int cs1550_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	int i;
	int found = 0;
	int nFile;
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	cs1550_directory_entry* subDirectory = malloc(sizeof(cs1550_directory_entry));

	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);

	memset(stbuf, 0, sizeof(struct stat));
   
	//is path the root dir?
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} 
	
	else if(root->nDirectories != 0){

		for(i = 0; i < root->nDirectories; i++){
			if(strcmp(directory,((root->directories[i]).dname)) == 0){
				found = 1;
				cs1550_read_disk((root->directories[i]).nStartBlock,subDirectory);
				break;
			}
		}
	
		//Check if name is subdirectory
		if(found){
		
			if(!strchr(path, '.')){
				//Might want to return a structure with these fields
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				res = 0; //no error
			}
			
			else{
				found = 0;
			
				for(i = 0; i < subDirectory->nFiles; i++){
					if(strcmp(filename,((subDirectory->files[i]).fname)) == 0){
						found = 1;
						nFile = i;
					}
				}
			
				if(found){
					//regular file, probably want to be read and write
					stbuf->st_mode = S_IFREG | 0666; 
					stbuf->st_nlink = 1; //file links
					stbuf->st_size = (subDirectory->files[nFile]).fsize; //file size - make sure you replace with real size!
					res = 0; // no error
				}
				
				else{
					res = -ENOENT;
				}
			}
		
		}
		
		else{
			res = -ENOENT;
		}
	}
	
	//Else return that path doesn't exist
	else{
		res = -ENOENT;
	}
	
	return res;
}

/* 
 * Called whenever the contents of a directory are desired. Could be from an 'ls'
 * or could even be when a user hits TAB to do autocompletion
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	//Since we're building with -Wall (all warnings reported) we need
	//to "use" every parameter, so let's just cast them to void to
	//satisfy the compiler
	(void) offset;
	(void) fi;
	int i;
	int found = 0;
	cs1550_directory_entry* subDirectory = malloc(sizeof(cs1550_directory_entry));
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	struct stat *stbuf = malloc(sizeof(struct stat));
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);

	//This line assumes we have no subdirectories, need to change
	if (strcmp(path, "/") == 0){
		
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		
		for(i = 0; i < root->nDirectories; i++){
			directory = (root->directories[i]).dname;
			filler(buf, directory, NULL, 0);
		}
		
		return 0;
	}
	
	else if(cs1550_getattr(path,stbuf) == 0){
	
		//the filler function allows us to add entries to the listing
		//read the fuse.h file for a description (in the ../include dir)
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		
		
		for(i = 0; i < root->nDirectories; i++){
			if(strcmp(directory,((root->directories[i]).dname)) == 0){
				cs1550_read_disk((root->directories[i]).nStartBlock,subDirectory);
				found = 1;
				break;
			}
		}
		
		if(found){
			for(i = 0; i < subDirectory->nFiles; i++){
				filename = (subDirectory->files[i]).fname;
				filler(buf, filename, NULL, 0);
			}
		}
		
		/*
		//add the user stuff (subdirs or files)
		//the +1 skips the leading '/' on the filenames
		filler(buf, newpath + 1, NULL, 0);
		*/
		return 0;
		
	}
	
	else{
		return -ENOENT;
	}
}

/* 
 * Creates a directory. We can ignore mode since we're not dealing with
 * permissions, as long as getattr returns appropriate ones for us.
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
	(void) mode;
	int i;
	int directoryLength;
	int freeBlock;
	int nSubDirectory;
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	struct stat *stbuf = malloc(sizeof(struct stat));
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
	
	directoryLength = strlen(directory);
	
	if(directoryLength > MAX_FILENAME){
		return -ENAMETOOLONG;
	}
	
	else if(cs1550_getattr(path,stbuf) == 0){
		return -EEXIST;
	}
	
	else{
		
		freeBlock = cs1550_getFreeBlock();
		
		nSubDirectory = root->nDirectories;
		for(i = 0; i < directoryLength; i++){
			root->directories[nSubDirectory].dname[i] = directory[i];
		}
		root->directories[nSubDirectory].dname[directoryLength] = '\0';
		
		root->directories[nSubDirectory].nStartBlock = freeBlock;
		root->nDirectories = nSubDirectory + 1;
		
		cs1550_write_disk(0,root);
		return 0;
	}
	
	return 0;
}

/* 
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{
	(void) path;
    return 0;
}

/* 
 * Does the actual creation of a file. Mode and dev can be ignored.
 *
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
	(void) mode;
	(void) dev;
	int i;
	int filenameLength;
	int extensionLength;
	int freeBlock;
	int found = 0;
	int subDirBlock;
	int nFile;
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	struct stat *stbuf = malloc(sizeof(struct stat));
	cs1550_directory_entry* subDirectory = malloc(sizeof(cs1550_directory_entry));
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
	
	if (directory == NULL || directory[0] == '\0'){
		return -EPERM;
	}
	
	filenameLength = strlen(filename);
	extensionLength = strlen(extension);
	
	if(filenameLength > MAX_FILENAME || extensionLength > MAX_EXTENSION){
		return -ENAMETOOLONG;
	}
	
	if(cs1550_getattr(path,stbuf) == 0){
		return -EEXIST;
	}
	
	for(i = 0; i < root->nDirectories; i++){
		if(strcmp(directory,(root->directories[i]).dname) == 0){
			cs1550_read_disk((root->directories[i]).nStartBlock,subDirectory);
			subDirBlock = (root->directories[i]).nStartBlock;
			found = 1;
			break;
		}
	}
		
	if(found){
	
		for(i = 0; i < subDirectory->nFiles; i++){
			if(strcmp(filename,(subDirectory->files[i]).fname) == 0){
				return -EEXIST;
			}
		}

		freeBlock = cs1550_getFreeBlock();
		
		nFile = subDirectory->nFiles;
		subDirectory->nFiles = nFile+1;
		
		subDirectory->files[nFile].fsize = 0;
		subDirectory->files[nFile].nStartBlock = freeBlock;
		
		for(i = 0; i < filenameLength; i++){
			subDirectory->files[nFile].fname[i] = filename[i];
		}
		subDirectory->files[nFile].fname[filenameLength] = '\0';
		
		for(i = 0; i < extensionLength; i++){
			subDirectory->files[nFile].fext[i] = extension[i];
		}
		subDirectory->files[nFile].fext[extensionLength] = '\0';
				
		cs1550_write_disk(subDirBlock,subDirectory);
		cs1550_write_disk(0,root);
		
		return 0;	
	}
	
	else{
		return -ENOENT;
	}
	
	
	return 0;
}

/*
 * Deletes a file
 */
static int cs1550_unlink(const char *path)
{
    
    int found = 0;
    int subDirBlock;
	int fileBlock;
	int nFile;
	int i;
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	struct stat *stbuf = malloc(sizeof(struct stat));
	cs1550_directory_entry* subDirectory = malloc(sizeof(cs1550_directory_entry));
	cs1550_disk_block* theFile = malloc(sizeof(cs1550_disk_block));
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
    
    //check to make sure path exists
	if(cs1550_getattr(path,stbuf) != 0){
		return -ENOENT;
	}
	
	for(i = 0; i < root->nDirectories; i++){
		if(strcmp(directory,(root->directories[i]).dname) == 0){
			cs1550_read_disk((root->directories[i]).nStartBlock,subDirectory);
			subDirBlock = (root->directories[i]).nStartBlock;
			found = 1;
			break;
		}
	}
	
	if(!found){
		return -ENOENT;
	}
	
	found = 0;
	for(i = 0; i < subDirectory->nFiles; i++){
		if(strcmp(filename,(subDirectory->files[i]).fname) == 0){
			cs1550_read_disk((subDirectory->files[i]).nStartBlock,theFile);
			fileBlock = (subDirectory->files[i]).nStartBlock;
			cs1550_addFreeBlock(fileBlock);
			nFile = i;
			found = 1;
			break;
		}
	}
	
	if(!found){
		return -EISDIR;
	}
	
	while(theFile->nNextBlock != 0){
		fileBlock = theFile->nNextBlock;
		cs1550_addFreeBlock(fileBlock);
		cs1550_read_disk(fileBlock,theFile);
	}
	
	for (i = nFile; i < subDirectory->nFiles; i++){
    	subDirectory->files[i] = subDirectory->files[i+1];	
    }
    
    subDirectory->nFiles = subDirectory->nFiles-1;
    
    cs1550_write_disk(subDirBlock,subDirectory);
	cs1550_write_disk(0,root);
	
    return 0;
}

/* 
 * Read size bytes from file into buf starting from offset
 *
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{
	(void) fi;
	int i,j;
	int found = 0;
	int subDirBlock;
	int fileBlock;
	int fileSize;
	int loop = 0;
	int newOffset;
	int nFile;
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	struct stat *stbuf = malloc(sizeof(struct stat));
	cs1550_directory_entry* subDirectory = malloc(sizeof(cs1550_directory_entry));
	cs1550_disk_block* theFile = malloc(sizeof(cs1550_disk_block));
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);

	//check to make sure path exists
	if(cs1550_getattr(path,stbuf) != 0){
		return -ENOENT;
	}
	
	//check that size is > 0
	if(size <= 0){
		return -EPERM;
	}
	
	for(i = 0; i < root->nDirectories; i++){
		if(strcmp(directory,(root->directories[i]).dname) == 0){
			cs1550_read_disk((root->directories[i]).nStartBlock,subDirectory);
			subDirBlock = (root->directories[i]).nStartBlock;
			found = 1;
			break;
		}
	}
	
	if(!found){
		return -ENOENT;
	}
	
	found = 0;
	for(i = 0; i < subDirectory->nFiles; i++){
		if(strcmp(filename,(subDirectory->files[i]).fname) == 0){
			cs1550_read_disk((subDirectory->files[i]).nStartBlock,theFile);
			fileBlock = (subDirectory->files[i]).nStartBlock;
			fileSize = (subDirectory->files[i]).fsize;
			nFile = i;
			found = 1;
			break;
		}
	}
	
	if(!found){
		return -EISDIR;
	}
	
	//check that offset is <= to the file size
	if(offset > fileSize){
		return -EFBIG;
	}
	
	//check if offset is in another file block
	if(offset >= MAX_DATA_IN_BLOCK){
		loop = offset / MAX_DATA_IN_BLOCK;
		newOffset = offset;
		
		for(i = 0; i < loop; i++){
			fileBlock = theFile->nNextBlock;
			cs1550_read_disk(theFile->nNextBlock,theFile);
			newOffset = newOffset - MAX_DATA_IN_BLOCK;
		}
		
		offset = newOffset;
	}
	
	//read in data
	for(i = offset, j = 0; j < size; i++, j++){
		buf[j] = theFile->data[i];
		
		if(offset >= MAX_DATA_IN_BLOCK && theFile->nNextBlock != 0){
			fileBlock = theFile->nNextBlock;
			cs1550_read_disk(fileBlock,theFile);
			offset = 0;
		}
	}
	
	return size;
}

/* 
 * Write size bytes from buf into file starting from offset
 *
 */
static int cs1550_write(const char *path, const char *buf, size_t size, 
			  off_t offset, struct fuse_file_info *fi)
{
	(void) fi;
	int i,j;
	int found = 0;
	int subDirBlock;
	int fileBlock;
	int freeBlock;
	int fileSize;
	int loop = 0;
	int newOffset;
	int nFile;
	char* directory = malloc(MAX_FILENAME + 1);
	char* filename = malloc(MAX_FILENAME + 1);
	char* extension = malloc(MAX_EXTENSION + 1);
	struct stat *stbuf = malloc(sizeof(struct stat));
	cs1550_directory_entry* subDirectory = malloc(sizeof(cs1550_directory_entry));
	cs1550_disk_block* theFile = malloc(sizeof(cs1550_disk_block));
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
	
	//check to make sure path exists
	if(cs1550_getattr(path,stbuf) != 0){
		return -ENOENT;
	}

	//check that size is > 0
	if(size <= 0){
		return -EPERM;
	}
	
	for(i = 0; i < root->nDirectories; i++){
		if(strcmp(directory,(root->directories[i]).dname) == 0){
			cs1550_read_disk((root->directories[i]).nStartBlock,subDirectory);
			subDirBlock = (root->directories[i]).nStartBlock;
			found = 1;
			break;
		}
	}
	
	if(!found){
		return -ENOENT;
	}
		
	found = 0;
	for(i = 0; i < subDirectory->nFiles; i++){
		if(strcmp(filename,(subDirectory->files[i]).fname) == 0){
			cs1550_read_disk((subDirectory->files[i]).nStartBlock,theFile);
			fileBlock = (subDirectory->files[i]).nStartBlock;
			fileSize = (subDirectory->files[i]).fsize;
			nFile = i;
			found = 1;
			break;
		}
	}
	
	if(!found){
		return -ENOENT;
	}
	
	//check that offset is <= to the file size
	if(offset > fileSize){
		return -EFBIG;
	}
	
	//check if offset is in another file block
	if(offset >= MAX_DATA_IN_BLOCK){
		loop = offset / MAX_DATA_IN_BLOCK;
		newOffset = offset;
		
		for(i = 0; i < loop; i++){
			fileBlock = theFile->nNextBlock;
			cs1550_read_disk(theFile->nNextBlock,theFile);
			newOffset = newOffset - MAX_DATA_IN_BLOCK;
		}
		
		offset = newOffset;
	}
	
	
	//write data
	for(i = offset, j = 0; j < size; i++, j++){
		theFile->data[i] = buf[j];
		
		if(offset >= MAX_DATA_IN_BLOCK && theFile->nNextBlock == 0){
			freeBlock = cs1550_getFreeBlock();
			theFile->nNextBlock = freeBlock;
			cs1550_write_disk(fileBlock,theFile);
			fileBlock = freeBlock;
			cs1550_read_disk(fileBlock,theFile);
			offset = 0;
		}
		
		else if(offset >= MAX_DATA_IN_BLOCK && theFile->nNextBlock != 0){
			cs1550_write_disk(fileBlock,theFile);
			fileBlock = theFile->nNextBlock;
			cs1550_read_disk(fileBlock,theFile);
			offset = 0;
		}
	}
	
	cs1550_write_disk(fileBlock,theFile);

	subDirectory->files[nFile].fsize = (fileSize+size);

	cs1550_write_disk(subDirBlock,subDirectory);
	cs1550_write_disk(0,root);
	
	//set size (should be same as input) and return, or error
	return size;
	
}

/******************************************************************************
 *
 *  DO NOT MODIFY ANYTHING BELOW THIS LINE
 *
 *****************************************************************************/

/*
 * truncate is called when a new file is created (with a 0 size) or when an
 * existing file is made shorter. We're not handling deleting files or 
 * truncating existing ones, so all we need to do here is to initialize
 * the appropriate directory entry.
 *
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;
 
    return 0;
}


/* 
 * Called when we open a file
 *
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;
    /*
        //if we can't find the desired file, return an error
        return -ENOENT;
    */

    //It's not really necessary for this project to anything in open

    /* We're not going to worry about permissions for this project, but 
	   if we were and we don't have them to the file we should return an error

        return -EACCES;
    */

    return 0; //success!
}

/*
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file 
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush (const char *path , struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;

	return 0; //success!
}


//register our new functions as the implementations of the syscalls
static struct fuse_operations hello_oper = {
    .getattr	= cs1550_getattr,
    .readdir	= cs1550_readdir,
    .mkdir	= cs1550_mkdir,
	.rmdir = cs1550_rmdir,
    .read	= cs1550_read,
    .write	= cs1550_write,
	.mknod	= cs1550_mknod,
	.unlink = cs1550_unlink,
	.truncate = cs1550_truncate,
	.flush = cs1550_flush,
	.open	= cs1550_open,
};

//Don't change this.
int main(int argc, char *argv[])
{
	cs1550_root();
	return fuse_main(argc, argv, &hello_oper, NULL);
}
