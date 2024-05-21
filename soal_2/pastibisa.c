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

// Decoding tables for Base64
static char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table() {
    decoding_table = malloc(256);
    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

unsigned char *base64_decode(const char *data, size_t input_length, size_t *output_length) {
    if (decoding_table == NULL) build_decoding_table();
    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = data[i] == '=' ? 0 : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

char rot13_char(char c) {
    if ('a' <= c && c <= 'z') {
        return ((c - 'a' + 13) % 26) + 'a';
    } else if ('A' <= c && c <= 'Z') {
        return ((c - 'A' + 13) % 26) + 'A';
    } else {
        return c;
    }
}

void rot13(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = rot13_char(str[i]);
    }
}

void decode_hex(const char* hex, char* output) {
    while (*hex) {
        char tmp[3] = { hex[0], hex[1], '\0' };
        *output++ = (char)strtol(tmp, NULL, 16);
        hex += 2;
    }
    *output = '\0';
}

void decode_rev(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

void log_result(const char *tag, const char *info, int success) {
    FILE *log_file = fopen("/home/satsujinki/sensitif/logs-fuse.log", "a");
    if (log_file == NULL) {
        return;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log_file, "[%s]::%02d/%02d/%04d-%02d:%02d:%02d::[%s]::[%s]\n",
            success ? "SUCCESS" : "FAILED",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            tag, info);
    fclose(log_file);
}

static const char *root_path = "/home/satsujinki/sensitif";

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    // Check if the file is in a "rahasia" directory
    if (strstr(full_path, "rahasia") != NULL) {
        char input_password[256];

        // Prompt for a password
        printf("Enter password to access this folder: ");
        scanf("%255s", input_password);

        // Check if the entered password matches the correct password
        if (strcmp(input_password, "bismillah") != 0) {
            printf("Incorrect password!\n");
            return -EACCES;
        }
    }

    int res = open(full_path, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    fd = open(full_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
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

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int res = lstat(full_path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
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

static int xmp_mkdir(const char *path, mode_t mode)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int res = mkdir(full_path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_rmdir(const char *path)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int res = rmdir(full_path);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int fd = creat(full_path, mode);
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

static int xmp_unlink(const char *path)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int res = unlink(full_path);
    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    (void) fi;

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", root_path, path);

    int fd;
    int res;

    fd = open(full_path, O_WRONLY);
    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .create = xmp_create,
    .unlink = xmp_unlink,
    .write = xmp_write,
};

int main(int argc, char *argv[])
{
    return fuse_main_real(argc, argv, &xmp_oper, sizeof(xmp_oper), NULL);
}

