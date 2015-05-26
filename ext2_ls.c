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
void print_directory_entry(int blocknum);
int found_name(char*name, int blocknum, int inum);
unsigned int explore_inode(const char *absolute_path);

int main(int argc, char const *argv[]){
    if(argc != 3) {
        fprintf(stderr, "Usage: ext2_ls <image file name> <path>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // get the group descriptor table
    // we know that this table always start at third datablock (index 2)
    struct ext2_group_desc *group_des_table = (struct ext2_group_desc*)(disk + EXT2_BLOCK_SIZE * 2);
    int inode_table = group_des_table->bg_inode_table;
    void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;
    

    int found_inode;
    found_inode = explore_inode(argv[2]);
    if (found_inode == -1){
        fprintf(stderr, "No such file or directory\n");
        exit(ENOENT);
    } 
    else if (found_inode == 0){ // found a relevant file
        char dup_path[strlen(argv[2]) + 1];
        strcpy(dup_path, argv[2]);

        const char ch = '/';
        char *ret;

        ret = strrchr(dup_path, ch);
        char*res;

        res = ret + 1;
        printf("%s\n", res);
    } 
    else {
        // get the list of data blocks 
        struct ext2_inode *inode = (struct ext2_inode*)(tablestart)+(found_inode-1);
        //return size of d_block
        int size_dblock = size_data_block(inode->i_block); 
        // allocate a space for array of d_block
        int *data_block = (int*)malloc(size_dblock*sizeof(int)); 
        // return a list of relevant data blocks
        data_block = create_data_block(tablestart, found_inode, size_dblock); 
        int i;
        for (i = 0; i < size_dblock; i++){
            // print all the datablocks
            print_directory_entry(data_block[i]);
        }
    }
    return 0;
}

// helper for printing directory entry
// input the blocknum and the for_inode_num
void print_directory_entry(int blocknum){
	int end = EXT2_BLOCK_SIZE; // each block is 1024 so keep print from block 
	int current = 0; // beg of block

	// root can always be hard coded 
	while (current < end){
		struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * blocknum) + current);
		//int inode_num = dir_entry->inode;
		unsigned short dir_len = dir_entry->rec_len;
		int name_length = dir_entry->name_len;
		char *fdirname; // need reformatt the filename
		fdirname = create_fdir_name(dir_entry->name, name_length);
		printf("%s\n", fdirname);
		current += (int)dir_len;
	}
}

unsigned int explore_inode(const char *absolute_path){

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
            temp = found_name(searchfile, data_block[j], foundnum);
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
int found_name(char*name, int blocknum, int inum){
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
                return 0; // we found a valid file instead 
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



