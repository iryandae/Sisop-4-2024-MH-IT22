#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fuse.h>
#include<errno.h>
#include<fcntl.h>

#define FUSE_USE_VERSION 28
static char *dir="/home/user/modul_4/soal_1/"

void reverseBuffer(char *buff, int length){
    int start=0;
    int end=lenght-1;
    while(start<end){
        char temp=buffer[start];
        buffer[start]=buffer[end];
        buffer[end]=temp;
        start++;
        end--;
    }
}

void reverseContent(const char *path){
    int fd=open(path,O_RDWR);
    if(fd==-1) return;

    struct stat st;
    if(fstat(fd,&st)==-1){
        close(fd);
        return;
    }

    size_t size=st.st_size;
    char *content=malloc(size);
    if(content==NULL){
        close(fd);
        return;
    }
    for(size_t i=0;i<size/2;i++){
        char temp=content[i];
        content[i]content[size-i-1];
        content[size-i-1]=temp;
    }
    
    lseek(fd,0,SEEK_SET);
    ssize_t bytesWritten=write(fd,content,size);
    if(byteWritten==-1){
        free(content);
        close(fd);
        return;
    }
    free(content);
    close(fd);
}

int oper_mkdir(const char *path, mode_t mode){
    char filepath[100];
    strcpy(filepath,dir);
    strcat(filepath,path);
    int result=mkdir(file,mode);
    if(result==-1) return -errno;
    return 0;
}

int oper_rmdir(const char *path){
    char filepath[100];
    strcpy(filepath,dir);
    strcat(filepath,path);
    int result=rmdir(filepath);
    if(result==-1) return -errno;
    return 0;
}

int oper_readdir(const char *path, void *buff, fuse_fill_dir_t fl, off_t offset, struct fuse_file_info *fileInfo){
    char filepath[100];
    if(strcmp(path,"/")==0){
        path=dir;
        strcpy(fpath, path);
    }
    else{
        strcpy(filepath,dir);
        strcat(filepath,path);
    }
    int result=0;
    DIR *dr=opendir(filepath);
    if(dr==NULL) return -errno;
    struct dirent *de;
    while((de=readdir(dr))!=NULL){
        struct stat status;
        memset(&status,0,sizeof(status));
        status.status_ino=de->d_ino;
        staus.status_mode=de->d_type<<12;
        result=(fl(buff,de->d_name,&status,0));
        if(result!=0) break;
    }
    closedir(dr);
    return 0;
}

int oper_getattr(const char *path, struct stat *status){
    char filepath[100];
    int result;
    strcpy(filepath,dir);
    strcat(filepath,path);
    result=lstat(filepath,status);
    if(res==-1) return -errno;
    return 0;
}

int oper_rename(const char *source, const char *destination){
    char sourcePath[100],destinationPath[100];
    strcpy(sourcePath,dir);
    strcat(sourcePath,source);

    strcpy(destinationPath,dir);
    strcat(destinationPath,destination);

    if(strstr(destinationPath,"/wm")!=NULL){
        int src=open(sourcePath,O_RDONLY);
        if(src==-1){
            perror("unable to access source");
            return -errno;
        }
        int dest=open(destinationPath,O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(dest==-1){
            perror("unable to access destination");
            close(src);
            return -errno;
        }
        
        char cmd[100];
        sprintf(cmd, "convert -gravity south -geometry +0+20 /proc/%d/fd/%d -fill white -pointsize 36 -annotate +0+0 'inikaryakita.id' /proc/%d/fd/%d", getpid(), src, getpid(), dest);
        
        int result=system(cmd);
        close(src);
        close(dest);

        if(result==-1){
            perror("ImageMagick execution failed");
            return -errno;
        } 

        if(unlink(sourcePath)==-1){
            perror("failed removing source file");
            return -errno;
        }
        
        result=rename(sourcePath,destinationPath);
        if(result==-1){
            perror("failed moving file");
            return -errno;
        }
        return 0;
    }
}

int oper_chmod(const char *path, mode_t mode){
    char filepath[100];
    strcpy(filepath,dir);
    strcat(filepath,path);
    if(strstr(filepath,"/script.sh")!=NULL) return -EACCES;
    int result=chmod(filepath,mode);
    if(res==-1) return -errno;
    return 0;
}

int oper_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *fileInfo){
    int fd,result;
    char filepath[100];

    strcpy(filepath,dir);
    strcat(filepath,path);
    fd=open(filepath,O_RDONLY);
    if(fd==-1) return -errno;

    result=pread(fd,buff,size,offset);
    if(result==-1){
        close(fd);
        return -errno;
    }
    if(strstr(filepath,"/test")!=NULL){
        reverseBuffer(buff,result);
    }
    close(fd);
    return result;
}

int oper_write(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *fileInfo){
    int fd,result;
    char filepath[100];

    strcpy(filepath,dir);
    strcat(filepath,path);
    fd=open(filepath,O_RDONLY);
    if(fd==-1) return -errno;

    if(strstr(filepath,"/test")!=NULL){
        char *rev_buf=malloc(size);
        if(!rev_buf){
            close(fd);
            return -ENOMEM;
        }
        memcpy(rev_buf,buff,size);
        reverseBuffer(rev_buf,size);
        result=pwrite(fd,rev_buf,size,offset);
        free(rev_buf);
    }
    else{
        result=pwrite(fd,buff,size,offset);
    }

    if(result==-1){
        close(fd);
        return -errno;
    }
    close(fd);
    return result;
}

int oper_create(const char *path, mode_t mode, struct fuse_file_info *fileInfo){
    char filepath[100];
    strcpy(filepath,dir);
    strcat(filepath,path);
    int fd=creat(filepath,mode);
    if(fd==-1) return -errno;
    fileInfo->fh=fd;

    if(strstr(filepath,"/test")!=NULL){
        close(fd);
        int result=reverseContent(filepath);
        if(result!=0) return result;
        fd=open(filepath,fileInfo->flags);
        if(fd==-1) return -errno;
        fileInfo->fh=fd;
    }
    return 0;
}

int oper_remove(const char *path){
    char filepath[100];
    strcpy(filepath,dir);
    strcat(filepath,path);
    int result=unlink(filepath);
    if(result==-1) return -errno;
    return 0;
}

static struct fuse_operations fuse_oper={
    .mkdir=oper_mkdir,
    .rmdir=oper_rmdir,
    .readdir=oper_readdir,
    .getattr=oper_getattr,
    .rename=oper_rename,
    .chmod=oper_chmod,
    .read=oper_read,
    .write=oper_write,
    .create=oper_create,
    .remove=oper_remove,
};

int main(int argc, char *argv[]){
    return fuse_main(argc,argv,&fuse_oper,NULL);
}
