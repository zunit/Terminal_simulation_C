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

	if(argc != 4){
		fprintf(stderr, "Usage: ext2_ls <image file name> <absolute file path> <path to new file>\n");
		exit(1);
	}

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE, PROT_READ 
    						| PROT_WRITE, MAP_SHARED, fd, 0);

    if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
    }
    //search for the inode for the given absolute file path 
    int found_inode = search_inode(argv[2]);
    //if first file does not exist
    if (found_inode == -1){
    	fprintf(stderr, "File not found\n");
    	exit(ENOENT);
	}

	// get the group descriptor table
	// we know that this table always start at third datablock (index 2)
	struct ext2_group_desc *group_des_table = (struct ext2_group_desc*)
												(disk + EXT2_BLOCK_SIZE * 2);
	int inode_table = group_des_table->bg_inode_table;
	void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;

	struct ext2_inode *inode = (struct ext2_inode*)
								(tablestart)+(found_inode-1);

	//trying to link a directory
	if (inode->i_mode & EXT2_S_IFDIR){
		fprintf(stderr, "Is a directory\n");
		exit(EISDIR);
	}
	
	//make sure second file doesn't exist
	int second_inode = search_inode(argv[3]);
	if(second_inode != -1){
		struct ext2_inode *snode = (struct ext2_inode*)
									(tablestart)+(second_inode-1);
		if (snode->i_mode & EXT2_S_IFDIR){
			fprintf(stderr, "Is a directory\n");
			exit(EISDIR);
		}
		else{
			fprintf(stderr, "File exists\n");
			exit(EEXIST);
		}
	}

	//get the directory into which we want to cp the file
	int parent_dir_inode = parent_conversion(argv[3]);
    if (parent_dir_inode == -1){
    	fprintf(stderr, "No such file or directory\n");
    	exit(ENOENT);
	}

	//get the inode of that parent directory
	struct ext2_inode *parent_inode = (struct ext2_inode*)
								(tablestart)+(parent_dir_inode-1);

	if (parent_inode->i_mode & EXT2_S_IFREG){
		fprintf(stderr, "No such file or directory\n");
		exit(ENOENT);
	}

	//get location of the directory block
	//int block_num = parent_inode->i_block[0];
	int c;
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

		    par_dir->inode = found_inode;
		    par_dir->file_type = 1;

		    //get the name of the new directory
		    char dup_path[strlen(argv[3]) + 1];
		    strcpy(dup_path, argv[3]);
		    char *ret;
		    ret = strrchr(dup_path, '/');
		    if (ret[0] == '/'){
		    	ret++;
		    }
		    par_dir->rec_len = new_rec_len;
		    par_dir->name_len = strlen(ret);
		    strcpy(par_dir->name,ret);

		    //update links count
		    inode->i_links_count = inode->i_links_count + 1;
		    break;
		}
		c += node_dir->rec_len;
	}
	return 0;
}

