# Sisop-4-2024-MH-IT22
## Anggota Kelompok
- 5027231003  Chelsea Vania Hariyono (mengerjakan nomor 3)
- 5027231024  Furqon Aryadana (mengerjakan nomor 2)
- 5027231057  Elgracito Iryanda Endia (mengerjakan nomor 1)


## Soal 1
Pada soal nomor 1, untuk menggunakan fuse, diperlukan *preprocessor directive* yang mendefinisikan sebuah *macro*. Pada program saya, hal ini dapat dilihat pada *line* pertama pada file
```C
#define FUSE_USE_VERSION 28
```
Tentunya juga dibutuhkan header untuk menggunakan perintah pada library yang dibutuhkan
```C
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
```C
static char *dir="/home/user/modul_4/soal_1/portofolio";
```
Berikutnya, terdapat beberapa fungsi yang diperlukan untuk menjalankan program sesuai arahan dari soal dan fungsi yang pertama akan saya jelaskan adalah fungsi `reverseBuffer` yang berfungsi untuk membalikkan isi buffer
```C
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
```C
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
```C
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
```C
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
```C
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
```C
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
```C
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
```C
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
```C
int main(int argc, char *argv[]){
    umask(0);
    return fuse_main(argc,argv,&fuse_oper,NULL);
}
```
Perintah `return fuse_main(argc,argv,&fuse_oper,NULL);` memanggil fungsi `fuse_main` dengan meneruskan argumen baris perintah `argc` dan `argv`, bersama dengan struktur `fuse_oper` yang menentukan operasi sistem file. Fungsi ini memulai sistem file FUSE.


## Soal 2
Unduh file sensitif.zip dari link yang didapat dari email
``wget --no-check-certificate 'https://drive.google.com/uc?export=download&id=1t1CXcJgesUYj2i7KrHKdwXd79neKWF1u' -O sensitif.zip``

```C
#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
```
Deklarasikan library yang akan digunakan,definisi fuse yang digunakan.

```C
static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char *decoding_table = NULL;
static const int mod_table[] = {0, 2, 1};

void build_decoding_table() {
    decoding_table = malloc(256);
    if (decoding_table == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}
```
Fungsi untuk membangun tabel decoding untuk operasi Base64.

```C
void free_decoding_table() {
    free(decoding_table);
    decoding_table = NULL;
}
```
Fungsi untuk membebaskan memori yang dialokasikan untuk tabel decoding agar memory tidak penuh.

```C
unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length) {
    if (decoding_table == NULL) build_decoding_table();
    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = data[i] == '=' ? 0 : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 : decoding_table[(unsigned char)data[i++]];

        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}
```
Fungsi untuk mendecode data dalam format base64.

```C
char rot13_char(char c) {
    if ('a' <= c && c <= 'z') {
        return ((c - 'a' + 13) % 26) + 'a';
    } else if ('A' <= c && c <= 'Z') {
        return ((c - 'A' + 13) % 26) + 'A';
    } else {
        return c;
    }
}
```
Fungsi untuk melakukan enkripsi dan dekripsi ROT13 untuk satu karakter.

```C
void rot13(char* str) {
    for (size_t i = 0; str[i]; i++) {
        str[i] = rot13_char(str[i]);
    }
}
```
Fungsi untuk melakukan enkripsi dan dekripsi ROT13 untuk seluruh string.

```C
void decode_hex(const char* hex, char* output) {
    while (*hex) {
        char tmp[3] = { hex[0], hex[1], '\0' };
        *output++ = (char)strtol(tmp, NULL, 16);
        hex += 2;
    }
    *output = '\0';
}
```
Fungsi untuk decode data dari format heksadesimal ke format biner.

```C
void decode_rev(char* str) {
    size_t len = strlen(str);
    for (size_t i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}
```
Fungsi untuk memutar balikkan setiap string dalam file.

```C
void log_result(const char *tag, const char *info, int success) {
    FILE *log_file = fopen("/home/satsujinki/sensitif/logs-fuse.log", "a");
    if (log_file == NULL) {
        return;
    }

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    fprintf(log_file, "[%s]::%02d/%02d/%04d-%02d:%02d:%02d::[%s]::[%s]\n",
            success ? "SUCCESS" : "FAILED",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            tag, info);
    fclose(log_file);
}
```
Fungsi untuk mencatat hasil operasi dalam sebuah file log bernama logs-fuse.log dengan format yang ditentukan.

```C
static const char *root_path = "/home/satsujinki/sensitif";
```
sebagai dasar pemanggilan fungsi-fungsi fuse untuk mengonstruksi path lengkap ke file atau direktori yang diminta oleh sistem operasi.

```C
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    if (strstr(full_path, "rahasia") != NULL) {
        char *input_password = getpass("Enter password to access this folder: ");
        if (!input_password) {
            printf("Error reading password!\n");
            return -EACCES;
        }

        if (strcmp(input_password, "bismillah") != 0) {
            printf("Incorrect password!\n");
            return -EACCES;
        }
    }

    int res = open(full_path, fi->flags);
    if (res == -1)
        return -errno;

    fi->fh = res; // Store file descriptor for later use
    return 0;
}
```
Fungsi untuk membuka file sekaligus dengan ``strstr`` membaca nama file apabila ada file yang mengandung nama ``rahasia`` akan meminta password untuk mengakses file.

```C
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd = fi->fh;
    int res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else {
        if (strstr(path, "rot13,") != NULL) {
            rot13(buf);
            log_result("readFile", "File read and decoded with ROT13", 1);
        } else if (strstr(path, "hex,") != NULL) {
            char *decoded_buf = malloc(size);
            if (!decoded_buf) {
                log_result("readFile", "Memory allocation failed", 0);
                close(fd);
                return -ENOMEM;
            }
            decode_hex(buf, decoded_buf);
            memcpy(buf, decoded_buf, size);
            free(decoded_buf);
            log_result("readFile", "File read and decoded with Hex", 1);
        } else if (strstr(path, "base64,") != NULL) {
            size_t output_length;
            unsigned char *decoded_data = base64_decode(buf, res, &output_length);
            if (!decoded_data) {
                log_result("readFile", "Base64 decode failed", 0);
                close(fd);
                return -EINVAL;
            }
            memcpy(buf, decoded_data, output_length);
            free(decoded_data);
            log_result("readFile", "File read and decoded with Base64", 1);
            res = output_length;
        } else if (strstr(path, "rev,") != NULL) {
            decode_rev(buf);
            log_result("readFile", "File read and decoded with Rev", 1);
        } else {
            log_result("readFile", "File read normally", 1);
        }
    }

    close(fd);
    return res;
}
```
Fungsi untuk membaca isi file dan sekaligus dengan ``strstr`` membaca nama file apabila ada file yang mengandung nama ``rot13`` akan didecode dengan logika rot13,apabila ada file yang mengandung nama ``hex`` akan didecode dengan logika hex, apabila ada file yang mengandung nama ``base64`` akan didecode dengan logika base64, apabila ada file yang mengandung nama ``rev`` akan didecode dengan logika rev.

```C
static int xmp_getattr(const char *path, struct stat *stbuf) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int res = lstat(full_path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}
```
Fungsi ini digunakan untuk mendapatkan atribut (metadata) dari sebuah file atau direktori. Ketika sistem operasi membutuhkan informasi tentang file atau direktori tertentu, fungsi ini akan dipanggil.

```C
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    dp = opendir(full_path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        if (filler(buf, de->d_name, NULL, 0))
            break;
    }

    closedir(dp);
    return 0;
}
```
Fungsi xmp_readdir digunakan untuk membaca isi dari sebuah direktori. Ketika sistem operasi memerlukan daftar konten dari sebuah direktori, fungsi ini akan dipanggil.

```C
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};
```
Struktur ini menyimpan pointer ke fungsi-fungsi yang akan dipanggil oleh FUSE ketika terjadi operasi tertentu pada filesystem yang di-mount.

```C
int main(int argc, char *argv[]) {
    int ret = fuse_main(argc, argv, &xmp_oper, NULL);
    free_decoding_table();
    return ret;
}
```
Mulai eksekusi program filesystem FUSE, menggunakan fungsi operasi yang telah ditentukan, dan kemudian membebaskan sumber daya sebelum program berakhir.



revisi:
pada kode sebelumnya saya membuat 310 line code namun saat dijalankan terminal saya tidak bisa mengeluarkan output dan untuk membuka file manager hanya loading sampai vm ditutup secara paksa, saya mencoba untuk menghilangkan beberapa fungsi yang tidak digunakan agar tidak terlalu membebani system, sudah saya coba menjadi 230 line namun saya belum berhasil memecahkan masalah ini, mungkin terlalu berat bagi VM saya yang saya set ke 4GB ram.

<img width="1276" alt="Screenshot 2024-05-23 at 23 44 43" src="https://github.com/iryandae/Sisop-4-2024-MH-IT22/assets/150358232/cc71af50-b55c-4b07-97bc-6919a3aa24e0">

## Soal 3

Setelah menginstal fuse, buat folder ```museum``` untuk mempermudah pengerjaan 
```shell
mkdir museum && cd museum
```
Selanjutnya buat folder ```[nama_bebas]``` (saya menggunakan `ree`) dan ```report```
```shell
mkdir ree && mkdir report
```
Dowload file dengan link yang dibagikan, lalu extract, sehingga terbentuk folder ```relics```

MAX_BUFFER: Ukuran maksimum buffer untuk path file.
MAX_SPLIT: Ukuran maksimum setiap bagian file.
root_path: Direktori basis untuk filesystem.
```c
#define MAX_BUFFER 1024
#define MAX_SPLIT  10000

static const char *root_path = "/home/combrero/museum/relics";
```
Membangun path lengkap file dengan menggabungkan path yang diberikan ke direktori root.
```c
static void build_full_path(char *fpath, const char *path) {
    snprintf(fpath, MAX_BUFFER, "%s%s", root_path, path);
}
```
Menghitung ukuran total dari sebuah file yang terbagi dengan menjumlahkan ukuran dari setiap bagian.
```c
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
```
Operasi FUSE
Mendapatkan atribut file. Direktori dan file reguler memiliki atribut yang berbeda.
```c
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
```

Membaca direktori dan mengembalikan *entries* di dalamnya.
```c
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
```

Membaca data dari file.
```c
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
```

Menulis data ke sebuah file, membuat bagian baru jika diperlukan.
```c
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
```

Menghapus file dan semua bagiannya.
```c
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
```

Membuat file baru.
```c
static int arch_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    snprintf(fpath, sizeof(fpath), "%s%s.000", root_path, path);

    int res = creat(fpath, mode);
    return res == -1 ? -errno : (close(res), 0);
}
```

Memotong file menjadi ukuran tertentu, menyesuaikan atau menghapus bagian-bagiannya sesuai yang dibutuhkan.
```c
static int arch_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    (void) fi;

    char fpath[MAX_BUFFER];
    build_full_path(fpath, path);

    char ppath[MAX_BUFFER + 4];
    int pcurrent = 0;
    off_t size_rmn = size;

    while (size_rmn > 0) {
        snprintf(ppath, sizeof(ppath), "%s.%03d

```c
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
```

Struktur menyediakan "menu" dari operasi-operasi FUSE yang diperlukan untuk filesystem yang sudah dibuat sebelumnya.
```c
static struct fuse_operations arch_oper = {
    .getattr  = arch_getattr,
    .readdir  = arch_readdir,
    .read     = arch_read,
    .write    = arch_write,
    .unlink   = arch_unlink,
    .create   = arch_create,
    .truncate = arch_truncate,
};
```
Fungsi Main
Merupakan entry point dari program, menginisialisasi FUSE dan memasang filesystem.
```c
int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &arch_oper, NULL);
}
```
