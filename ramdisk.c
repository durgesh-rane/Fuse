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
	//printf("in init ramdisk\n");				
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
	{
		fprintf(stdout, "returing root in getParent\n");
		return "/";
	}
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
	head = head->next;
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

 static int ramdisk_rmdir(const char *path)
{
 	struct node *curr = root;
	struct node *rm = get(path);
	fprintf(stdout, "rmdir path\n",rm->abs);
	struct node *prev = NULL;
	while(curr)
	{
		if(curr == rm)
		{
			prev->next = curr->next;
			fprintf(stdout, "rmdir successful\n");
			return 0;
		}
		prev = curr;
		curr = curr->next;
	}	
	fprintf(stdout, "rmdir no path found\n");
	return -ENOENT;
}

 static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
 		      struct fuse_file_info *fi)
 {
	/*fprintf(stdout, "in ramdisk read\n");
 	size_t len;
 	struct node *temp = get(path);
	fprintf(stdout, "ramdisk read filename: %s\n",temp->abs);
 	if(!temp)
 		return -ENOENT;

 	if(temp->isfile == 0)	
 		return -EISDIR;	
	if(temp->content == NULL)
	{
		size = 0;
	}
	else
	{
		fprintf(stdout, "ramdisk read in else\n");
		len = strlen(temp->content);
		if(offset <  len)
		{
			if(offset + size > len)
			{
				size = len - offset;
			}
			memcpy(buf, temp->content + offset, size);
		}
		else
		{
			size = 0;
		}
			
	}
	fprintf(stdout, "ramdisk read buffer: %s\n",buf);
	fprintf(stdout, "out ramdisk read\n");
	fprintf(stdout, "ramdisk read size: %d\n",size);
 	return size;
	*/
	size_t len;
 	struct node *temp = get(path);
 	if(!temp)
 		return -ENOENT;

 	if(temp->isfile == 0)	
 		return -EISDIR;	
	if(temp->content == NULL)
	{
		size = 0;
	}
	else
	{
		len = strlen(temp->content);
		if(offset <  len)
		{
			if(offset + size > len)
			{
				size = len - offset;
			}
			memcpy(buf, temp->content+ offset, size);
			buf[size]='\0';
		}
		else
		{
			size = 0;
		}
			
	}
	
return size; 	
 }

 static int ramdisk_write(const char *path, const char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi)
 {
	/*fprintf(stdout, "in ramdisk write\n");
 	int len;
	char *write = NULL;
	int modsize = 0;
	int diff = 0;
	int i=0;
	int j=0;
 	struct node *temp = get(path);
	fprintf(stdout, "write filename: %s\n",temp->abs);
 	if(!temp)
 		return -ENOENT;

 	if(temp->isfile == 0)	
 		return -EISDIR;
	
	if(temp->content  == NULL)
	{
		len = strlen(buf);
		write = malloc(size+1);
		memset(write, 0, size+1);
		memcpy(write, buf, size);
		write[size]='\0';
		temp->content = malloc(size+1);
		strcpy(temp->content, write);
	}
	else
	{
		modsize = offset + size;
		len = strlen(temp->content);
		if(modsize>len)
		{
			diff = modsize - len;
			write = malloc(modsize+1);
			memset(write, 0 , modsize+1);
			strncpy(write, temp->content, len);
		}
		for(i=offset; i<(offset+size);i++)
		{
			write[i]=buf[j];
			j++;
		}
		write[i]='\0';
		strcpy(temp->content, write);
	}
	fprintf(stdout, "out ramdisk write\n");
	fprintf(stdout, "ramdisk write content: %s\n",temp->content);
	fprintf(stdout, "ramdisk write size: %d\n", size);
 	return size;
	*/
	int len;
	char *write = malloc(size+1);
	int modsize = 0;
	int diff = 0;
	int i=0;
	int j=0;
 	struct node *temp = get(path);
 	if(!temp)
 		return -ENOENT;

 	if(temp->isfile == 0)	
 		return -EISDIR;
	
	if(temp->content  == NULL)
	{
		len = strlen(buf);
		strncpy(write, buf, size);
		write[size]='\0';
		temp->content = write;
	}
	else
	{
		modsize = offset + size;
		len = strlen(temp->content);
		if(modsize>len)
		{
			diff = modsize - len;
			strncpy(write, temp->content, len);
			write[len]='\0';
			temp->content = write;
		}
		for(i=offset; i<(offset+size);i++)
		{
			temp->content[i]=buf[j];
			j++;
		}
		temp->content[i]='\0';
	}
return size;
 }



static int ramdisk_utimens(const char *path, const struct timespec tv[2])
 {
 	return 0;
 }

static int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int ramdisk_rename(const char *from, const char *to)
{
	struct node *fromnode = get(from);
	struct node *tonode = get(to);
	if(!fromnode)
		return -ENOENT;
	if(fromnode->isfile!=1)
		return -EISDIR;
	char tempTo[256];
	strcpy(tempTo, to);
	fprintf(stdout, "tempTo: %s\n",tempTo);
	char *token = strtok(tempTo,"/");
	fprintf(stdout, "after token\n");
	char file[256];
	strcpy(file, token);
	fprintf(stdout,"printing\n");
	fprintf(stdout, "token :%s\n",token);
	fprintf(stdout,"file: %s\n", file);
	while(token)
	{
		fprintf(stdout, "token: %s\n",token);
		strcpy(file, token);
		fprintf(stdout, "file: %s\n",file);
		token = strtok(NULL,"/");
	}
	strcpy(fromnode->filename, file);
	strcpy(fromnode->abs, to);
	return 0;
}

static int ramdisk_release(const char *path, struct fuse_file_info *fi)
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
	.rmdir		= ramdisk_rmdir,
	.unlink		= ramdisk_rmdir,
	.read		= ramdisk_read,
	.write		= ramdisk_write,
	.rename		= ramdisk_rename,
	.release	= ramdisk_release,
 };



int main(int argc, char *argv[])
{
	if( argc != 3)
    {
        printf("Invalid number of arguments\n");	//todo too many args
        //return -1;
    }
    memfree = (long) atoi(argv[4]);
    memfree = memfree * 1024 * 1024;
    argc = argc - 1;
    int out = initRamdisk();
    fuse_main(argc, argv, &ramdisk_oper, NULL);
	return 0;
}
