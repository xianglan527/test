/*
 * guzhoudiaoke@126.com
 * 2018-01-12
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"
#include "file.h"

file_system_t fs;
process_t* current;

int sys_open(char* path, uint32 mode)
{
    return fs.do_open(path, mode);
}

int sys_close(int fd)
{
    return fs.do_close(fd);
}

int sys_read(int fd, void* buffer, unsigned count)
{
    return fs.do_read(fd, buffer, count);
}

int sys_write(int fd, void* buffer, unsigned count)
{
    return fs.do_write(fd, buffer, count);
}

int sys_mkdir(char* path)
{
    return fs.do_mkdir(path);
}

int sys_link(char* path_old, char* path_new)
{
    return fs.do_link(path_old, path_new);
}

int sys_unlink(char* path)
{
    return fs.do_unlink(path);
}

void halt()
{
    while (1) {
    }
}

void get_name(char* path, char* name)
{
    char* p = path + strlen(path);
    char* end;
    while (*p == '/') {
        p--;
    }
    end = p;
    while (*p != '/') {
        p--;
    }

    strncpy(name, p+1, end-p);
}

void check(char* path, char* dpath)
{
    int fd = sys_open(dpath, file_t::MODE_RDWR);
    if (fd < 0) {
        printf("failed to open %s\n", dpath);
        halt();
    }

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        printf("failed to open %s\n", path);
        halt();
    }

    char buffer[512], buffer1[512];
    int nbyte;
    do {
        nbyte = sys_read(fd, buffer, 512);
        fread(buffer1, 1, nbyte, fp);
        if (memcmp(buffer, buffer1, nbyte) != 0) {
            printf("file error %s\n", path);
            halt();
        }
    } while (nbyte > 0);

    fclose(fp);
    sys_close(fd);
}

void ls(char* path)
{
    printf("\n%s:\n", path);
    inode_t* inode = fs.namei(path);
    if (inode == NULL) {
        printf("failed to ls %s\n", path);
        return;
    }

    if (inode->m_type == inode_t::I_TYPE_FILE) {
        printf("%s %d %d %d %d\n", path, inode->m_type, inode->m_inum, inode->m_size);
    }
    else if (inode->m_type == inode_t::I_TYPE_DIR) {
        dir_entry_t dir;
        unsigned int offset = 0;
        while (fs.read_inode(inode, (char *) &dir, offset, sizeof(dir_entry_t)) == sizeof(dir_entry_t)) {
            if (dir.m_inum != 0) {
                printf("%s\t %u\n", dir.m_name, dir.m_inum);
            }
            offset += sizeof(dir_entry_t);
            memset(dir.m_name, 0, MAX_PATH);
        }
    }

    fs.put_inode(inode);
}

void write_file(char* path)
{
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return;
    }

    char name[128] = {0};
    get_name(path, name);

    char disk_path[128] = {0};
    sprintf(disk_path, "/bin/%s", name);
    sys_unlink(disk_path);

    int fd = sys_open(disk_path, file_t::MODE_CREATE | file_t::MODE_RDWR);

    char buffer[512];
    int nbyte;
    int total_w = 0, total_r = 0;
    do {
        nbyte = fread(buffer, 1, 512, fp);
        total_r += nbyte;
        int nwrite = sys_write(fd, buffer, nbyte);
        total_w += nwrite;
    } while (nbyte > 0);

    printf("%s, total read: %u, total write %u\n", disk_path, total_r, total_w);
    sys_close(fd);
    fclose(fp);

    check(path, disk_path);
}

int main(int argc, char **argv)
{
    process_t proc;
    proc.init();
    current = &proc;

    fs.init();

    sys_mkdir("/bin");

    for (int i = 1; i < argc; i++) {
        write_file(argv[i]);
    }

    ls("/");
    ls("/bin");

    return 0;
}

