#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

static const char *source_path = "/path/to/relics";

// Helper function to construct full path
static void construct_fullpath(char *fullpath, const char *path) {
    snprintf(fullpath, PATH_MAX, "%s%s", source_path, path);
}

static int relicfs_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fullpath[PATH_MAX];
    construct_fullpath(fullpath, path);
    
    res = lstat(fullpath, stbuf);
    if (res == -1)
        return -errno;
    
    return 0;
}

static int relicfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    char fullpath[PATH_MAX];
    construct_fullpath(fullpath, path);

    dp = opendir(fullpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int relicfs_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char fullpath[PATH_MAX];
    construct_fullpath(fullpath, path);

    res = open(fullpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

static int relicfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char fullpath[PATH_MAX];
    construct_fullpath(fullpath, path);

    fd = open(fullpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static struct fuse_operations relicfs_oper = {
    .getattr    = relicfs_getattr,
    .readdir    = relicfs_readdir,
    .open       = relicfs_open,
    .read       = relicfs_read,
};


};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &relicfs_oper, NULL);
}
