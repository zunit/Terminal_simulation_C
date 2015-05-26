#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;


// this transforms it into bits
void byte_to_bits(unsigned char *bytes){

	printf(" ");
	unsigned char value = *bytes;
	char temp;
	temp = value & 1 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 2 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 4 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 8 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 16 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 32 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 64 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 128 ? '1' : '0';
	printf ("%c", temp);
}

// helper function for checking the flag type either file or dir
// NOTE: This check is from the inode level 
char typecheck(int flag){
		char type;
		if(flag & EXT2_S_IFREG){
			type = 'f';
		}
		else if (flag & EXT2_S_IFDIR){
			type = 'd';
		} else {
			type = 'q';
		}
		return type;
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
		return 'a'; //EXT2_FT_CHRDEV		= 3,
	}
	else if(flag == 4){ //EXT2_FT_BLKDEV		= 4,
		return 'b';
	}
	else if(flag == 5){ //EXT2_FT_FIFO		= 5,
		return 'c';
	}
	else if(flag == 6){ //EXT2_FT_SOCK		= 6,
		return 'd';
	}
	else if(flag == 7){ //EXT2_FT_SYMLINK		= 7,
		return 'e';
	} else {
		return 'o';
	}
}

// helper function for reformatting the name
// the file name in directory entry doesn't have end of char 
char *create_fdir_name(char *fdir_name, int length){
	char *name = (char*)malloc(length*sizeof(char));
	int i;

	for (i=0; i<length; i++){
		name[i] = fdir_name[i];
	}
	return name;
}

// helper function for printing out data blocks
// NOTE: reading from inode level 
void printdatablocks(unsigned int *i_block){
	int i = 0;
	while (i_block[i]){
		printf("%u ", i_block[i]);
		i++;
	}
}

// this prints out all valid inodes
// go through the valid inodes and print out it's struct info
void print_valid_inodes(void *tablelocation, int i){
	struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
	// something valid at that inode block			
	int type = typecheck(inode->i_mode);  
	int size = inode->i_size;
	int links = inode->i_links_count;
	int iblocks = inode->i_blocks;
	printf("[%d] type: %c size: %d links: %d blocks: %d\n", i, type, size, links, iblocks);
	printf("[%d] blocks: ", i);
	printdatablocks(inode->i_block);
	printf("\n");
}

// helper for printing out the array 
// NOTE: Creating an array of available inode 
void print_inode_array(int *inode_array, int size){
	int i;
	for (i = 0; i < size; i++){
		printf("%d ", inode_array[i]);
	}
}

// helper for printing directory entry
// input the blocknum and the for_inode_num
void print_directory_entry(int blocknum, int inodenum){
	int end = 1024; // each block is 1024 so keep print from block 
	int current = 0; // beg of block

	// root can always be hard coded 
	printf("\tDIR BLOCK NUM: %d (for inode %d)\n", blocknum, inodenum);
	while (current < end){
		struct ext2_dir_entry_2 *dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * blocknum) + current);
		int inode_num = dir_entry->inode;
		unsigned short dir_len = dir_entry->rec_len;
		int name_length = dir_entry->name_len;
		char file_flag = filetypecheck(dir_entry->file_type); 
		char *fdirname; // need reformatt the filename
		fdirname = create_fdir_name(dir_entry->name, name_length);

		printf("Inode: %d rec_len: %hu name_len: %d type= %c name=%s\n", 
			inode_num, dir_len, name_length, file_flag, fdirname);
		current += (int)dir_len;
	}
}



// helper for returning the size of data_block
// when looking at a valid inode we check how many data block it point
// input: tablelocation - the location of inode table
// input: i - how far down offset into the table we should point to 
int size_data_block(void *tablelocation, int i){
	struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
	int k = 0;
	while(inode->i_block[k]){
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
int * create_data_block(void *tablelocation, int i, int length_block){
	struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
	int k = 0;
	int *array =(int*)malloc(length_block*sizeof(int));
	int block = 0;
	while(inode->i_block[k]){
		array[block] = inode->i_block[k];
		k++;
	}
	return array;
}


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
	
	// super block is always the second data block
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    
    // group description table always the third data block 
    struct ext2_group_desc *group_desc_table = (struct ext2_group_desc *)(disk + (1024 * 2));
    int inode_bitmap = group_desc_table->bg_inode_bitmap;
    int block_bitmap = group_desc_table->bg_block_bitmap;
    int inode_table = group_desc_table->bg_inode_table;
    printf("Block group: \n");
    printf("\t block bitmap: %d\n", block_bitmap);
    printf("\t inode bitmap: %d\n", inode_bitmap);
    printf("\t inode table: %d\n", inode_table);
    printf("\t free block: %d\n", group_desc_table->bg_free_blocks_count);
    printf("\t free inodes: %d\n", group_desc_table->bg_free_inodes_count);
    printf("\t used_dirs: %d\n", group_desc_table->bg_used_dirs_count); 
    printf("Block bitmap:");
    
    // starts at 3072 at c00  
    int start = EXT2_BLOCK_SIZE * block_bitmap;
    int total = sb->s_blocks_count / 8; // 8 bits in a byte. 16 groups 
    int i;
    for (i = 0; i < total; i++){
		// convert all the bits and bytes 
		byte_to_bits(disk+start+i);
	}
    printf("\n");
 
    printf("Inode bitmap:");
    // starts at 4096
    int total_inode = 4	;
    int start_inode = EXT2_BLOCK_SIZE *  inode_bitmap;
    int j;
    for (j = 0; j  < total_inode; j++){
		byte_to_bits(disk+start_inode+j);
	}
    printf("\n\n");
    
    printf("Inodes:\n");
    // find out where the table starts
    void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;
    // we are hard coding the root inode since it's always 2
    // we plus one to indicate the start of root inode and end of the first inode
    struct ext2_inode *rootinode = (struct ext2_inode*)(tablestart)+1;
    int type = typecheck(rootinode->i_mode);  
    int size = rootinode->i_size;
    int links = rootinode->i_links_count;
    int iblocks = rootinode->i_blocks;
    printf("[2] type: %c size: %d links: %d blocks: %d\n", type, size, links, iblocks);
    printf("[2] blocks:");
    printdatablocks(rootinode->i_block);
    printf("\n");
    
    // below are for inode block after 11    
    // need another pointer that starts at the beginning of the inode_bitmap
    void *inode_bit_start = disk + start_inode;
    int inode_bits = *((int*)inode_bit_start);
    inode_bits = inode_bits >> 11; 
    
    int k;
    int counter = 0; // this will record how many inode are occupied 

    //we start at 11 because index 0 counts
    for (k = 11; k < 32; k++){
		if(inode_bits & 1){
			print_valid_inodes(tablestart, k+1);
			counter += 1;
		}
		inode_bits = inode_bits >> 1; 
	}
    
    // this code is for other parts in the acutally assignment 3
    // ----------------------------------------------------------
    // this is extra 2 for the putting the root and lost+found
    //counter += 2;

    //printf("counter length: %d\n", counter);
    // the reason we plus 1 here is because of the root
    /*int occupied_inode[counter];

    // need another pointer that starts at the beginning of the inode_bitmap
    void *inode_bit_array_form = disk + start_inode;
    int ibits_array = *((int*)inode_bit_array_form);
    ibits_array = ibits_array >> 11; 
 
    occupied_inode[0] = 2; // filling in the root inode
    occupied_inode[1] = 11; // filling in the lost+found inode
    int inode_count = 2;
    for (k = 11; k < 32; k++){
		if(ibits_array & 1){
			// this stores the inode that need to be examined
			occupied_inode[inode_count] = k+1;
			inode_count += 1;
		}
		ibits_array = ibits_array >> 1; 
	}*/
	 // ----------------------------------------------------------
	// testing code ---> print_inode_array(occupied_inode, counter);
	printf("\n");
	printf("Directory Blocks:\n");
	
	// loop through the 
	// root starts at 9
	
	int end = 1024;
	int current = 0;

	// root can always be hard coded 
	printf("\tDIR BLOCK NUM: %d (for inode %d)\n", 9, 2);
	while (current < end){
		struct ext2_dir_entry_2 *root_dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * 9) + current);
		int inode_num = root_dir_entry->inode;
		unsigned short dir_len = root_dir_entry->rec_len;
		int name_length = root_dir_entry->name_len;
		char file_flag = filetypecheck(root_dir_entry->file_type);
		char *fdirname;
		fdirname = create_fdir_name(root_dir_entry->name, name_length);

		printf("Inode: %d rec_len: %hu name_len: %d type= %c name=%s\n", 
			inode_num, dir_len, name_length, file_flag, fdirname);
		current += (int)dir_len;

	}
	/*
	 * go through the bit again and get all the iblocks[i]
	 **/
	void *inode_bit_start_block = disk + start_inode;
    int inode_bits_block = *((int*)inode_bit_start_block);
    inode_bits_block = inode_bits_block >> 11; 
    
    int l;
    int length_block;

    //int list_data_block[length_block]; // this is the list of data blocks
    //we start at 11 because index 0 counts
    for (l = 11; l < 32; l++){
		if(inode_bits_block & 1){
			length_block = size_data_block(tablestart, l+1);
			int *list_data_block;

			struct ext2_inode *inode_dir_check = (struct ext2_inode*)(tablestart)+(l);
			char type = typecheck(inode_dir_check->i_mode);
			//printf("type: %c\n", type);
			// this creates the data_block 
			if (type == 'd'){
				list_data_block = create_data_block(tablestart, l+1, length_block);
				int m;
				// this prints out the data block that we are going into
				for (m = 0; m<length_block; m++){ 
					//printf("printing: %d ", list_data_block[m]);
					print_directory_entry(list_data_block[m], l+1);
					// function for print out each content
				}
			}
		}
		inode_bits_block = inode_bits_block >> 1; 
	}
    return 0;
    
   
}
