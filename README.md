# Sisop-4-2024-MH-IT22
## Anggota Kelompok
- 5027231003  Chelsea Vania Hariyono (mengerjakan nomor 3)
- 5027231024  Furqon Aryadana (mengerjakan nomor 2)
- 5027231057  Elgracito Iryanda Endia (mengerjakan nomor 1)


## Soal 1
Pada soal nomor 1, untuk menggunakan fuse, diperlukan *preprocessor directive* yang mendefinisikan sebuah *macro*. Pada program saya, hal ini dapat dilihat pada *line* pertama pada file
```C++
#define FUSE_USE_VERSION 28
```
Tentunya juga dibutuhkan header untuk menggunakan perintah pada library yang dibutuhkan
```C++
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fuse.h>
#include<errno.h>
#include<fcntl.h>
```
Selain itu saya juga membuat variabel pada luar fungsi yang akan menyimpan path utama yang akan diakses supaya memudahkan ketika saya akan menggunakan path tersebut pada setiap fungsi yang saya buat
```C+++
static char *dir="/home/user/modul_4/soal_1/portofolio";
```
Berikutnya, terdapat beberapa fungsi yang diperlukan untuk menjalankan program sesuai arahan dari soal dan fungsi yang pertama akan saya jelaskan adalah fungsi `reverseBuffer` yang berfungsi untuk membalikkan isi buffer
```C++
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
```
Dalam fungsi tersebut, fungsi akan mengambil isi dari buffer dan panjangnya untuk kemudian buffer tersebut akan di*reverse* dengan loop while secara berulang hingga akhir isi file yang dapat dilihat dengan menghitung panjang file tersebut. Oleh karena itu, panjang isi file juga diperlukan berdampingan dengan isi file tersebut.

Pada *line* berikutnya, terdapat beberapa fungsi yang merupakan bagian dari fungsi FUSE, berikut penjelasan terkait masing-masing fungsi
- fungsi `oper_mkdir` dapat digunakan untuk membuat file pada direktori fuse yang dijalankan
```C++
int oper_mkdir(const char *path, mode_t mode) {
    char fpath[1000];
    strcpy(fpath,dir);
    strcat(fpath,path);
    int res = mkdir(fpath, mode);
    if(res==-1) return -errno;
    return 0;
}
```

- fungsi `oper_readdir` akan membaca konten dari direktori dan mengisi buffer yang disediakan dengan *entry* dari direktori tersebut
```C++
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
```

- fungsi `oper_getattr` akan mengambil informasi terkait file yang terdapat di dalam direktori
```C++
int oper_getattr(const char *path, struct stat *status){
    int res;
    char fpath[100];
    strcpy(fpath,dir);
    strcat(fpath,path);
    res=lstat(fpath,status);
    if(res==-1) return -errno;
    return 0;
}
```

- fungsi `oper_rename` dapat digunakan untuk mengubah nama dan/atau memindah lokasi file pada direktori. Pada fungsi ini, jika direktori tujuan memiliki prefix `/wm`, maka file akan dipindah dan sekaligus diberikan watermark
```C++
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
```

- fungsi `oper_chmod` dapat digunakan untuk mengubah akses pada direktori fuse yang sedang dijalankan
```C++
int oper_chmod(const char *path, mode_t mode){
    char fpath[100];
    strcpy(fpath,dir);
    strcat(fpath,path);
    int result=chmod(fpath,mode);
    if(result==-1) return -errno;
    return 0;
}
```
Berikut merupakan penerapan fungsi chmod
    
![Cuplikan layar 2024-05-25 111316](https://github.com/iryandae/Sisop-4-2024-MH-IT22/assets/121481079/9496cd26-615f-4291-886d-3f856940aad7)

    
- fungsi `oper_read` akan membaca data didalam file dan menyimpannya dalam buffer serta panjang isi file tersebut. Pada fungsi ini, jika target path memiliki prefix `/test`, maka isi file akan dibalikkan menggunakan fungsi `reverseBuffer`
```C++
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
```

Semua fungsi FUSE tersebut akan disimpan dalam sebuah struct seperti berikut
```C++
static struct fuse_operations fuse_oper={
    .readdir=oper_readdir,
    .getattr=oper_getattr,
    .rename=oper_rename,
    .chmod=oper_chmod,
    .read=oper_read,
    .mkdir=oper_mkdir,
};
```

Pada bagian terakhir, terdapat fungsi `main` yang berisi 2 line perintah. Perintah pertama merupakan perintah `umask(0)` yang digunakan untuk memungkinkan izin file diatur secara eksplisit oleh program
```C++
int main(int argc, char *argv[]){
    umask(0);
    return fuse_main(argc,argv,&fuse_oper,NULL);
}
```
Perintah `return fuse_main(argc,argv,&fuse_oper,NULL);` memanggil fungsi `fuse_main` dengan meneruskan argumen baris perintah `argc` dan `argv`, bersama dengan struktur `fuse_oper` yang menentukan operasi sistem file. Fungsi ini memulai sistem file FUSE.


## Soal 3
```c
#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_BUFFER 1024
#define MAX_SPLIT  10000
static const char *root_path = "/home/combrero/museum/relics";

static void build_full_path(char *fpath, const char *path) {
    snprintf(fpath, MAX_BUFFER, "%s%s", root_path, path);
}

static size_t get_total_size(const char *fpath) {
    size_t total_size = 0;
    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int i = 0;

    while (true) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, i++);
        if ((fd = fopen(ppath, "rb")) == NULL) break;

        fseek(fd, 0L, SEEK_END);
        total_size += ftell(fd);
        fclose(fd);
    }
    return i == 1 ? 0 : total_size;
}

static int arch_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = get_total_size(fpath);

    return stbuf->st_size == 0 ? -ENOENT : 0;
}

static int arch_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    DIR *dp = opendir(fpath);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (!strstr(de->d_name, ".000")) continue;

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char filename[MAX_BUFFER];
        strncpy(filename, de->d_name, strlen(de->d_name) - 4);
        filename[strlen(de->d_name) - 4] = '\0';

        if (filler(buf, filename, &st, 0, 0)) break;
    }

    closedir(dp);
    return 0;
}

static int arch_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int i = 0;
    size_t total_read = 0;

    while (size > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, i++);
        if ((fd = fopen(ppath, "rb")) == NULL) break;

        fseek(fd, 0L, SEEK_END);
        size_t part_size = ftell(fd);
        fseek(fd, 0L, SEEK_SET);

        if (offset >= part_size) {
            offset -= part_size;
            fclose(fd);
            continue;
        }

        fseek(fd, offset, SEEK_SET);
        size_t read_size = fread(buf, 1, size, fd);
        fclose(fd);

        buf += read_size;
        size -= read_size;
        total_read += read_size;
        offset = 0;
    }
    return total_read;
}

static int arch_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    FILE *fd;
    int pcurrent = offset / MAX_SPLIT;
    size_t poffset = offset % MAX_SPLIT;
    size_t total_write = 0;

    while (size > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        fd = fopen(ppath, "r+b");
        if (!fd) {
            fd = fopen(ppath, "wb");
            if (!fd) return -errno;
        }

        fseek(fd, poffset, SEEK_SET);
        size_t write_size = size > (MAX_SPLIT - poffset) ? (MAX_SPLIT - poffset) : size;

        fwrite(buf, 1, write_size, fd);
        fclose(fd);

        buf += write_size;
        size -= write_size;
        total_write += write_size;
        poffset = 0;
    }
    return total_write;
}

static int arch_unlink(const char *path) {
    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;

    while (true) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        int res = unlink(ppath);
        if (res == -1) {
            if (errno == ENOENT) break;
            return -errno;
        }
    }
    return 0;
}

static int arch_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    return res == -1 ? -errno : (close(res), 0);
}

static int arch_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;
    off_t size_rmn = size;

    while (size_rmn > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        size_t part_size = size_rmn > MAX_SPLIT ? MAX_SPLIT : size_rmn;
        int res = truncate(ppath, part_size);
        if (res == -1) return -errno;
        size_rmn -= part_size;
    }

    while (true) {
        snprintf(ppath, sizeof(ppath), "%s.%03d", fpath, pcurrent++);
        int res = unlink(ppath);
        if (res == -1) {
            if (errno == ENOENT) break;
            return -errno;
        }
    }
    return 0;
}

static struct fuse_operations arch_oper = {
    .getattr  = arch_getattr,
    .readdir  = arch_readdir,
    .read     = arch_read,
    .write    = arch_write,
    .unlink   = arch_unlink,
    .create   = arch_create,
    .truncate = arch_truncate,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &arch_oper, NULL);
}
```
