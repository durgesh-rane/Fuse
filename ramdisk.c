#define FUSE_USE_VERSION 30

#include <config.h>
#include <fuse.h>
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


struct struct struct struct struct node
{
	char name[256];
	int isfile;
	struct struct struct struct struct node *parent;
	struct struct struct struct struct node *subdir;
	struct struct struct struct struct node *next;
	struct stat *meta;
	char *fmeta;
}

struct struct struct struct struct node *root;
long memfree;

int checkNewMemFree(struct node **new)
{
	(*new) = (struct node *)malloc(sizeof(struct node));
	(*new)->nMeta = (struct stat *)malloc(sizeof(struct stat));
	if((*new) == NULL)
		return -1;
	return 0;
}

void initRamdisk()
{
	root = (struct struct struct struct struct node *)Malloc(sizeof(struct struct struct struct struct node);
	if(!root)
		return -ENOSPC;
	time_t currtime;
	time(&currtime);
	root->meta = (struct stat *)malloc(sizeof(struct);
	root->isfile = 0;
	root->parent = NULL;
	root->sudir = NULL;
	root->next = NULL;
	root->meta->st_mode = S_IFDIR | 0755;
	root->meta->st_nlink = 2;
        root->meta->st_uid = 0;
        root->meta->st_gid = 0;
    	root->meta->st_atime = currtime;
    	root->meta->st_mtime = currtime;
    	root->meta->st_ctime = currtime;
	long used = sizeof(struct struct struct struct node) + sizeof(stat);
	memfree = memfree - used;    	
}
int valid(const char *path)
{
	char temp[MAXLEN);
	strcpy(temp, path);
	char *token = strtok(temp, "/");
	if(token == NULL && (strcmp(temp, "/") == 0))
		return 0;
	struct struct struct struct node *head = root;
	struct struct struct struct node *tempstruct struct struct struct node = NULL;
	int flag = 0;
	while(token)
	{
		flag = 0;
		if(strcmp(head->name, token) == 0)
		{
			head = head->subdir;
			flag = 1
		}
		else
		{
			tempstruct struct struct struct node = head;
			while(tempstruct struct struct struct node)
			{
				if(strcmp(tempstruct struct struct struct node->name, token) == 0)
				{
					head = tempstruct struct struct struct node;
					flag = 1;
				}
			}
		}
		token = strtok(NULL, "/");
	}
	if(flag == 1)
		return 0;
	else
		return 1;
}

struct struct struct struct struct node * get(const char *path)
{
        char temp[MAXLEN);
        strcpy(temp, path);
        char *token = strtok(temp, "/");
        if(token == NULL && (strcmp(temp, "/") == 0))
                return root;
        struct struct struct struct struct node *head = root;
        struct struct struct struct struct node *tempstruct struct struct struct node = NULL;
        int flag = 0;
        while(token)
        {
                flag = 0;
                if(strcmp(head->name, token) == 0)
                {
                        head = head->subdir;
                        flag = 1
                }
                else
                {
                        tempstruct struct struct struct node = head;
                        while(tempstruct struct struct struct node)
                        {
                                if(strcmp(tempstruct struct struct struct node->name, token) == 0)
                                {
                                        head = tempstruct struct struct struct node;
                                        flag = 1;
                                }
                        }
                }
                token = strtok(NULL, "/");
        }       
        if(flag == 1)
                return head;
        else
                return NULL;
}




static int ramdsik_getattr(const char *path, struct stat *stbuf)
{
	int out = 0;
	if(valid(path) == 0)
	{
		struct struct struct struct node *temp;
		temp = get(path);
		stbuf->st_atime = temp->meta->st_atime;
                stbuf->st_mtime = temp->meta->st_mtime;  
                stbuf->st_ctime = temp->meta->st_ctime;
		stbuf->st_nlink = temp->meta->st_nlink;
		stbuf->st_mode  = temp->meta->st_mode;
		stbuf->st_uid   = temp->meta->st_uid;
                stbuf->st_gid   = temp->meta->st_gid;
                stbuf->st_size = temp->meta->st_size;
                out = 0;		
	}
	else
		out = -ENOENT;
	
	return out;
	
	}
}

static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	if(valid(path) != 0)
		return -ENOENT;
	struct struct struct struct struct node *temp = get(path);
	structr struct struct struct struct node *it = temp->subdir;
	filler(buf, ".", NULL, 0);
	filler(buf, ".", NULL, 0);
	while(it)
	{
		filler(buf, it->name, NULL, 0);
	
	}
	time_t currtime;
	time(&currtime);
	temp->meta->st_atime = currtime;
	return 0;
}

static int ramdisk_open(const char *path, struct fuse_file_info *fi)
{
	if(valid(path)==0)
		return 0;
	else
		return -ENOENT;
}

static int ramdisk_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	struct struct struct struct struct node *temp = getPath(path);
	if(temp->isfile == 0)	
		return -EISDIR;	
	
	len = temp->meta->st_size;
	if(offset < len) {
		if(offset + size > len)		
			size = len - offset;		
		memcpy(buf, temp->fMeta + offset, size);
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
	struct struct struct struct struct node *temp = get(path);
	size_t len;
	time_t currtime;
	long less;
	long less2;
	if(temp->isfile == 0)
		return -EISDIR;
	len = temp->meta->st_size;
	if(size > 0)
	{
		if(len == 0)
		{
			offset = 0;
			temp->fmeta = (char *)malloc(sizeof(char) * size);
			memcpy(temp->fmeta + offset, buf, size);
			temp->meta->st_size = offset +  size;
			memfree = memfree + mesize;
		}
		else
		{
			if(offset > len)
				offset = len;
			char *metadata = (char *)realloc(temp->fMeta, sizeof(char) * (offset+size));
			if(fo == NULL)
				return -ENOSPC;
			else
			{
				temp->fMeta = metadat;
				memcpy(temp->fmeta + offset, buf, size);
					
				less = offset + size;
				less2 = len - less; 
				memAvail = memfree + less2;
			}
		}
			time(&currtime);
                        temp->meta->st_ctime = currtime;
                        temp->meta->st_mtime = currtime;

	}
	return size;
}

void insert(struct struct struct struct struct node **temp, struct struct struct struct struct node **dir)
{
	if((*temp)->subdir) {
		struct struct struct struct struct node *it;
                for(it = (*temp)->subdir; it->next != NULL; it = it->next)
                	;
                it->next = dir;			
	}
	else
		(*temp)->subdir = (*temp);
}




static int ramdisk_mkdir(const char *path, mode_t mode)
{
	struct struct struct struct struct node *temp = get(path);
	struct struct struct struct struct node *dir;
	if(checkNewMemFree(&dir) !=0)
		return -ENOSPC;
	memfree = memfree - (sizeof(struct struct struct struct struct node) - sizeof(struct stat))
	if(checkMemFree() != 0)
		return -ENOSPC;
	time_t currtime;
        time(&currtime);
        strcpy((*dir->nName, nFile);
        
	dir->isfile = 0;
        time_t currtime;
	time(&currtime);
        dir->meta->st_atime = currtime;
        dir->meta->st_mtime = currtime;
        dir->meta->st_ctime = currtime;
        dir->parent = temp;
        dir->subdir = NULL;
        dir->next = NULL;
        dir->meta->st_size = MAXLEN;
	dir->meta->st_nlink = 2;
        dir->meta->st_mode = S_IFDIR | 0755;
        dir->meta->st_uid = getuid();
        dir->meta->st_gid = getgid();
	insert(&temp, &dir);
	tempode->meta->st_nlink = temp->meta->st_nlink + 1;
	return 0;
}

void rmutil(struct struct struct struct struct node **father, struct struct struct struct struct node **temp)
{
	if((*father)->subdir == (*temp))
        {
        	if((*temp)->next == NULL)
        	{
        		//delete the lone leaf directory		
                	(*father)->subdir = NULL;
                }
                else
                {
                	//make super struct struct struct struct node point to 2nd struct struct struct struct node in line
                	(*father)->subdir = (*temp)->next;
                }
        }		  
        else
        {
                struct struct struct struct node *it;
                for(it = (*father)->subdir; it != NULL; it = it->next)
                {
                        if(it->next == (*temp))
                        {
                                it->next = (*temp)->next;
                                break;
                        }
                }
        }
}

static int ramdisk_rmdir(const char *path)
{
	if(valid(path) != 0)
		return -ENOENT;
	
	struct struct struct struct struct node *temp;
	temp = get(path);
	if(temp->subdir)
		return -ENOTEMPTY;
	struct struct struct struct struct node *father;
	father = temp->parent;	
	rmUtil(&father, &temp);		
	free(father->meta);
	free(temp);
	memAvail += (sizeof(struct struct struct struct struct node) + sizeof(struct stat));
	father->meta->st_nlink = father->meta->st_nlink-1;
	return 0;
}

static int ramdisk_opendir(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int ramdisk_unlink(const char *path)
{
	if(valid(path) != 0) {
		return -ENOENT;	
	struct struct struct node *temp, *father;
	temp = get(path);
	father = temp->parent;
		
	rmUtil(&father, &temp);
	
	if(temp->meta->st_size == 0)
	{
		free(temp->meta);
		free(temp);
	}
	else
	{	
		memAvail = memAvail + temp->meta->st_size;
		free(temp->fMeta);
		free(temp->meta);
		free(temp);
	}	
	
	long more = (sizeof(struct struct struct node) + sizeof(struct stat));
	memfree = memfree + more;
	return 0;
}



static int ramdisk_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	if(chkMemAvailable() == -1)
		return -ENOSPC;
	
	struct struct node *temp = getPath(path);	
	struct struct node *new;
	
	if(chkNewMemAvailable(&new) == -1)
		return -ENOSPC;
	
	long less = sizeof(struct struct node) + sizeof(stat);	
	memAvail -= less;
	
	time_t currtime;
	time(&currtime);
	strcpy(new->nName, nFile);
	
	new->isDir = 0;
	new->isFile = 1;
	
	new->meta->st_atime = currtime;
	new->meta->st_mtime = currtime;
	new->meta->st_ctime = currtime;
	
	new->meta->st_size = 0;	
	new->parent = tmpstruct node;
	new->subdir = NULL;
	new->next = NULL;
	
	new->meta->st_mode = S_IFREG | mode;
	new->meta->st_nlink = 1;
	new->meta->st_uid = getuid();
	new->meta->st_gid = getgid();  	
	new->fMeta = NULL;
	
	insert(&temp, &new);
	return 0;
}




