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
#include "disk.h"

unsigned char *disk;

int main(int argc, char const *argv[]){
	if(argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <formatted virtual disk> <local path> <absolute disk path>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE, PROT_READ 
    						| PROT_WRITE, MAP_SHARED, fd, 0);

    if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
    }

    FILE * fp;
    fp = fopen(argv[2], "r");
    if (fp == NULL) {
    	printf("File not found\n");
    	exit(ENOENT);
    }
    //get file size
    fseek(fp, 0L, SEEK_END);
    int file_sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    //number of blocks needed to copy file
    int num_blocks_needed = (int) (file_sz / EXT2_BLOCK_SIZE) + 1;

    //get the superblock 
    struct ext2_super_block *superblock = (struct ext2_super_block *)
    										(disk + EXT2_BLOCK_SIZE);

    int first_data_block = superblock->s_first_data_block;

    if (num_blocks_needed > superblock->s_free_blocks_count){
    	fprintf(stderr, "No space left on device\n");
    	exit(ENOSPC);
    }

	// get the group descriptor table
	// we know that this table always start at third datablock (index 2)
	struct ext2_group_desc *group_des_table = (struct ext2_group_desc*)
												(disk + EXT2_BLOCK_SIZE * 2);
	int inode_bitmap = group_des_table->bg_inode_bitmap;
	int block_bitmap = group_des_table->bg_block_bitmap;
	int inode_table = group_des_table->bg_inode_table;
	void *tablestart = disk + EXT2_BLOCK_SIZE * inode_table;

	//check that the path does not already exist
	int found_inode;
	found_inode = search_inode(argv[3]);
    if (found_inode != -1){ // has to be not found
    	fprintf(stderr, "File exists\n");
        exit(EEXIST);
    }

    //get the parent directory into which to copy the file
    int parent_inode_num = parent_conversion(argv[3]);
    if (parent_inode_num == -1){
    	fprintf(stderr, "No such file or directory\n");
    	exit(ENOENT);
    }

    //get the inode of that parent directory
    struct ext2_inode *parent_inode = (struct ext2_inode*)
    									(tablestart) + (parent_inode_num - 1);
    if (parent_inode->i_mode & EXT2_S_IFREG){
    	fprintf(stderr, "No such file or directory\n");
    	exit(ENOENT); 
    }

	//get block bitmap
    int block_bitmap_block = block_bitmap * EXT2_BLOCK_SIZE;
    void * block_bits = (void *)(disk + block_bitmap_block);

    //get inode bitmap
    unsigned long inode_bitmap_block = inode_bitmap * EXT2_BLOCK_SIZE;
    void * inode_bits = (void *)(disk + inode_bitmap_block);

    //find free inode
    int inode_indx = get_free_inode(inode_bits);

    if (inode_indx == -1){
    	fprintf(stderr, "No space left on device\n");
    	exit(ENOSPC);
    }

    //update superblock
    superblock->s_free_inodes_count = superblock->s_free_inodes_count - 1;
    
    //init new inode
    struct ext2_inode *new_inode = (struct ext2_inode*)(tablestart) 
    													+ (inode_indx);
    new_inode->i_mode = 32768; //is file
    new_inode->i_size = file_sz;
    new_inode->i_blocks = 1;
    new_inode->i_links_count = 1;

    //global variables for indirect block if necessary
    void *indirect_block;
    unsigned int indr_block[15];
    void *data_block;

    int i;
    for (i = 0; i < num_blocks_needed; i++){
    	if (i < 12){
    		int free_block_num = get_free_block(block_bits);
    		if (free_block_num == -1){
    			fprintf(stderr, "No space left on device\n");
    			exit(ENOSPC);
    		}
    		superblock->s_free_blocks_count = superblock->s_free_blocks_count - 1;
    		new_inode->i_blocks += 1;
    		new_inode->i_block[i] = first_data_block + free_block_num -1;
    		data_block = (void *)(disk + (EXT2_BLOCK_SIZE * 
    													new_inode->i_block[i]));

    	}	
    	else if (i == 12){
    		//i_blocks[12] entry is the 13th entry in the array
    		//It holds the block number of the indirect block
    		//- this block contains an array of block numbers 
    		//for the next data blocks of the file.
    		//get free block for indirect block
    		int indirect_block_num = get_free_block(block_bits);
    		if (indirect_block_num == -1){
    			fprintf(stderr, "No space left on device\n");
    			exit(ENOSPC);
    		}
    		new_inode->i_block[i] = first_data_block + indirect_block_num;
    		//initialize indirect block
    		indirect_block = (void *)(disk + (EXT2_BLOCK_SIZE 
    											* new_inode->i_block[i]));
    		//get data block
    		int free_block_num = get_free_block(block_bits);
    		if (free_block_num == -1){
    			fprintf(stderr, "No space left on device\n");
    			exit(ENOSPC);
    		}
    		superblock->s_free_blocks_count = superblock->s_free_blocks_count - 1;
    		new_inode->i_blocks += 1;
    		indr_block[i - 12] = first_data_block + free_block_num -1;
    		memcpy(indirect_block, (void *) indr_block, 16);
    		data_block = (void *)(disk + (EXT2_BLOCK_SIZE * indr_block[i - 12]));
    	}
    	else if (i > 12){
    		int free_block_num = get_free_block(block_bits);
    		if (free_block_num == -1){
    			fprintf(stderr, "No space left on device\n");
    			exit(ENOSPC);
    		}
    		superblock->s_free_blocks_count = superblock->s_free_blocks_count - 1;
    		new_inode->i_blocks += 1;
    		indr_block[i - 12] = first_data_block + free_block_num -1;
    		memcpy(indirect_block, (void *) indr_block, 16);
    		data_block = (void *)(disk + (EXT2_BLOCK_SIZE * indr_block[i - 12]));
    	}

    	//read data from file to data block
    	int num_read = fread(data_block, 1, EXT2_BLOCK_SIZE, fp);
    	if (num_read == 0){
    		perror("Error reading file");
    	}
    }

    fclose(fp);

    //add this new file's directory entry to the parent directory
    int c;
    //loop through directory entries to get to the last entry
    for (c = 0; c < EXT2_BLOCK_SIZE;){
    	struct ext2_dir_entry_2 *node_dir = (struct ext2_dir_entry_2 *)
    				(disk + (EXT2_BLOCK_SIZE * parent_inode->i_block[0]) + c);

    	if (c + node_dir->rec_len == EXT2_BLOCK_SIZE){
    		//this is the last block in the directory
			int new_rec_len = node_dir->rec_len - node_dir->name_len - 8;
			node_dir->rec_len = node_dir->name_len + 8;

			//create directory entry
		    struct ext2_dir_entry_2 *par_dir = (struct ext2_dir_entry_2 *)
		    		(disk + (EXT2_BLOCK_SIZE * parent_inode->i_block[0] 
		    		+ c + node_dir->rec_len));

		    par_dir->inode = inode_indx + 1;
		    par_dir->file_type = 1;

		    //get name of new file
		    char dup_path[strlen(argv[3])];
		    strcpy(dup_path, argv[3]);
		    char *ret;
		    ret = strrchr(dup_path, '/');
		    if (ret[0] == '/'){
		    	ret++;
		    }
		    par_dir->rec_len = new_rec_len;
		    par_dir->name_len = strlen(ret);
		    strcpy(par_dir->name, ret);
		    break;
    	}
    	c += node_dir->rec_len;
    }
    return 0;
}




