#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "disk.h"
#include "fs.h"
#define MAX_F_NAME 16
#define MAX_FILES 64
#define MAX_FILDES 32
#define BLOCK_SIZE 4096

struct super_block {
	int fat_idx; // First block of the FAT
	int fat_len; // Length of FAT in blocks
	int dir_idx; // First block of directory
	int dir_len; // Length of directory in blocks
	int data_idx; // First block of file-data
	int free;
};

struct dir_entry {
	int used; // Is this file-”slot” in use
	char name [MAX_F_NAME + 1]; // DOH!
	int size; // file size
	int head; // first data block of file
	int ref_cnt;
	// how many open file descriptors are there?
	// ref_cnt > 0 -> cannot delete file
};

struct file_descriptor {
	int used; // fd in use
	int file; // the first block of the file (f) to which fd refers too
	int offset; // position of fd within f
};

struct super_block fs;
struct file_descriptor fildes[MAX_FILDES]; // 32
int FAT[(8*BLOCK_SIZE)/sizeof(int)]; // Will be populated with the FAT data
struct dir_entry DIR[BLOCK_SIZE/sizeof(struct dir_entry)]; // Will be populated with the directory data
char user_buf[BLOCK_SIZE];

int new_FAT();

int make_fs(char *disk_name){
	
	if(make_disk(disk_name) == -1){
		return -1;
	}
	 if(open_disk(disk_name) == -1){
                return -1;
        }

	fs.fat_idx = 2;
        fs.fat_len = 8;
	fs.dir_idx = 1; 
	fs.dir_len = 1; 
	fs.data_idx = 10;
	
	memset(DIR, 0, fs.dir_len * BLOCK_SIZE);

	int i;	
	for (i = 0; i < MAX_FILES; i++) {
        	DIR[i].used = 0;
        	DIR[i].ref_cnt = 0;
		DIR[i].head = -3; 
   	}
	
	for (i = 0; i < MAX_FILDES; i++) {
        	fildes[i].used = 0;
		fildes[i].file = -1; 
   	}	

	//memset(FAT, 0, fs.fat_len * BLOCK_SIZE);	
	
	if (block_write(0,(char*) &fs) == -1) {
        	return -1;
    	}
	for(i = 0; i<(8*BLOCK_SIZE)/sizeof(int); i++){
		FAT[i] = -1;

	}

	for (i=0; i<fs.fat_len; i++){
		//FAT[i] = -1;
    		if (block_write(fs.fat_idx + i,(char*) &FAT[i*(BLOCK_SIZE/sizeof(int))]) == -1) {
        		return -1;
    		}
	}

    	if (block_write(fs.dir_idx,(char*)& DIR) == -1) {
        	return -1;
    	}		
	
	close_disk();
		
	return 0;
}


int mount_fs(char *disk_name){
	if (open_disk(disk_name) == -1 || block_read(0, (char*)&fs) == -1) {
        	return -1;
    	}
	
//	user_buf = (char*) malloc(BLOCK_SIZE);

	int i;
	for (i = 0; i<fs.fat_len; i++){
    		if (block_read(fs.fat_idx + i,(char*)& FAT[i*(BLOCK_SIZE/sizeof(int))]) == -1) {
        		return -1;
    		}
	}

    	if (block_read(fs.dir_idx, (char*) &DIR) == -1) {
        	return -1;
    	}
/*
    	for (i = 0; i < MAX_FILDES; i++) {
        	fildes[i].used = 0;
    	}
*/
    	return 0;			

}

int umount_fs(char *disk_name){
    	
	if (block_write(0,(char*) &fs) == -1) {
        	return -1;
    	}

    	if (block_write(fs.dir_idx, (char*)&DIR) == -1) {
        	return -1;
    	}
	
	int i;
	for (i = 0; i<fs.fat_len; i++){
    		if (block_write(fs.fat_idx + i, (char*)&FAT[i*(BLOCK_SIZE/sizeof(int))]) == -1) {
        		return -1;
    		}
	}

    	if (close_disk() == -1) {
        	return -1;
    	}
    	return 0;

}



int fs_open(char *name){
	int i;
        for (i = 0; i < MAX_FILES; ++i) {
                if (strcmp(DIR[i].name, name) == 0) {
                        int fd;
                        for (fd = 0; fd < MAX_FILDES; ++fd) {
                                if (!fildes[fd].used) {

                                	fildes[fd].used = 1;
                                	fildes[fd].file = i;
                                	fildes[fd].offset = 0;
                                	DIR[i].ref_cnt++;
                                	return fd;
                                }
                        }
		}
        }

	return -1;
}



int fs_close(int fd){
	if (fildes[fd].used != 1){
		return -1;
	}	
	
	fildes[fd].used = 0;
	
	if(DIR[fildes[fd].file].ref_cnt == 0 || fildes[fd].file	== -1){
		return -1;
	}

	DIR[fildes[fd].file].ref_cnt--;
	fildes[fd].file = -1;

	return 0;
}

int fs_create(char *name){
	int i;
	for (i = 0; i < MAX_FILES; ++i) {
                if (strcmp(DIR[i].name, name) == 0) {
			return -1;
		}
	}	

	if(strlen(name) > MAX_F_NAME){
		return -1;
	}

	int file = 100;
	for(i = 0; i< MAX_FILES; i++){
		if(!DIR[i].used){
			file = i;
			break;
		}
	}
	
	if (file == 100){
		return -1;
	}

	DIR[file].head = new_FAT();
//	FAT[DIR[file].head] = -2;
	DIR[file].used = 1;
	strncpy(DIR[file].name, name, MAX_F_NAME);
	DIR[file].name[MAX_F_NAME] = '\0';
	DIR[file].size = 0; 
	
	
	return 0;


}

int fs_delete(char *name){
	int i;
	for (i = 0; i < MAX_FILES; ++i) {
                if (strcmp(DIR[i].name, name) == 0) {
                        break;
                }
		if(i == MAX_FILES -1){
			return -1;
		}
        }

	if(DIR[i].ref_cnt == 0){
 
        }else{
/*
                int j;
                for(j = 0; j<DIR[i].ref_cnt; j++){
                        int k = 0;
                        for (k = 0; k < MAX_FILDES; k++) {
                                if (fildes[k].file == i) {
                                        fildes[k].file = -1;
                                        fildes[k].used = 0;
                                }
                        }

                }
*/
		return -1;
        }
/*	
        int head = DIR[i].head;

        for (i = 0; i<BLOCK_SIZE;i++){
                user_buf[i] = '\0';
        }
        int oldhead = head;
        int newhead = head;
        while(FAT[newhead] != -1 || FAT[newhead] != -2){
                block_write(newhead, user_buf);
                if (FAT[newhead] == -2){
                        break;
                }else{
                        newhead = FAT[newhead];
                        FAT[head] = -1;


                        head = newhead;
                }

        }
        FAT[oldhead] = -1;
*/



	DIR[i].used = 0;
	DIR[i].size = 0;
	DIR[i].name[0] = '\0';

	return 0;


}


int fs_read(int fd, void *buf, size_t nbyte){
/*	
	int head = DIR[fildes[fd].file].head;
	int offset = fildes[fd].offset;
	int bytes = 0;
	if(offset!= 0){
		bytes = read(head*BLOCK_SIZE + offset, buf, BLOCK_SIZE-offset);	
		nbyte -= offset;
	}



	int blocks = BLOCK_SIZE/nbyte;
	int additional = BLOCK_SIZE % nbyte;
	blocks = blocks + additional;
	int i;
	for(i = 0; i<blocks; i++){
		bytes += block_read(head+(i+1), (char*) buf +(i*(BLOCK_SIZE/sizeof(int))));	
	}
	
	fildes[fd].offset = bytes;	
	return bytes;
*/
/*
	int i;
	int head = DIR[fildes[fd].file].head;
	int offset = fildes[fd].offset;
	int counter = 0;
	int test = 0;
	int newoffset = 0;
	if(offset!= 0){
		int numblock = offset/BLOCK_SIZE;
		newoffset = offset%BLOCK_SIZE;
		for (i = 0; i< numblock; i++){
			head = FAT[head];
		}		
                block_read(head*BLOCK_SIZE, user_buf); 
                nbyte -= BLOCK_SIZE - newoffset;
		memcpy(buf, user_buf+newoffset, BLOCK_SIZE- offset);
		test = 1;
        }
	counter = BLOCK_SIZE - newoffset;
	int newhead;
	if (test){

//		if (FAT[newhead] != -1 || FAT[newhead] != -2){
			newhead = FAT[head];
//		}
	}
	while(nbyte > BLOCK_SIZE){
		if(newhead < 0){
			break;
		}else{
			block_read(newhead*BLOCK_SIZE, user_buf);
			memcpy(buf+counter, user_buf, BLOCK_SIZE);
			counter += BLOCK_SIZE;
			newhead = FAT[newhead];
			nbyte -= BLOCK_SIZE;
		}
	}
	if(nbyte>0 && newhead >0){
		block_read(newhead*BLOCK_SIZE, user_buf);
		memcpy(buf+counter, user_buf, nbyte);
	}
	counter += nbyte;

	return counter;

*/
	if(fd <0 || fd >32 || fildes[fd].used != 1){
		return -1;
	}else if(DIR[fildes[fd].file].size == 0 || fildes[fd].offset > DIR[fildes[fd].file].size){
		return 0;
	}

	int toread = nbyte;
	if(fildes[fd].offset + nbyte > DIR[fildes[fd].file].size){
		toread = DIR[fildes[fd].file].size - fildes[fd].offset;

	}
	
	
	int block = DIR[fildes[fd].file].head;
	int offset = fildes[fd].offset;
	int numblock = offset/BLOCK_SIZE;
	int i;
	for(i=0; i<numblock-1; i++){
		block = FAT[block];
	}

	user_buf[0] = '\0';
	int counter = 0;
	while(toread>0 && (block!=-1 || block != -2)){
		if(block_read(block, user_buf)<0){
			return -1;
		}
		int newoffset = offset % BLOCK_SIZE;
		int left = BLOCK_SIZE - newoffset;
		int toread2;
		if(toread < left){

			toread2 = toread;
		}else{
			toread2 = left;
		}
		
		memcpy((char*)buf +counter, user_buf+newoffset, toread2);
		
		counter += toread2;
		toread -= toread2;
		offset += toread2;
		block = FAT[block];

	} 	
		fildes[fd].offset += counter;
		return counter;

}

int fs_write(int fd, void *buf, size_t nbyte){

/*
	int i;
	int disk = 0;
        if(DIR[fildes[fd].file].head == -3){
                for(i = 10; i < (8*BLOCK_SIZE)/sizeof(int); i++){
                        if (FAT[i] == -1){
                                DIR[fildes[fd].file].head = i;                  
                        }
			if(i == (8*BLOCK_SIZE)/sizeof(int) -1){
                                        disk = 1;
                        } 
               }
        }
	int head = DIR[fildes[fd].file].head;
        int offset = fildes[fd].offset;
        int counter = 0;
        int test = 0;
	int numblock = 0;
	int newoffset = 0;
        if(offset!= 0){
		numblock = offset/BLOCK_SIZE;
                newoffset = offset%BLOCK_SIZE;
                for (i = 0; i< numblock; i++){
                        head = FAT[head];
         
       }
		block_read(head*BLOCK_SIZE, user_buf);
		memcpy(user_buf+newoffset, buf, BLOCK_SIZE- newoffset);
		block_write(head*BLOCK_SIZE, user_buf);
                nbyte -= BLOCK_SIZE - offset;
                test = 1;
        }
        counter = BLOCK_SIZE - newoffset;
        int newhead = head;
	if (test){
                if (FAT[newhead] != -1 || FAT[newhead] != -2){
                        newhead = FAT[newhead];
                }else{
			for(i = 10; i < (8*BLOCK_SIZE)/sizeof(int); i++){
                                if (FAT[i] == -1){
					FAT[newhead] = i;
					newhead = i;                                
                                }
        			if(i == (8*BLOCK_SIZE)/sizeof(int) -1){
                                	disk = 1;
                                }
	                }


		}
        }
	
        while(nbyte > BLOCK_SIZE){
                if(newhead < 0){
                        for(i = 10; i < (8*BLOCK_SIZE)/sizeof(int); i++){
                        	if (FAT[i] == -1){
					FAT[newhead] = i;
                                        newhead = i;
				}
				if(i == (8*BLOCK_SIZE)/sizeof(int) -1){
                                disk = 1;
                        	}
			}
                }
		if(!disk){
                        block_read(newhead*BLOCK_SIZE, user_buf);
                        memcpy(user_buf, buf + counter, BLOCK_SIZE);
			block_write(newhead*BLOCK_SIZE, user_buf);
                        counter += BLOCK_SIZE;
                        newhead = FAT[newhead];
			nbyte -= BLOCK_SIZE;
                }
        }

	//disk = 0;
        if(nbyte>0 && newhead >0){
		block_read(newhead*BLOCK_SIZE, user_buf);
                memcpy(user_buf, buf + counter, nbyte);
                block_write(newhead*BLOCK_SIZE, user_buf);
		nbyte -= nbyte;
                counter += nbyte;
        }else if(nbyte >0 && newhead<0){
		for(i = 10; i < (8*BLOCK_SIZE)/sizeof(int); i++){
                	if (FAT[i] == -1){
                        	FAT[newhead] = i;
                                newhead = i;
                        }
			if(i == (8*BLOCK_SIZE)/sizeof(int) -1){
				disk = 1;		
			
			}
                }
		if(!disk){
			block_read(newhead*BLOCK_SIZE, user_buf);
                	memcpy(user_buf, buf + counter, nbyte);
                	block_write(newhead*BLOCK_SIZE, user_buf);
			nbyte -= nbyte;
			counter += nbyte;
		}
	}

	FAT[newhead] = -2;
        int added = counter - DIR[fildes[fd].file].size - offset;
        if(added <0){
                added = 0;
        }
        DIR[fildes[fd].file].size += added;
	fildes[fd].offset += counter;
        return counter;
*/
	int i;
	if( fd > 32 || fd <0 || fildes[fd].used != 1){
//		printf("intitial");
		return -1;
	}
//		printf("intitial");

	//int idx = fildes[fd].file;
	//16777216

	if(nbyte > 16777216 - fildes[fd].offset){
		nbyte = 16777216 - fildes[fd].offset;
	}		
	
	int offset = fildes[fd].offset;
	int byteswritten = 0;
	int head = DIR[fildes[fd].file].head;
	
	int newblock = offset / BLOCK_SIZE;
	int newoffset = offset % BLOCK_SIZE;

	if(DIR[fildes[fd].file].size == 0){
//		head = new_FAT();
		if(head == -1){
//			printf("intitial");		
			return -1;
//			printf("head read1");
		}
		FAT[head] = -2;		
	}else{
		for(i = 0; i<newblock-1; i++){
			if(FAT[head] != -2){
				head = FAT[head];
			}else{
				break;

			}
		}
	}

	while(nbyte>0){
		if(newoffset == BLOCK_SIZE){
			int nextblock = new_FAT();
			if(nextblock == -1){
				break;
			}
			FAT[head] = nextblock;
			head = nextblock;
			FAT[head] = -2;
			newoffset = 0;

		}
		int writebytes = BLOCK_SIZE - newoffset;
		if(writebytes > nbyte){
			writebytes = nbyte;
		}
		
		block_read(head,(char*) user_buf);
	
		memcpy(user_buf + newoffset, (char*)buf+byteswritten, writebytes);
		
		block_write(head,(char*) user_buf);

		byteswritten += writebytes;

		nbyte -= writebytes;
		fildes[fd].offset += writebytes;

		newoffset += writebytes;
	}

	if(DIR[fildes[fd].file].size > fildes[fd].offset){

	}else{
		DIR[fildes[fd].file].size = fildes[fd].offset;
	}

	return byteswritten;

}

int new_FAT(){
	int i;
	for(i = 10; i < (8*BLOCK_SIZE)/sizeof(int); i++){
                        if (FAT[i] == -1){
                                //FAT[newhead] = i;
                                return i;
                        }
                        if(i == (8*BLOCK_SIZE)/sizeof(int) -1){
                                return -1;               
                        
                        }
         }
	return -1;

}
	
int fs_get_filesize(int fd){

	if(fd > 32 || fd < 0 || fildes[fd].used != 1){
		return -1;
	}

	return DIR[fildes[fd].file].size;

}

int fs_listfiles(char ***files){
	
    	*files = (char**)malloc((MAX_FILES + 1) * sizeof(char*));
    	if (*files == NULL) {
        	return -1; 
    	}

   	int index = 0;
	int i;
    	for (i = 0; i < MAX_FILES; i++) {
        	if (DIR[i].used) {
            		(*files)[index] = (char*)malloc((strlen(DIR[i].name) + 1) * sizeof(char));
			strcpy((*files)[index], DIR[i].name);
            		index++;
        	}
    	}

    	(*files)[index] = NULL;

    	return 0; 
}

int fs_lseek(int fd, off_t offset){

	if(fildes[fd].used != 1 || offset > DIR[fildes[fd].file].size || offset < 0 ){
		return -1;
	}
	
	if(fd>32 || fd < 0){
		return -1;
	}
		
	fildes[fd].offset = offset;


	return 0;


}

int fs_truncate(int fd, off_t length){
	
	if (fd<0 || fd >32 || fildes[fd].used != 1){
		return -1;
	}

	int size = DIR[fildes[fd].file].size;
	int head = DIR[fildes[fd].file].head;

	if(size < length){
		return -1;
	}
	
	int numblock = length / BLOCK_SIZE;
	int offset = length % BLOCK_SIZE;

	int i;

	for(i = 0; i<numblock-1; i++){
		head = FAT[head];
	}			
	
	block_read(head, (char*) user_buf);
	for (i = offset+1; i<BLOCK_SIZE;i++){
		user_buf[i] = '\0';
	} 
	
	block_write(head, (char*) user_buf);
	for (i = 0; i<BLOCK_SIZE;i++){
                user_buf[i] = '\0';
        }
	int oldhead = head;
        int newhead = head;
        while(FAT[newhead] != -1 || FAT[newhead] != -2){
                block_write(newhead, user_buf);
                if (FAT[newhead] == -2){
                        break;
                }else{
                        newhead = FAT[newhead];
                        FAT[head] = -1;

                        
                        head = newhead;
                }
                
        }
	FAT[oldhead] = -2;       




/*
	int oldhead;
	int oldesthead = head;

	while(FAT[head] != -1 || FAT[head] != -2){
		block_write(head, user_buf);
		oldhead = head;
		head = FAT[head];
		FAT[oldhead] = -1;

	}

	FAT[oldesthead] = -2;
*/
	DIR[fildes[fd].file].size = length;


	return 0;

}












