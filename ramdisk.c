#define FUSE_USE_VERSION 30

//#include <config.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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
	char abs[1024];
	int isfile;
	struct node *next;
	struct node *parent;
	mode_t mode;
	char *content;
};

void printlist()
{
	struct node *head = root;
	while(head)
	{
		fprintf(stdout, "name: %s---%s\n",head->filename,head->abs);
		head = head->next;
	}
}
int initRamdisk()
{
	printf("in init ramdisk\n");				
	root = (struct node *)malloc(sizeof(struct node));
	if(!root)
		return -ENOSPC;
	time_t currtime;
	time(&currtime);
	strcpy(root->filename, "/");
	strcpy(root->abs,"/");
	fprintf(stdout, "%s\n",root->filename);
	root->isfile = 0;
	root->parent = NULL;
	root->next = NULL;
	root->mode = 0755;
	long used = sizeof(root);
	memfree = memfree - used; 
	if(memfree < 0)
	{
		root = NULL;
		return -ENOSPC;
	}
}
char *getParentPath(const char *path)
{
	fprintf(stdout, "in getParent path: %s\n",path);
	char *newfile = strrchr(path,'/');
	newfile++;
	fprintf(stdout, "%s\n",newfile);
	size_t newsize = strlen(newfile);
	size_t pathsize = strlen(path);
	size_t parentsize = pathsize - newsize-1;
	if(parentsize==0)
		return "/";
	char *parentpath = malloc(parentsize);
	fprintf(stdout, "%d %d %d\n",newsize,pathsize,parentsize);
	strncpy(parentpath, path, parentsize);
	parentpath[parentsize]='\0';
	fprintf(stdout,"in get parent: parentpath: %s\n",parentpath);
	return parentpath;
}

int valid(const char *path)
{
	

	struct node *head = root;
	while(head)
	{
		if(strcmp(head->abs,path) == 0)
			return 0;
		head = head->next;
	}
	return -ENOENT;
}

void insert(struct node *filenode)
{
	struct node *head = root;
	while(head->next)
	{
		head = head->next;
	}
	head->next = filenode;
}


struct node * get(const char *path)
{

	struct node *head= root;
	while(head)
	{
		if(strcmp(head->abs, path) == 0)
			return head;
		head= head->next;
	}
	return NULL;
}


static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
	fprintf(stdout, "in getattr\n");
	struct node *tmpNode = get(path);
	if(!tmpNode)
	{
		
		return -ENOENT;
	}
	if(tmpNode->isfile == 1)
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
	}
	else
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	fprintf(stdout, "leaving getattr\n");
	return 0;
}

static int ramdisk_mkdir(const char *path, mode_t mode)
{
	fprintf(stdout, "mkdir: %s\n",path);
	char *parentPath = getParentPath(path);
	fprintf(stdout, "in ramdisk mkdir: %s\n",parentPath);
	struct node *parent = get(parentPath);
	fprintf(stdout, "%s\n",parent->abs);
	fprintf(stdout, "got parent\n");
	struct node *dir = (struct node *)malloc(sizeof(struct node));
	if(!dir)
		return -1;
	memfree = memfree - sizeof(struct node);
	if(memfree <  0)
		return -ENOSPC;
    	dir->parent = parent;        
	dir->isfile = 0;
	dir->mode = mode;
	char file[256];
	char pathTemp[1024];
	strcpy(pathTemp, path);
	char *token = strtok(pathTemp, "/");
	while(token)
	{
		strcpy(file, token);
		token = strtok(NULL, "/");
	}
	strcpy(dir->filename, file);
	fprintf(stdout,"in mkdir orig path: %s\n",path);
	strcpy(dir->abs, path);
	fprintf(stdout,"in mkdir abs path: %s---%s\n",dir->abs,path);
	insert(dir);
	fprintf(stdout, "leaving mkdir\n");
	printlist();
	return 0;
}

static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	fprintf(stdout, "create: %s\n",path);
	char *parentPath = getParentPath(path);
	fprintf(stdout, "in ramdisk create: %s\n",parentPath);
	struct node *parent = get(parentPath);
	fprintf(stdout, "%s\n",parent->abs);
	fprintf(stdout, "got parent\n");
	struct node *file = (struct node *)malloc(sizeof(struct node));
	if(!file)
		return -1;
	memfree = memfree - sizeof(struct node);
	if(memfree <  0)
		return -ENOSPC;
    	file->parent = parent;        
	file->isfile = 1;
	file->mode = mode;
	char filename[256];
	char pathTemp[1024];
	strcpy(pathTemp, path);
	char *token = strtok(pathTemp, "/");
	while(token)
	{
		strcpy(filename, token);
		token = strtok(NULL, "/");
	}
	strcpy(file->filename, filename);
	strcpy(file->abs, path);
	insert(file);
	fprintf(stdout, "leaving create\n");
	return 0;
}

 static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
  			 off_t offset, struct fuse_file_info *fi)
  {
  	if(valid(path) != 0)
  		return -ENOENT;
  	struct node *dir = get(path);
  	filler(buf, ".", NULL, 0);
	 filler(buf, "..", NULL, 0);
	struct node *head = root;
	char *subdir = malloc(1024);

  	while(head)
  	{
			
			subdir = getParentPath(head->abs);
			if(strcmp(path, subdir)==0)
			{
				fprintf(stdout, "read dir: %s\n",head->filename);				
	 			filler(buf, head->filename, NULL, 0);
			} 		
			head = head->next;
	 }
	fprintf(stdout, "out read dir\n");
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

static int ramdisk_utimens(const char *path, const struct timespec tv[2])
 {
 	return 0;
 }

static struct fuse_operations ramdisk_oper = {
 	.getattr	= ramdisk_getattr,
 	.mkdir		= ramdisk_mkdir,
	.readdir	= ramdisk_readdir,
	.opendir	= ramdisk_opendir,
	.utimens	= ramdisk_utimens,
	.create		= ramdisk_create,
	.open		= ramdisk_open,
 };



int main(int argc, char *argv[])
{
	if( argc != 3)
    {
        printf("Invalid number of arguments\n");	//todo too many args
        //return -1;
    }
    printf("in main\n");
    memfree = (long) atoi(argv[4]);
    memfree = memfree * 1024 * 1024;
    argc = argc - 1;
    int out = initRamdisk();
    printf("%d\n",out);
    printf("after init ram\n");
    fuse_main(argc, argv, &ramdisk_oper, NULL);
	return 0;
}
