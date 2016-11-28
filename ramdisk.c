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

int removeQueue(struct queue *q, struct node *filenode)	//DONE
{
	struct queuenode *prev = NULL;
	struct queuenode *curr = q->front;
	if(!filenode)
		return 0;
	if(isQueueEmpty(q))
		return 0;
	int size = sizeof(filenode);
	if(q->front->filenode == filenode && q->rear->filenode == filenode)
	{
		initializeQueue(q);
		return size;
	}
	if(q->front->filenode == filenode)
	{
		q->front = q->front->next;
		return size;
	}
	while(curr)
	{
		if(curr->filenode == filenode)
		{
			if(curr == q->rear)
			{
				prev->next = curr->next;
				q->rear = prev;
			}
			else
			{
				prev->next=curr->next;
			}
			return size;
		}
	}
	return 0;
}

int initRamdisk()
{
	#ifdef DEBUG
		printf("in init ramdisk\n");				
	#endif
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
	#ifdef DEBUG
		printf("in valid\n");				
	#endif
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
	#ifdef DEBUG
		printf("in get\n");				
	#endif
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
		token = strtok(NULL, "/");	
	}
	return head;
}

struct node * getParent(const char *path)
{
	#ifdef DEBUG
		printf("in getParent\n");				
	#endif
	char pathTemp[MAXLEN];
	strcpy(pathTemp, path);
	char *token = strtok(pathTemp, "/");
	if(!token && (strcmp(pathTemp, "/") == 0))
		return root->parent;
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
		token = strtok(NULL, "/");	
		if(strchr(token,'/') == NULL)
		{
			strcpy(file, token);
			return head;
		}
	}
	return NULL;
}

static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
	#ifdef DEBUG
		printf("in ramdisk getattr\n");				
	#endif
	struct node *tmpNode = get(path);
	if(!tmpNode)
		return -ENOENT;
	stbuf->st_atime = tmpNode->meta->st_atime;
    stbuf->st_mtime = tmpNode->meta->st_mtime;  
    stbuf->st_ctime = tmpNode->meta->st_ctime;            
	stbuf->st_nlink = tmpNode->meta->st_nlink;
	stbuf->st_mode  = tmpNode->meta->st_mode;	
	stbuf->st_uid   = tmpNode->meta->st_uid;
    stbuf->st_gid   = tmpNode->meta->st_gid;            
	stbuf->st_size = tmpNode->meta->st_size;
	return 0;
}

// static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
// 			 off_t offset, struct fuse_file_info *fi)
// {
// 	if(valid(path) != 0)
// 		return -ENOENT;
// 	struct node *dir = get(path);
// 	filler(buf, ".", NULL, 0);
// 	filler(buf, "..", NULL, 0);
// 	struct queuenode *head = dir->q->front;
// 	while(head)
// 	{
// 		filler(buf, head->filenode->filename, NULL, 0);
// 		head = head->next;
// 	}
// 	time_t currtime;
// 	time(&currtime);
// 	dir->meta->st_atime = currtime;
// 	return 0;
// }

static int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
	#ifdef DEBUG
		printf("in ramdisk open\n");				
	#endif
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	
	if(valid(path)==0)
		return 0;
	
	return -ENOENT;
}

static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	#ifdef DEBUG
		printf("in ramdisk read\n");				
	#endif
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
	#ifdef DEBUG
		printf("in ramdisk write\n");				
	#endif
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
			memfree = memfree - size;
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

static int ramdisk_mkdir(const char *path, mode_t mode)
{
	#ifdef DEBUG
		printf("in ramdisk mkdir\n");				
	#endif
	if(valid(path)!=0)
		return -ENOENT;

	struct node *parent = getParent(path);

	struct node *dir = (struct node *)malloc(sizeof(struct node));
	dir->meta = (struct stat *)malloc(sizeof(struct stat));
	if(!dir)
		return -1;
	memfree = memfree - sizeof(struct node) - sizeof(struct stat);
	if(memfree <  0)
		return -ENOSPC;
    
    strcpy(dir->filename, file);
    dir->parent = parent;        
	dir->isfile = 0;
	time_t currtime;
	time(&currtime);
	dir->meta = (struct stat *)malloc(sizeof(struct stat));
    dir->meta->st_atime = currtime;
    dir->meta->st_mtime = currtime;
    dir->meta->st_ctime = currtime;
    dir->meta->st_size = MAXLEN;
	dir->meta->st_nlink = 2;
    dir->meta->st_mode = S_IFDIR | 0755;
    dir->meta->st_uid = getuid();
    dir->meta->st_gid = getgid();
	parent->meta->st_nlink = parent->meta->st_nlink + 1;
	enQueue(parent->q, dir);
	return 0;
}


static int ramdisk_rmdir(const char *path)
{
	#ifdef DEBUG
		printf("in ramdisk rmdir\n");				
	#endif
	if(valid(path) != 0)
		return -ENOENT;
	
	struct node *temp;
	temp = get(path);
	if(isQueueEmpty(temp->q)!=1)
		return -ENOTEMPTY;
	struct node *father;
	father = temp->parent;	
	removeQueue(father->q, temp);		
	memfree = memfree + sizeof(temp);
	father->meta->st_nlink = father->meta->st_nlink-1;
	return 0;
}

static int ramdisk_opendir(const char *path, struct fuse_file_info *fi)
{
	#ifdef DEBUG
		printf("in ramdisk opendir\n");				
	#endif
	struct node *tempnode = get(path);
	if(!tempnode)
		return -ENOENT;
	if(tempnode->isfile == 1)
		return -ENOTDIR;
	if(tempnode->isfile == 1)
		return -1;
	return 0;
}


static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	#ifdef DEBUG
		printf("in ramdisk create\n");				
	#endif
	if(memfree < 0 )
		return -ENOSPC;
	struct node *parent = getParent(path);
	struct node *new = (struct node *)malloc(sizeof(struct node));
	
	time_t currtime;
	time(&currtime);
	strcpy(new->filename, file);
	
	new->isfile = 1;
	new->parent = parent;
	new->meta = (struct stat *)malloc(sizeof(struct stat));
	new->meta->st_atime = currtime;
	new->meta->st_mtime = currtime;
	new->meta->st_ctime = currtime;
	
	new->meta->st_size = 0;	
	new->meta->st_mode = S_IFREG | mode;
	new->meta->st_nlink = 1;
	new->meta->st_uid = getuid();
	new->meta->st_gid = getgid();  	
	new->content = NULL;
	long less = sizeof(new) + sizeof(stat);	
	memfree = memfree - less;
	if(memfree < 0)
		return -ENOSPC;
	enQueue(parent->q, new);
	return 0;
}


 static struct fuse_operations ramdisk_oper = {
// 	.readdir	= ramdisk_readdir,
 	.open		= ramdisk_open,
 	.getattr	= ramdisk_getattr,
 	.opendir	= ramdisk_opendir,
 	.read		= ramdisk_read,
 	.write		= ramdisk_write,
 	.mkdir		= ramdisk_mkdir,
 	.rmdir		= ramdisk_rmdir,
 	.create		= ramdisk_create,
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
    initRamdisk();
	fuse_main(argc, argv, &ramdisk_oper, NULL);
	return 0;
}