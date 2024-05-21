#define FUSE_USE_VERSION 28
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fuse.h>
#include<errno.h>
#include<fcntl.h>

static char *dir="/home/user/modul_4/soal_1/portofolio";

void reverseBuffer(char *buff, int length){
    int start=0;
    int end=length-1;
    while(start<end){
        char temp=buff[start];
        buff[start]=buff[end];
        buff[end]=temp;
        start++;
        end--;
    }
}

int oper_mkdir(const char *path, mode_t mode) {
    char fpath[1000];
    strcpy(fpath,dir);
    strcat(fpath,path);
    int res = mkdir(fpath, mode);
    if(res==-1) return -errno;
    return 0;
}

int oper_readdir(const char *path, void *buff, fuse_fill_dir_t fl, off_t offset, struct fuse_file_info *fileInfo){
    char fpath[100];
    strcpy(fpath,dir);
    strcat(fpath,path);
    
    DIR *dr;
    struct dirent *de;

    dr=opendir(fpath);
    if(dr==NULL) return -errno;

    while((de=readdir(dr))!=NULL){
        struct stat st;
        memset(&st,0,sizeof(st));
        st.st_ino=de->d_ino;
        st.st_mode=de->d_type<<12;
        if(fl(buff,de->d_name,&st,0)!=0) break;
    }
    closedir(dr);
    return 0;
}

int oper_getattr(const char *path, struct stat *status){
    int res;
    char fpath[100];
    strcpy(fpath,dir);
    strcat(fpath,path);
    res=lstat(fpath,status);
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
        int srcfd=open(sourcePath, O_RDONLY);
        int destfd=open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        char command[1000],watermark_text[]="inikaryakita.id";
        sprintf(command, "convert -gravity south -geometry +0+20 /proc/%d/fd/%d -fill white -pointsize 36 -annotate +0+0 '%s' /proc/%d/fd/%d", getpid(), srcfd, watermark_text, getpid(), destfd);
        
        system(command);
        unlink(sourcePath);

        close(srcfd);
        close(destfd);
    }
    else {
        rename(sourcePath,destinationPath);
    } 
    return 0;
}

int oper_chmod(const char *path, mode_t mode){
    char fpath[100];
    strcpy(fpath,dir);
    strcat(fpath,path);
    int result=chmod(fpath,mode);
    if(result==-1) return -errno;
    return 0;
}

int oper_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *fileInfo){
    char fpath[100];
    int fd,res;
    strcpy(fpath,dir);
    strcat(fpath,path);

    fd=open(fpath,O_RDONLY);
    res=pread(fd,buff,size,offset);
    if(strstr(fpath,"/test")!=NULL){
        reverseBuffer(buff,res);
    }
    close(fd);
    return res;
}
static struct fuse_operations fuse_oper={
    .readdir=oper_readdir,
    .getattr=oper_getattr,
    .rename=oper_rename,
    .chmod=oper_chmod,
    .read=oper_read,
    .mkdir=oper_mkdir,
};

int main(int argc, char *argv[]){
    umask(0);
    return fuse_main(argc,argv,&fuse_oper,NULL);
}
