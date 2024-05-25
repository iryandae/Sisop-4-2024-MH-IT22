# Sisop-4-2024-MH-IT22
## Anggota Kelompok
- 5027231003  Chelsea Vania Hariyono
- 5027231024  Furqon Aryadana
- 5027231057  Elgracito Iryanda Endia


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
