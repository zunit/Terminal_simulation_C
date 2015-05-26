#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/errno.h>
#include "ext2.h"
#include "helper.h"

unsigned char *disk;

int dblock_name(char* name, int *data_block, int size_block);
int location_dir_entry(int block, char*name);
int rec_len_dirblock(int location, int block);
void rec_len_change(int location, int block, unsigned short value);
int found_name(char*name, int blocknum, int inum);
unsigned int explore_inode(const char *absolute_path);
unsigned int explore_delete_inode(const char *absolute_path);
int found_delete_name(char*name, int blocknum, int inum);
int found_delete_path(char*name, int blocknum, int inum);
int size_data_indirect(int blocknum);
int *create_ind_dlist(int blocknum, int size);
int *create_first_dblock(void *tablelocation, int i, int length_block);

int main(int argc, char const *argv[])
{
	if(argc != 3) {
        fprintf(stderr, "Usage: ext2_rm <image file name> <path>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
    }
    // creating target name
    char dup_path[strlen(argv[2]) + 1];
	strcpy(dup_path, argv[2]);	
	const char ch = '/';
	char *ret;
	ret = strrchr(dup_path, ch);
	char*target_name;
	target_name = ret + 1; // this is the name we want to remove
    
    // get the group descriptor table
    // we know that this table always start at third datablock (index 2)
    struct ext2_group_desc *group_des_table = (struct ext2_group_desc*)(disk + EXT2_BLOCK_SIZE * 2);
    int inode_bitmap = group_des_table->bg_inode_bitmap;
    int block_bitmap = group_des_table->bg_block_bitmap;
    int inode_table = group_des_table->bg_inode_table;
    void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;
    //root check 
    int rootcheck = strcmp(argv[2],"/");
    if (rootcheck == 0){
		fprintf(stderr, "Operation not permitted\n");
		exit(EPERM);
	}
	
	if (argv[2][0] != '/'){
		fprintf(stderr, "No such file or directory\n");
		exit(ENOENT);
	}

    int found_inode;
    found_inode = explore_delete_inode(argv[2]); // returns the inode_number to del
    if (found_inode == -1){
		fprintf(stderr, "No such file or directory\n");
		exit(ENOENT);
	} 
	if (found_inode == 0){ // can't delete a directory
		fprintf(stderr, "No such file or directory\n");
		exit(ENOENT);
	}
    
    int parent_inodenum = parent_conversion(argv[2]); 
    
    // create the parent innode
    struct ext2_inode *parent_inode = (struct ext2_inode*)(tablestart)+(parent_inodenum -1);
    int size_dblock = size_data_block(parent_inode->i_block);
    int *data_block = (int*)malloc(size_dblock*sizeof(int));
	data_block = create_data_block(tablestart, parent_inodenum, size_dblock);

    
    // get the inode of the parent 
    // alter the rec_len
    int target_dblock;
    // this returns the block that the file data is stored in. 
    // gets all the information from parent block
    target_dblock = dblock_name(target_name, data_block, size_dblock);
    if (target_dblock == -1){ // this isn't a file!
		fprintf(stderr, "No such file or directory\n");
		exit(ENOENT);
	}
    // findout location 
    int location; // index start at 0
    location = location_dir_entry(target_dblock, target_name);

    // merge the file with previous in the target data block 
    unsigned short target_reclen = rec_len_dirblock(location, target_dblock);
    rec_len_change(location, target_dblock, target_reclen);
    

    // mark the data bitmap to zero
    struct ext2_inode *target_inode = (struct ext2_inode*)(tablestart)+(found_inode -1);
    int target_size =  size_data_block(target_inode->i_block);
    int *tar_block = (int*)malloc(target_size*sizeof(int));
    tar_block = create_data_block(tablestart, found_inode, target_size);
    if(tar_block[0] == 0){ // there exist a indirect
		// FIRST LEVEL BLOCK -----------------------
		int *first_level_block = (int*)malloc(12*sizeof(int));
		first_level_block = create_first_dblock(tablestart, found_inode, 12);
		
		// DEEP BLOCK ------------------------------
		int size_of_id = size_data_indirect(target_inode->i_block[12]);
		int *ind_block_list = (int*)malloc(size_of_id*sizeof(int));
		ind_block_list = create_ind_dlist(target_inode->i_block[12], size_of_id);
		
		int *beg_data_bitmap = (int*)(disk + EXT2_BLOCK_SIZE * block_bitmap);
		// setting first level data
		int t;
		for (t = 0; t < 12; t++){
			setbitmap(beg_data_bitmap, 0, first_level_block[t]);
			// increment free data block 
			group_des_table->bg_free_blocks_count += 1;
		}
		//setting the last block 
		setbitmap(beg_data_bitmap, 0, target_inode->i_block[12]);
		group_des_table->bg_free_blocks_count += 1;
		// setting second level data
		int s;
		for (s = 0; s < size_of_id; s++){
			setbitmap(beg_data_bitmap, 0, ind_block_list[s]);
			// increment free data block 
			group_des_table->bg_free_blocks_count += 1;
		}
		
		int *beg_inode_bitmap = (int*)(disk + EXT2_BLOCK_SIZE * inode_bitmap);
		setbitmap(beg_inode_bitmap, 0, found_inode);
		group_des_table->bg_free_inodes_count += 1;
		return 0;
	}

    int *beg_data_bitmap = (int*)(disk + EXT2_BLOCK_SIZE * block_bitmap);
    int i;
    for (i = 0; i < target_size; i++){

		setbitmap(beg_data_bitmap, 0, tar_block[i]);
		// increment free data block 
		group_des_table->bg_free_blocks_count += 1;
	}
    // mark the inode bitmap zero 
	int *beg_inode_bitmap_nod = (int*)(disk + EXT2_BLOCK_SIZE * inode_bitmap);
	setbitmap(beg_inode_bitmap_nod, 0, found_inode);
	// increment free inodes 
    group_des_table->bg_free_inodes_count += 1;
    
	// free all the malloc to prevent memory leak
	free(tar_block);
	free(data_block);
	return 0;
}


/* 
* helper for returning a list of data blocks 
* looks at the inode table and length of data block, return the array
* input: tablelocation - base location of inode table
* input: i - offset into the base table   
* input: length_block - length of the supposed data block array
* output: array of data block
*/                                                                                                                               
int *create_first_dblock(void *tablelocation, int i, int length_block){
    struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
    int k = 0;
    int *array =(int*)malloc(length_block*sizeof(int));
	while((inode->i_block[k]) && (k < 13)){
		array[k] = inode->i_block[k];
		k++;
	}
    return array;
}


/*
 * input blocknum - block number to use
 * input size - size of the array
 */
int *create_ind_dlist(int blocknum, int size){
	//int total = 1024/8; // total array size stored in a block for indir
	int i = 0;
	int *pointer = (int*)(disk + 1024 * blocknum);
	int *array = (int*)malloc(size*sizeof(int));
	while (pointer[i]){
		array[i] = pointer[i];
		i++;
	}
	return array;
}

/*
 * returns the size of the indirect block 
 * input - blocknum is the blocknumber that has the array of ints of large file
 */
int size_data_indirect(int blocknum){

	int total = 1024/8; // total array size stored in a block for indir
	int *pointer = (int*)malloc(total*sizeof(int));
	pointer = (int*)(disk + 1024 * blocknum);
	int i;
	int counter = 0;
	
	for (i = 0; i < total; i++){
		if (pointer[i]){
			counter++;
		}
	}
	return counter;
}

// return datablock that the file is in
// input datablock - set of datablock we look at
// input name - name of the directory or file that we searching for
// input size_block - length of the int array
int dblock_name(char* name, int *data_block, int size_block){
	int i;
	for (i = 0; i < size_block; i++){
		int end = EXT2_BLOCK_SIZE; // each block is 1024 so keep print from block 
		int current = 0; // beg of block
		
		while (current < end){ // traversing through all the d_blocks
			struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * data_block[i]) + current);
			unsigned short dir_len = dir_entry->rec_len;
			int name_length = dir_entry->name_len;
			char ftype = filetypecheck(dir_entry->file_type); //file type
			char *fdirname; // need reformatt the filename
			fdirname = create_fdir_name(dir_entry->name, name_length);
			int cmp_val = strncmp(name, fdirname, name_length);
			if((cmp_val == 0) && (ftype == 'f')){ // make sure its a file
				return data_block[i];
			}
			current += (int)dir_len; 	
		}	
	}
	return -1; // we didn't find anything =(
}

// return location of where 
int location_dir_entry(int block, char*name){
	int end = EXT2_BLOCK_SIZE; // each block is 1024 so keep print from block 
	int current = 0;
	int counter = 0;
	while (current < end){ // traversing through all the d_blocks
		struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * block) + current);
		unsigned short dir_len = dir_entry->rec_len;
		int name_length = dir_entry->name_len;
		char *fdirname; // need reformatt the filename
		fdirname = create_fdir_name(dir_entry->name, name_length);
		int cmp_val = strncmp(name, fdirname, name_length);
		if (cmp_val == 0){
			return counter;
		}
		counter++;
		current += (int)dir_len; 	
	}
	return -1; // we didn't find it
}

// returns the rec len of the specified location directory entry
int rec_len_dirblock(int location, int block){
	int end = EXT2_BLOCK_SIZE; // each block is 1024 so keep print from block 
	int current = 0;
	int counter = 0;
	while (current < end){ // traversing through all the d_blocks
		struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * block) + current);
		unsigned short dir_len = dir_entry->rec_len;
		if (counter == (location)){
			return dir_len;
		}
		counter++;
		current += (int)dir_len; 	
	}
	return -1; // we didn't find it
}
	
// alter rec len of specified location 
// changed rec len for the previous block
/*
 * input location is the location of the target we wish to remove
 * input block is the target block 
 * input value is the value of rec_len we want to add to previous 
 * 
 * */
void rec_len_change(int location, int block, unsigned short value){
	int end = EXT2_BLOCK_SIZE; // each block is 1024 so keep print from block 
	int current = 0;
	int counter = 0;
	while (current < end){ // traversing through all the d_blocks
		struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * block) + current);
		unsigned short dir_len = dir_entry->rec_len;
		if (counter == (location-1)){
			dir_entry->rec_len = dir_entry->rec_len + value;
		}
		counter++;
		current += (int)dir_len; 	
	}
}

unsigned int explore_delete_inode(const char *absolute_path){

    // bitmap and inode table
    // located from the group_descriptor_table [2 nd block] 
    struct ext2_group_desc *group_desc_table = (struct ext2_group_desc *)(disk + (EXT2_BLOCK_SIZE * 2));
    // location of the bitmaps in blocks 
    int inode_table = group_desc_table->bg_inode_table;
    
    // start from the root inode
    // find out where the inode tale start
    void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;

    // need to create a duplicate path so we don't change original
    char dup_path[strlen(absolute_path) + 1];
    strcpy(dup_path, absolute_path);
    char separator[2] = "/"; // we have to turn this into a string
    char *member = strtok(dup_path, separator);

    // check if first one is "/" we only want to look at is only root
    // special root file may have more than one block 
    if ((dup_path[0] == '/') && (strlen(dup_path) == 2)){
        return 2; // if it is root only then just return 2
    } 
    else if(dup_path[0] != '/'){ // when the path is invalid 
        return -1;
    }

    int foundnum = 2;
    while(member != NULL){ // loop through each member file name
        // creating the inode
        // foundnum - 1 is to go to beginning of inode
        struct ext2_inode *inode = (struct ext2_inode*)(tablestart)+(foundnum-1);
        int size_dblock = size_data_block(inode->i_block);
        int *data_block = (int*)malloc(size_dblock*sizeof(int));
        data_block = create_data_block(tablestart, foundnum, size_dblock);

        char *searchfile = member; // file name

        int j;
        int temp;
        for (j = 0; j < size_dblock; j++){
            temp = -1; // clear temp
            temp = found_delete_path(searchfile, data_block[j], foundnum);
            if (temp == -1){
                fprintf(stderr, "No such file or directory\n");
                exit(ENOENT);
            }
            foundnum = temp;
        }
        free(data_block); // clear the allocated block 
        member = strtok(NULL, separator); // iterater through the loop
    }
    return foundnum;
}

// helper function 
/* Input : 
 *   name - the name we are looking for 
 *   blocknum - data block we are looking into
 * Output :
 *   int value - a mathed name and the inode number
 * */
int found_delete_name(char*name, int blocknum, int inum){
    // always set current to 0 when we start searching a block
    int current = 0; 
    int end = 1024; // end of block 
    unsigned int matched; // this is the inode that is returned
    while (current < end){
        // start the direct entry at where it is printing
        struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (EXT2_BLOCK_SIZE * blocknum) + current);

        // get the inode number that this directory data is in
        int inode_num = dir_entry->inode;
        unsigned short dir_len = dir_entry->rec_len; // incrementer
        int name_length = dir_entry->name_len; 
        char file_flag = filetypecheck(dir_entry->file_type); // d or f
        char *fdirname;
        fdirname = create_fdir_name(dir_entry->name, name_length);
        current += (int)dir_len; // increment the rec_len
        // if there is a name that match 
        int str_compare_val = strcmp(name,fdirname);
        // condition: if match and its a directory
        if ((str_compare_val == 0) && (file_flag == 'd')){
			return 0; // we found a valid directory
        } 
        else if ((str_compare_val == 0) && (file_flag == 'f')){
            matched = inode_num;
            return matched;
        } 
        else {
                matched = -1; // didn't find anything yet
        }
    }
    if (matched == -1){
        return -1; // exit the function we didn't find anything
    } 
    return matched; 
}

// helper function 
// returns the proper path to dele the file
/* Input : 
 *   name - the name we are looking for 
 *   blocknum - data block we are looking into
 * Output :
 *   int value - a mathed name and the inode number
 * */
int found_delete_path(char*name, int blocknum, int inum){
    // always set current to 0 when we start searching a block
    int current = 0; 
    int end = 1024; // end of block 
    unsigned int matched; // this is the inode that is returned
    while (current < end){
        // start the direct entry at where it is printing
        struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (EXT2_BLOCK_SIZE * blocknum) + current);

        // get the inode number that this directory data is in
        int inode_num = dir_entry->inode;
        unsigned short dir_len = dir_entry->rec_len; // incrementer
        int name_length = dir_entry->name_len; 
        char file_flag = filetypecheck(dir_entry->file_type); // d or f
        char *fdirname;
        fdirname = create_fdir_name(dir_entry->name, name_length);
        current += (int)dir_len; // increment the rec_len
        // if there is a name that match 
        int str_compare_val = strcmp(name,fdirname);
        // condition: if match and its a directory
        if ((str_compare_val == 0) && (file_flag == 'd')){
        	matched = inode_num;
            return matched;
        } 
        else if ((str_compare_val == 0) && (file_flag == 'f')){
            matched = inode_num;
            return matched;
        } 
        else {
                matched = -1; // didn't find anything yet
        }
    }
    if (matched == -1){
        return -1; // exit the function we didn't find anything
    } 
    return matched; 
}

