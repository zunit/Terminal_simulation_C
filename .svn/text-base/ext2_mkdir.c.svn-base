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

int main(int argc, char const *argv[]){
	if(argc != 3) {
        fprintf(stderr, "Usage: ext2_mkdir <image file name> <path>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE, PROT_READ 
                | PROT_WRITE, MAP_SHARED, fd, 0);

    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    //get the superblock 
    struct ext2_super_block *superblock = (struct ext2_super_block *)
                                            (disk + EXT2_BLOCK_SIZE);

    int first_data_block = superblock->s_first_data_block;

    // get the group descriptor table
    // we know that this table always start at third datablock (index 2)
    struct ext2_group_desc *group_des_table = (struct ext2_group_desc*)
                                                (disk + EXT2_BLOCK_SIZE * 2);

    int inode_bitmap = group_des_table->bg_inode_bitmap;
    int block_bitmap = group_des_table->bg_block_bitmap;
    int inode_table = group_des_table->bg_inode_table;
    void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;

    int found_inode = search_inode(argv[2]);
    //already exists
    if (found_inode != -1){ 
        fprintf(stderr, "Directory exists\n");
        exit(EEXIST);
    } 

    //get the parent directory in which to create this new dir
    int parent_inode_num = parent_conversion(argv[2]);
    if (parent_inode_num == -1){
    	exit(ENOENT);
    }
    //get the inode of that parent directory
    struct ext2_inode *parent_inode = (struct ext2_inode*)
                                        (tablestart) + (parent_inode_num - 1);

    //parent does not exist
    if (parent_inode->i_mode & EXT2_S_IFREG){
        fprintf(stderr, "No such file or directory\n");
    	exit(ENOENT);
    }

    //make new directory - block bitmap to find free block
    unsigned long block_bitmap_block = block_bitmap * EXT2_BLOCK_SIZE;
    void * block_bits = (void *)(disk + block_bitmap_block);
   
    int free_block_num = get_free_block(block_bits);
    if (free_block_num == -1){
        fprintf(stderr, "No space left on device\n");
    	exit(ENOSPC);
    }
    superblock->s_free_blocks_count = superblock->s_free_blocks_count - 1;
    superblock->s_free_inodes_count = superblock->s_free_inodes_count - 1;

    //get free inode number
    unsigned long inode_bitmap_block = inode_bitmap * EXT2_BLOCK_SIZE;
    void * inode_bits = (void *)(disk + inode_bitmap_block);

    int inode_indx = get_free_inode(inode_bits);

    //create new inode
    struct ext2_inode *new_inode = (struct ext2_inode*)
                                    (tablestart) + (inode_indx);
    new_inode->i_mode = 16384;
    new_inode->i_size = EXT2_BLOCK_SIZE;
    new_inode->i_blocks = 2;
    new_inode->i_links_count = 2;
    new_inode->i_block[0] = first_data_block + free_block_num - 1;

    //create new directory entry for self
    struct ext2_dir_entry_2 *new_dir_self = (struct ext2_dir_entry_2 *)
                        (disk + (EXT2_BLOCK_SIZE * new_inode->i_block[0]));

    new_dir_self->inode = inode_indx + 1;
    new_dir_self->rec_len = 12;
    new_dir_self->name_len = 1;
    new_dir_self->file_type = 2;
    strcpy(new_dir_self->name, ".");

    //create new directory entry for parent
    struct ext2_dir_entry_2 *new_dir_parent = (struct ext2_dir_entry_2 *)
                        (disk + (EXT2_BLOCK_SIZE * new_inode->i_block[0])+ 12);

    new_dir_parent->inode = parent_inode_num;
    new_dir_parent->rec_len = 1012;
    new_dir_parent->name_len = 2;
    new_dir_parent->file_type = 2;
    strcpy(new_dir_parent->name, "..");

    //get the last directory block of its parent's directory
    int k = 0;
    while (parent_inode->i_block[k+1]){
        k++;
    }

    //add directory entry to the parent directory
    int c;
    for (c = 0; c < EXT2_BLOCK_SIZE;){
    	struct ext2_dir_entry_2 *node_dir = (struct ext2_dir_entry_2 *)
                (disk + (EXT2_BLOCK_SIZE * parent_inode->i_block[k]) + c);

    	if (c + node_dir->rec_len == EXT2_BLOCK_SIZE){
    		//this is the last block in the directory
			int new_rec_len = node_dir->rec_len - node_dir->name_len - 8;
			node_dir->rec_len = node_dir->name_len + 8;

			//create directory entry
		    struct ext2_dir_entry_2 *par_dir = (struct ext2_dir_entry_2 *)
                    (disk + (EXT2_BLOCK_SIZE * parent_inode->i_block[k] 
                        + c + node_dir->rec_len));

		    par_dir->inode = inode_indx + 1;
		    par_dir->file_type = 2;

		    //get the new name
		    char dup_path[strlen(argv[2])];
		    strcpy(dup_path, argv[2]);
		    char *ret;
		    ret = strrchr(dup_path, '/');
		    if (ret[0] == '/'){
		    	ret++;
		    }

		    par_dir->rec_len = new_rec_len;
		    par_dir->name_len = strlen(ret);
		    strcpy(par_dir->name,ret);

		    break;
    	}
    	c += node_dir->rec_len;
    }
	return 0;
}

