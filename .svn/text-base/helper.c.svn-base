#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include "ext2.h"
#include "helper.h"
#include "disk.h"

/*
* traverses inode bitmap to find a free inode
* marks that inode bit as used
* returns the index of the inode
*/
int get_free_inode(void * inode_map){
    int *map = inode_map;
    int i = 11;
    int indx = -1;
    for (i = 11; i < 32; ){
        int temp = *map;
        if (temp & (1 << i)){
            i++;
        }
        else{
            //this is the free inode number
            indx = i;
            *(int*)inode_map |= 1 << i;
            break;
        }
    }
    return indx;
}

/*
* Traverses the block bitmap to find a free block
* marks that block bit as used
* returns the block number of the free block
*/
int get_free_block(void * start){
    int i;
    int free_blk = -1;
    for (i = 0; i < 16; i++){
        free_blk = check_free_bit(start + i);
        if (free_blk != -1){
            setbitmap(start, 1, i * 8 + free_blk);
            return (i * 8 + free_blk);
        }
    }
    return free_blk;
}

//checks whether the bit is free; returns the index of free bit
int check_free_bit(void * *bytes){

    unsigned char value = (unsigned char)*bytes;
    //char temp;
    //temp = value & 1 ? '1' : '0';
    if ((value & 1) == 0){
        return 1;
    }
    else if ((value & 2) == 0){
        return 2;
    }
    else if ((value & 4) == 0){
        return 3;
    }
    else if ((value & 5) == 0){
        return 4;
    }
    else if ((value & 16) == 0){
        return 5;
    }
    else if ((value & 32) == 0){
        return 6;
    }
    else if ((value & 64) == 0){
        return 7;
    }
    else if ((value & 128) == 0){
        return 8;
    }
    return -1;
}

/* input - bitmap pointer to the bitmap
 * input - set 0 or 1 value we want to set to. 
 * input - location of where we want to change the bit
 * void - but the bitmap is altered
 */
void setbitmap(void *bitmap, int set, int location){
	int *begin = bitmap;
	location = location - 1;
	int offset = location % 32;
	int counter = location / 32; 
	begin = begin + counter;
	if (set == 1){
		*begin |= 1 << offset;
	} 
    else { // setting it to 0
		*begin &= ~(1 << offset);
	}
}

// function returns the parent inode
// input - the target absolute that we want the parent
// output the inode number of the parent
unsigned int parent_conversion(const char *absolute_path){
	// string manipulation to get the parent path
	char dup_path[strlen(absolute_path) + 1];
	strcpy(dup_path, absolute_path);
	const char ch = '/';
	char *ret;
	ret = strrchr(dup_path, ch);
	char*res;
	res = ret;
	char parentpath[1024];
	if (strlen(dup_path)-strlen(res) == 0){ // when root is parent
		strcpy(parentpath, "/");
	} else {
		strncpy(parentpath, dup_path, strlen(dup_path)-strlen(res));
	}
	return search_inode(parentpath);	
}

// find a inode with specified path 
// 1. parse the path 
// 2. get the name and search it in that inode
// 3. looping
unsigned int search_inode(const char *absolute_path){

    // bitmap and inode table
    // located from the group_descriptor_table [2 nd block] 
    struct ext2_group_desc *group_desc_table = (struct ext2_group_desc *)
                                                (disk + (EXT2_BLOCK_SIZE * 2));

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
    if ((dup_path[0] == '/') && (strlen(dup_path) == 2)){
        return 2; // if it is root only then just return 2
    } 
    else if(dup_path[0] != '/'){
	    // when the path is invalid 
        return -1;
        exit(1);
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
            temp = matched_name(searchfile, data_block[j], foundnum);
            foundnum = temp;
        }
        if (foundnum == -1){
            break;
        }
        free(data_block); // clear the allocated block 
        member = strtok(NULL, separator); // iterater through the loop
    }
    return foundnum;
}

// helper for returning the size of data_block                                                                                                                
// when looking at a valid inode we check how many data block it point                                                                                        
// input: tablelocation - the location of inode table                                                                                                         
// input: i - how far down offset into the table we should point to                                                                                           
unsigned int size_data_block(unsigned int *i_block){
    int k = 0;
    while(i_block[k]){
        k++;
    }
    return k;
}

// helper for return a list of data block                                                                                                                     
// looks at the inode table and length of data block, return the array                                                                                        
// input: tablelocation - base location of inode table                                                                                                        
// input: i - offset into the base table                                                                                                                      
// input: length_block - length of the supposed data block array                                                                                              
// output: array of data block                                                                                                                                
int *create_data_block(void *tablelocation, int i, int length_block){
    struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
    int k = 0;
    int *array =(int*)malloc(length_block*sizeof(int));
    if (inode->i_block[12]){
		int *huge_array =(int*)malloc(1 * sizeof(int));
		huge_array[0] = 0;
		return huge_array;
	} 
    else {
		while(inode->i_block[k]){
			array[k] = inode->i_block[k];
			k++;
		}
	}
    return array;
}

// helper function 
/* Input : 
 *   name - the name we are looking for 
 *   blocknum - data block we are looking into
 * Output :
 *   int value - a mathed name and the inode number
 * */
int matched_name(char*name, int blocknum, int inum){
    // always set current to 0 when we start searching a block
    int current = 0; 
    int end = 1024; // end of block 
    unsigned int matched; // this is the inode that is returned
    while (current < end){
        // start the direct entry at where it is printing
        struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)
                            (disk + (EXT2_BLOCK_SIZE * blocknum) + current);

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

// helper function for reformatting the name
// the file name in directory entry doesn't have end of char 
char *create_fdir_name(char *fdir_name, int length){
    // we plus 1 on length because of end of char
    char *name = (char*)malloc((length+1)*sizeof(char));
    int i;

    for (i=0; i<length; i++){
            name[i] = fdir_name[i];
    }
    return name;
}

// helper function checks the type of the file
// NOTE: This check is for when check each type inside a directory entry
char filetypecheck(char flag){
    if (flag == 0){
            return 'u'; //unknown
    }
    else if(flag == 1){
            return 'f'; //regular file
    }
    else if(flag == 2){
            return 'd'; //dir
    }
    else if(flag == 3){
            return 'a'; //EXT2_FT_CHRDEV            = 3,
    }
    else if(flag == 4){ //EXT2_FT_BLKDEV            = 4,
            return 'b';
    }
    else if(flag == 5){ //EXT2_FT_FIFO              = 5,
            return 'c';
    }
    else if(flag == 6){ //EXT2_FT_SOCK              = 6,
            return 'd';
    }
    else if(flag == 7){ //EXT2_FT_SYMLINK           = 7,
            return 'e';
    } 
    else {
            return 'o';
    }
}
