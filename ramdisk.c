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


int initRamdisk()
{
	printf("in init ramdisk\n");				
	root = (struct node *)malloc(sizeof(struct node));
	if(!root)
		return -ENOSPC;
	time_t currtime;
	time(&currtime);
	root->meta = (struct stat *)malloc(sizeof(struct stat));
	if(!root->meta)
		return -ENOSPC;
	strcpy(root->filename, "/");
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

char *getParentPath(const char *path)
{
	char *newfile = strrchr(path,"/");
	newfile++;
	size_t newsize = strlen(newfile);
	size_t pathsize = strlen(path);
	size_t parentsize = pathsize - newsize - 1;
	char *parentpath =  NULL;
	strncpy(parentpath, path, parentsize);
	return parentpath;
}

int valid(const char *path)
{
	// fprintf(stdout, "in valid\n");				
	// char pathTemp[MAXLEN];
	// strcpy(pathTemp, path);
	// char *token = strtok(pathTemp, "/");
	// if(!token && (strcmp(pathTemp, "/") == 0))
	// 	return 0;
	// struct node *head = root;
	// while(token)
	// {
	// 	struct queuenode *tempnode = head->q->front;
	// 	while(tempnode || strcmp(tempnode->filenode->filename, token)==0)
	// 	{
	// 		tempnode = tempnode->next;
	// 	}
	// 	if(!tempnode)
	// 		return -ENOENT;
	// 	head = tempnode->filenode;
	// }

	struct node *head = root;
	while(head)
	{
		if(strcmp(head->abs,path))
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
	// fprintf(stdout, "in get: %s\n",path);				
	// char pathTemp[MAXLEN];
	// strcpy(pathTemp, path);
	// char *token = strtok(pathTemp, "/");
	
	// if(!token && (strcmp(pathTemp, "/") == 0))
	// 	return root;
	// struct node *head = root;
	// printQ(head->q);
	// while(token)
	// {
	// 	fprintf(stdout, "in while\n");
	// 	if(!head->q)
	// 		return NULL;
	// 	struct queuenode *tempnode = head->q->front;
	// 	if(!tempnode)
	// 	{
	// 		fprintf(stdout, "queuenode does not exist in get\n");
	// 		return NULL;
	// 	}
	// 	if(!tempnode->filenode)
	// 	{
	// 		fprintf(stdout, "filenode does not exist in get\n");
	// 		return NULL;
	// 	}
	// 	fprintf(stdout, "entering second while in get\n");
	// 	fprintf(stdout, "%s\n",tempnode->filenode->filename);
	// 	while(tempnode)
	// 	{
	// 		if(strcmp(tempnode->filenode->filename, token)==0)
	// 		{
	// 			fprintf(stdout, "file found\n");
	// 			break;
	// 		}
	// 		fprintf(stdout, "%s\n",tempnode->filenode->filename);
	// 		fprintf(stdout,"in second while in get\n");
	// 		tempnode = tempnode->next;
	// 	}
	// 	if(!tempnode)
	// 		return NULL;
	// 	head = tempnode->filenode;
	// 	token = strtok(NULL, "/");	
	// }
	// fprintf(stdout, "exiting get\n");
	// return head;


	struct node *head= root;
	while(head)
	{
		if(strcmp(head->abs, path) == 0)
			return head;
	}
	return NULL;
}

// struct node * getParent(const char *path)
// {
// 		printf("in getParent\n");		
// 	char pathTemp[MAXLEN];
// 	strcpy(pathTemp, path);
// 	char *token = strtok(pathTemp, "/");
// 	if(!token && (strcmp(pathTemp, "/") == 0))
// 		return root->parent;
// 	struct node *head = root;
// 	while(token)
// 	{
// 		fprintf(stdout, "in while get parent\n");
// 		if(!head->q->front)
// 		{
// 			fprintf(stdout, "queue does not exist in getParent\n");
// 			strcpy(file, token);
// 			printf("%s\n",file);
// 			return head;
// 		}
// 		struct queuenode *tempnode = head->q->front;
// 		if(!tempnode)
// 		{
// 			fprintf(stdout, "queuenode does not exist in getParent\n");
// 			return head;
// 		}
// 		if(!tempnode->filenode)
// 		{
// 			fprintf(stdout, "filenode does not exist in getParent\n");
// 			return head;
// 		}
// 		while(tempnode )
// 		{

// 			if(!tempnode->filenode)
// 			{
// 				fprintf(stdout, "no filenode found in while get parent\n");
// 				break;
// 			}
// 			if(strcmp(tempnode->filenode->filename, token)==0)
// 			{
// 				fprintf(stdout, "file found\n");
// 				break;
// 			}
// 			tempnode = tempnode->next;
// 		}
// 		if(!tempnode)
// 			return NULL;
// 		head = tempnode->filenode;
// 		token = strtok(NULL, "/");	
// 		if(strchr(token,'/') == NULL)
// 		{
// 			strcpy(file, token);
// 			fprintf(stdout, "file name in parent: %s\n",file);
// 			fprintf(stdout, "file name in parent: %s\n", token);
// 			return head;
// 		}
// 	}
// 	return NULL;
// }

static int ramdisk_getattr(const char *path, struct stat *stbuf)
{
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
	return 0;
}

 // static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
 // 			 off_t offset, struct fuse_file_info *fi)
 // {
 // 	if(valid(path) != 0)
 // 		return -ENOENT;
 // 	struct node *dir = get(path);
 // 	filler(buf, ".", NULL, 0);
	// filler(buf, "..", NULL, 0);
 // 	struct queuenode *head = dir->q->front;
 // 	while(head)
 // 	{
	//  		filler(buf, head->filenode->filename, NULL, 0); 		head = head->next;
	// }
 // 	time_t currtime;
 // 	time(&currtime);
 // 	dir->meta->st_atime = currtime;
 // 	return 0;
 // }

// static int ramdisk_open(const char *path, struct fuse_file_info *fi)
// {
// 	#ifdef DEBUG
// 		printf("in ramdisk open\n");				
// 	#endif
// 	if ((fi->flags & 3) != O_RDONLY)
// 		return -EACCES;
	
// 	if(valid(path)==0)
// 		return 0;
	
// 	return -ENOENT;
// }

// static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
// 		      struct fuse_file_info *fi)
// {
// 	#ifdef DEBUG
// 		printf("in ramdisk read\n");				
// 	#endif
// 	size_t len;
// 	struct node *temp = get(path);
// 	if(!temp)
// 		return -ENOENT;

// 	if(temp->isfile == 0)	
// 		return -EISDIR;	
	
// 	len = temp->meta->st_size;
// 	if(offset < len) {
// 		if(offset + size > len)		
// 			size = len - offset;		
// 		memcpy(buf, temp->content + offset, size);
// 	} else	
// 		size = 0;
		
// 	if(size > 0) {
// 		time_t currtime;
// 		time(&currtime);
// 		temp->meta->st_atime = currtime;
// 	}
	
// 	return size;	
// }

// static int ramdisk_write(const char *path, const char *buf, size_t size, off_t offset,
//                       struct fuse_file_info *fi)
// {
// 	#ifdef DEBUG
// 		printf("in ramdisk write\n");				
// 	#endif
// 	if(size > memfree)
// 		return -ENOSPC;
// 	struct node *temp = get(path);
// 	size_t len;
// 	time_t currtime;
// 	long less;
// 	long less2;
// 	char *metadata = NULL;
	
// 	if(temp->isfile == 0)
// 		return -EISDIR;
	
// 	len = temp->meta->st_size;
	
// 	if(size > 0)
// 	{
// 		if(len == 0)
// 		{
// 			offset = 0;
// 			temp->content = (char *)malloc(sizeof(char) * size);
// 			memcpy(temp->content + offset, buf, size);
// 			temp->meta->st_size = offset +  size;
// 			memfree = memfree - size;
// 		}
// 		else
// 		{
// 			if(offset > len)
// 				offset = len;
// 			*metadata = (char *)realloc(temp->content, sizeof(char) * (offset+size));
// 			if(metadata == NULL)
// 				return -ENOSPC;
// 			else
// 			{
// 				temp->content = metadata;
// 				memcpy(temp->content + offset, buf, size);
					
// 				less = offset + size;
// 				less2 = len - less; 
// 				memfree = memfree + less2;
// 			}
// 		}
// 		time(&currtime);
//         temp->meta->st_ctime = currtime;
//         temp->meta->st_mtime = currtime;

// 	}
// 	return size;
// }

static int ramdisk_mkdir(const char *path, mode_t mode)
{
	fprintf(stdout, "mkdir: %s\n",path);
	char *parentPath = getParentPath(path);
	struct node *parent = get(parentPath);
	fprintf(stdout, "got parent\n");
	struct node *dir = (struct node *)malloc(sizeof(struct node));
	if(!dir)
		return -1;
	memfree = memfree - sizeof(struct node);
	if(memfree <  0)
		return -ENOSPC;
	if(!(dir->q))
	{
	    dir->q = (struct queue *)malloc(sizeof(struct queue));
	}
    	dir->parent = parent;        
	dir->isfile = 0;
	time_t currtime;
	time(&currtime);
	dir->meta = (struct stat *)malloc(sizeof(struct stat));
    /*dir->meta->st_atime = currtime;
    dir->meta->st_mtime = currtime;
    dir->meta->st_ctime = currtime;
    dir->meta->st_size = MAXLEN;
	dir->meta->st_nlink = 2;
    dir->meta->st_mode = S_IFDIR | 0755;
    dir->meta->st_uid = getuid();
    dir->meta->st_gid = getgid();
	parent->meta->st_nlink = parent->meta->st_nlink + 1;*/
	char file[256];
	char *token = strtok(path, "/");
	while(token)
	{
		strcpy(file, token);
		token = strtok(NULL, "/");
	}
	strcpy(dir->filename, file);
	//fprintf(stdout, "dir name to be given: %s\n",file);
    //	fprintf(stdout, "dir name in mkdir: %s\n",dir->filename);
	//fprintf(stdout, "going into enqueue\n");
	insert(dir);
	//printQ(parent->q);
	return 0;
}


// static int ramdisk_rmdir(const char *path)
// {
// 	#ifdef DEBUG
// 		printf("in ramdisk rmdir\n");				
// 	#endif
// 	if(valid(path) != 0)
// 		return -ENOENT;
	
// 	struct node *temp;
// 	temp = get(path);
// 	if(isQueueEmpty(temp->q)!=1)
// 		return -ENOTEMPTY;
// 	struct node *father;
// 	father = temp->parent;	
// 	removeQueue(&father->q, temp);		
// 	memfree = memfree + sizeof(temp);
// 	father->meta->st_nlink = father->meta->st_nlink-1;
// 	return 0;
// }

// static int ramdisk_opendir(const char *path, struct fuse_file_info *fi)
// {
// 	#ifdef DEBUG
// 		printf("in ramdisk opendir\n");				
// 	#endif
// 	struct node *tempnode = get(path);
// 	if(!tempnode)
// 		return -ENOENT;
// 	if(tempnode->isfile == 1)
// 		return -ENOTDIR;
// 	if(tempnode->isfile == 1)
// 		return -1;
// 	return 0;
// }


// static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
// {
// 	#ifdef DEBUG
// 		printf("in ramdisk create\n");				
// 	#endif
// 	if(memfree < 0 )
// 		return -ENOSPC;
// 	struct node *parent = getParent(path);
// 	struct node *new = (struct node *)malloc(sizeof(struct node));
	
// 	time_t currtime;
// 	time(&currtime);
// 	strcpy(new->filename, file);
	
// 	new->isfile = 1;
// 	new->parent = parent;
// 	new->meta = (struct stat *)malloc(sizeof(struct stat));
// 	new->meta->st_atime = currtime;
// 	new->meta->st_mtime = currtime;
// 	new->meta->st_ctime = currtime;
	
// 	new->meta->st_size = 0;	
// 	new->meta->st_mode = S_IFREG | mode;
// 	new->meta->st_nlink = 1;
// 	new->meta->st_uid = getuid();
// 	new->meta->st_gid = getgid();  	
// 	new->content = NULL;
// 	long less = sizeof(new) + sizeof(stat);	
// 	memfree = memfree - less;
// 	if(memfree < 0)
// 		return -ENOSPC;
// 	enQueue(&parent->q, new);
// 	return 0;
// }

/*static int ramdisk_rename(const char *from, const char *to)
{
	int isValid, isValid2;
	isValid = validatePath(from);
	isValid2 = validatePath(to);
	struct node *from  = NULL;
	struct node *to = NULL;
	struct node *toParent = NULL;	
	char new_file_name[256];
	time_t currtime;
	
	if(valid(from )!=0)
		return -ENOENT;

	from = get(from);
				
	strcpy(new_file_name, file);

	if(valid(to)==0)
	{
		memset(to->filename,0,255);
		strcpy(to->filename, new_file_name);
		time(&currtime);
		to->meta->st_atime = currtime;
		to->meta->st_mtime = currtime;
		to->meta->st_ctime = currtime;
		return 0;
	}

	if(valid(to) != 0) 
	{
		//to path not already present. Then create one
		toParent = getParent(to);
		to = (struct node *)malloc(sizeof(to));
		to->meta = (struct node *stat)malloc(sizeof(struct));
		memfree = memfree - sizeof(to) - sizeof(to->meta);
		if(memfree < 0)
			return -ENOSPC;
		if(ptr->isFile == 1 ) 
		{
			to->isfile = 1;
		}
		if(ptr->isFile != 1) 
		{
			to->isfile=0;
		}
		memset(to->filename, 0, 255);
		strcpy(to->filename, new_file_name);
		to->parent = toParent;
		to->meta = from->meta;
		to->meta->st_atime = currtime;
		to->meta->st_mtime = currtime;
		to->meta->st_ctime = currtime;
		enQueue(&toParent->q, to);
		if(ramdisk_unlink(from) == 0)
			return 0;
	}
	
	return -1;	
}
*/
// static int ramdisk_unlink(const char *path)
// {
// 	if(valid(path) != 0)
// 		return -ENOENT;	

// 	struct node *del = get(path);
// 	struct node *delParent = getParent(path);
// 	del->parent = NULL;
// 	removeQueue(&delParent->q, del);
// 	long more = sizeof(del)+sizeof(del->meta);
// 	memfree = memfree + more;
// 	del->meta = NULL;
// 	del = NULL;
// 	return 0;
// }

/*static int ramdisk_utime(const char *path, struct utimbuf *ubuf)
{
	return 0;
}
*/
/*static int ramdisk_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	return 0;
}
*/
 static struct fuse_operations ramdisk_oper = {
/// 	.readdir	= ramdisk_readdir,
// 	.open		= ramdisk_open,
 	.getattr	= ramdisk_getattr,
 	//.opendir	= ramdisk_opendir,
 //	.read		= ramdisk_read,
 	//.write		= ramdisk_write,
 	.mkdir		= ramdisk_mkdir,
// 	.rmdir		= ramdisk_rmdir,
 //	.create		= ramdisk_create,
 	//.unlink		= ramdisk_unlink, 
// 	.rename		= ramdisk_rename,
 //	.utime	    = ramdisk_utime,
 //	.fsync	    = ramdisk_fsync 
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
    printf("before init ramdisk\n");
    int out = initRamdisk();
    printf("%d\n",out);
    printf("after init ram\n");
	fuse_main(argc, argv, &ramdisk_oper, NULL);
	return 0;
}
