#define FUSE_USE_VERSION 30

//#include <config.h>
#include "fuse.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#define MAXLEN 4096

int totalnodes=0;
char file[256];

struct node *root;
long memfree;


struct node
{
	char filename[256];
	int isfile;
	struct queue *q;
	//struct node *subdir;
	//struct node *next;
	struct node *parent;
	struct stat *meta;
	char *content;
};

struct queuenode
{
	struct node *filenode;
	struct queuenode *next;
};

struct queue
{
	struct queuenode *front;
	struct queuenode *rear;
};

void initializeQueue(struct queue *q)		
{
  q->front = NULL; 
  q->rear = NULL;
}

int isQueueEmpty(struct queue *q)	//DONE
{
	return (q->front == NULL && q->rear == NULL)? 1 : 0;
}

void enQueue(struct queue *q, struct node *filenode)	//DONE
{
	struct queuenode *tmpNode = (struct queuenode *)malloc(sizeof(struct queuenode));
	tmpNode->filenode = filenode;
	tmpNode->next = NULL;
	if(isQueueEmpty(q))
	{
		q->front = tmpNode;
		q->rear = tmpNode;
	}
	else
	{
		q->rear->next = tmpNode;
		q->rear = tmpNode;
	}
}


int initRamdisk()
{
	root = (struct node *)malloc(sizeof(struct node));
	if(!root)
		return -ENOSPC;
	time_t currtime;
	time(&currtime);
	root->meta = (struct stat *)malloc(sizeof(struct stat));
	root->isfile = 0;
	root->parent = NULL;
	initializeQueue(root->q);
	root->meta->st_mode = S_IFDIR | 0755;
	root->meta->st_nlink = 2;
    root->meta->st_uid = 0;
    root->meta->st_gid = 0;
    root->meta->st_atime = currtime;
    root->meta->st_mtime = currtime;
    root->meta->st_ctime = currtime;
	long used = sizeof(root) + sizeof(stat);
	memfree = memfree - used; 
	if(memfree < 0)
	{
		root = NULL;
		return -ENOSPC;
	}
	return 0;   	
}

int valid(const char *path)
{
	char pathTemp[MAXLEN];
	strcpy(pathTemp, path);
	char *token = strtok(pathTemp, "/");
	if(!token && (strcmp(pathTemp, "/") == 0))
		return 0;
	struct node *head = root;
	while(token)
	{
		struct queuenode *tempnode = head->q->front;
		while(tempnode || strcmp(tempnode->filenode->filename, token)==0)
		{
			tempnode = tempnode->next;
		}
		if(!tempnode)
			return -ENOENT;
		head = tempnode->filenode;
	}
	return 0;
}


struct node * get(const char *path)
{
	char pathTemp[MAXLEN];
	strcpy(pathTemp, path);
	char *token = strtok(pathTemp, "/");
	if(!token && (strcmp(pathTemp, "/") == 0))
		return root;
	struct node *head = root;
	while(token)
	{
		struct queuenode *tempnode = head->q->front;
		while(tempnode || strcmp(tempnode->filenode->filename, token)==0)
		{
			tempnode = tempnode->next;
		}
		if(!tempnode)
			return NULL;
		head = tempnode->filenode;	
	}
	return head;
}



static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
	struct node *temp = NULL;
	struct node *filenode = get(path);
	if(!filenode)
		return -ENOENT;
	if(filenode->isfile == 0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(filenode->content);
	}
	return 0;
}

// static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
// 			 off_t offset, struct fuse_file_info *fi)
// {
// 	if(validdir(path) != 0)
// 		return -ENOENT;
// 	struct node *temp = getdir(path);
// 	struct node *it = temp->subdir;
// 	filler(buf, ".", NULL, 0);
// 	filler(buf, "..", NULL, 0);
// 	while(it)
// 	{
// 		filler(buf, it->name, NULL, 0);
	
// 	}
// 	time_t currtime;
// 	time(&currtime);
// 	temp->meta->st_atime = currtime;
// 	return 0;
// }

static int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	
	if(valid(path)==0)
		return 0;
	
	return -ENOENT;
}

static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	struct node *temp = get(path);
	if(!temp)
		return -ENOENT;

	if(temp->isfile == 0)	
		return -EISDIR;	
	
	len = temp->meta->st_size;
	if(offset < len) {
		if(offset + size > len)		
			size = len - offset;		
		memcpy(buf, temp->content + offset, size);
	} else	
		size = 0;
		
	if(size > 0) {
		time_t currtime;
		time(&currtime);
		temp->meta->st_atime = currtime;
	}
	
	return size;	
}

static int ramdisk_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	if(size > memfree)
		return -ENOSPC;
	struct node *temp = get(path);
	size_t len;
	time_t currtime;
	long less;
	long less2;
	char *metadata = NULL;
	if(temp->isfile == 0)
		return -EISDIR;
	len = temp->meta->st_size;
	if(size > 0)
	{
		if(len == 0)
		{
			offset = 0;
			temp->content = (char *)malloc(sizeof(char) * size);
			memcpy(temp->content + offset, buf, size);
			temp->meta->st_size = offset +  size;
			memfree = memfree + size;
		}
		else
		{
			if(offset > len)
				offset = len;
			*metadata = (char *)realloc(temp->content, sizeof(char) * (offset+size));
			if(metadata == NULL)
				return -ENOSPC;
			else
			{
				temp->content = metadata;
				memcpy(temp->content + offset, buf, size);
					
				less = offset + size;
				less2 = len - less; 
				memfree = memfree + less2;
			}
		}
		time(&currtime);
        temp->meta->st_ctime = currtime;
        temp->meta->st_mtime = currtime;

	}
	return size;
}

// void insert(struct node **temp, struct node **dir)
// {
// 	if((*temp)->subdir) {
// 		struct node *it;
//                 for(it = (*temp)->subdir; it->next != NULL; it = it->next)
//                 	;
//                 it->next = dir;			
// 	}
// 	else
// 		(*temp)->subdir = (*temp);
// }




// static int ramdisk_mkdir(const char *path, mode_t mode)
// {
// 	struct node *temp = get(path);
// 	struct node *dir = (struct node *)malloc(sizeof(struct node));
// 	dir->meta = (struct stat *)malloc(sizeof(struct stat));
// 	if(!dir)
// 		return -1;
// 	memfree = memfree - (sizeof(struct node) - sizeof(struct stat));
// 	if(memfree < = 0)
// 		return -ENOSPC;
// 	time_t currtime;
//     time(&currtime);
//     strcpy(*dir->name, nFile);
        
// 	dir->isfile = 0;
// 	time(&currtime);
//     dir->meta->st_atime = currtime;
//     dir->meta->st_mtime = currtime;
//     dir->meta->st_ctime = currtime;
//     dir->parent = temp;
//     dir->subdir = NULL;
//     dir->next = NULL;
//     dir->meta->st_size = MAXLEN;
// 	dir->meta->st_nlink = 2;
//     dir->meta->st_mode = S_IFDIR | 0755;
//     dir->meta->st_uid = getuid();
//     dir->meta->st_gid = getgid();
// 	insert(&temp, &dir);
// 	temp->meta->st_nlink = temp->meta->st_nlink + 1;
// 	return 0;
// }

// void rmutil(struct node **father, struct node **temp)
// {
// 	if((*father)->subdir == (*temp))
//         {
//         	if((*temp)->next == NULL)
//         	{
//         		//delete the lone leaf directory		
//                 	(*father)->subdir = NULL;
//                 }
//                 else
//                 {
//                 	//make super struct struct node point to 2nd struct struct node in line
//                 	(*father)->subdir = (*temp)->next;
//                 }
//         }		  
//         else
//         {
//                 struct struct node *it;
//                 for(it = (*father)->subdir; it != NULL; it = it->next)
//                 {
//                         if(it->next == (*temp))
//                         {
//                                 it->next = (*temp)->next;
//                                 break;
//                         }
//                 }
//         }
// }

// static int ramdisk_rmdir(const char *path)
// {
// 	if(valid(path) != 0)
// 		return -ENOENT;
	
// 	struct node *temp;
// 	temp = get(path);
// 	if(temp->subdir)
// 		return -ENOTEMPTY;
// 	struct node *father;
// 	father = temp->parent;	
// 	rmUtil(&father, &temp);		
// 	free(father->meta);
// 	free(temp);
// 	memfree = memfree + (sizeof(struct node) + sizeof(struct stat));
// 	father->meta->st_nlink = father->meta->st_nlink-1;
// 	return 0;
// }

static int ramdisk_opendir(const char *path, struct fuse_file_info *fi)
{
	struct node *tempnode = get(path);
	if(!tempnode)
		return -ENOENT;
	if(tempnode->isfile == 1)
		return -ENOTDIR;
	if(tempnode->isfile == 0)
		return 0
}

// static int ramdisk_unlink(const char *path)
// {
// 	if(valid(path) != 0) {
// 		return -ENOENT;	
// 	struct node *temp, *father;
// 	temp = get(path);
// 	father = temp->parent;
		
// 	rmUtil(&father, &temp);
	
// 	if(temp->meta->st_size == 0)
// 	{
// 		free(temp->meta);
// 		free(temp);
// 	}
// 	else
// 	{	
// 		memfree = memfree + temp->meta->st_size;
// 		free(temp->content);
// 		free(temp->meta);
// 		free(temp);
// 	}	
	
// 	long more = (sizeof(struct node) + sizeof(struct stat));
// 	memfree = memfree + more;
// 	return 0;
// }



// static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
// {
// 	if(memfree < 0 )
// 		return -ENOSPC;
	
// 	struct node *temp = getPath(path);	
// 	struct node *new = (struct node *)malloc(sizeof(struct node));
	
// 	time_t currtime;
// 	time(&currtime);
// 	strcpy(new->name, nFile);
	
// 	new->isfile = 1;
	
// 	new->meta->st_atime = currtime;
// 	new->meta->st_mtime = currtime;
// 	new->meta->st_ctime = currtime;
	
// 	new->meta->st_size = 0;	
// 	new->parent = temp;
// 	new->subdir = NULL;
// 	new->next = NULL;
	
// 	new->meta->st_mode = S_IFREG | mode;
// 	new->meta->st_nlink = 1;
// 	new->meta->st_uid = getuid();
// 	new->meta->st_gid = getgid();  	
// 	new->content = NULL;
// 	long less = sizeof(new) + sizeof(stat);	
// 	memfree = memfree - less;
// 	if(memfree < 0)
// 		return -ENOSPC;
// 	insert(&temp, &new);
// 	return 0;
// }

// static int ramdisk_utime(const char *path, struct utimbuf *ubuf)
// {
// 	return 0;
// }

//TODO modify
// static int ramdisk_rename(const char *from, const char *to)
// {	

// 	struct node *ptr = NULL;
// 	struct node *temp = NULL;
// 	char newfilename[256];		
// 	if(valid(from) == 0) 
// 	{
// 		ptr = getPath(from);
// 		temp = getPath(to);
				
// 		strcpy(new_file_name, ptr->filename);
// 		if(valid(to) != 0) 
// 		{	//to path not already present. Then create one
// 			temp = (struct node *)malloc(sizeof(struct node));
// 			if(ptr->isfile == 1 ) 
// 			{
// 				memset(ptr->name, 0, 255);
// 				strcpy(ptr->name, new_file_name);
// 				return 0;
// 			}
// 			if(temp->isfile != 1) 
// 			{
// 				ramdisk_create(to, ptr->nMeta->st_mode, NULL);
// 				struct node *new_node  = getPath(to);
				
// 			    setParamsForRename(&new_node, ptr);
// 				if(setSizeForRename(&new_node, ptr) == -1)
// 					return -ENOSPC;
				    		
// 				ramdisk_unlink(from);
// 				return 0;
// 			}			
// 			else
// 				return -ENOENT;
// 		}
// 		else if(valid(to) == 0) 
// 		{	//to path already present.
// 			if(temp->isfile == 1) 
// 			{
// 				int fromSize, toSize;
// 				setParamsForRename(&temp, ptr);
// 				fromSize = ptr->meta->st_size;
// 				toSize = temp->meta->st_size;
// 				memset(temp->content, 0, toSize);
				
// 				if(setSizeForRename2(&temp, ptr, fromSize) == -1)
// 					return -ENOSPC;
				
// 				memset(temp->content, 0, toSize);
				
// 				ramdisk_unlink(from);
// 				return 0;
// 		        }
// 		}
// 		else
// 			return -ENOENT;	
// 	}
// 	else
// 		return -ENOENT;
//     return 0;
// }

 static struct fuse_operations ramdisk_oper = {
 	.getattr	= ramdisk_getattr,
// 	.readdir	= ramdisk_readdir,
 	.open		= ramdisk_open,
 	.opendir	= ramdisk_opendir,
 	.read		= ramdisk_read,
 	.write		= ramdisk_write,
// 	.mkdir		= ramdisk_mkdir,
// 	.rmdir		= ramdisk_rmdir,
// 	.create		= ramdisk_create,
// 	.unlink		= ramdisk_unlink, 
// 	.rename		= ramdisk_rename, 
 };



int main(int argc, char *argv[])
{
	if( argc != 3)
    {
        printf("Invalid number of arguments\n");	//todo too many args
        return -1;
    }
    memfree = (long) atoi(argv[2]);
    memfree = memfree * 1024 * 1024;
    argc = argc - 1;
	fuse_main(argc, argv, &ramdisk_oper, NULL);
	return 0;
}