#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
//    int fd = open("/home/guillaume/Projects/CLionProjects/[LINFO1252] Systemes Informatiques/[PROJECT2] TAR/complex.tar" , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }

    int ret = check_archive(fd);
    printf("check_archive returned %d, and should return [0..inf]\n", ret);


    // Tests with symlink.tar

    // exists() tests
    printf("\n\n====================\n|| exists() tests ||\n====================\n\n");
    int exists_make = exists(fd, "Makefile");
    printf("exists (Makefile) returned %d, and should return 1\n", exists_make);
    int exists_tests_c = exists(fd, "tests.c");
    printf("exists (tests.c) returned %d, and should return 1\n", exists_tests_c);

    int exists_lib_c = exists(fd, "lib_tar.c");
    printf("exists (lib_tar.c) returned %d, and should return 1\n", exists_lib_c);

    int exists_lib_h = exists(fd, "lib_tar.h");
    printf("exists (lib_tar.h) returned %d, and should return 1\n", exists_lib_h);

    int file_not_in_tar = exists(fd, "dghjkulim.o");
    printf("exists (not in tar) returned %d, and should return 0\n", file_not_in_tar);

    // is_dir() tests
    printf("\n\n====================\n|| is_dir() tests ||\n====================\n\n");
    int dir_make = is_dir(fd, "Makefile");
    printf("is_dir (Makefile) returned %d, and should return 0\n", dir_make);

    // add directory 'dir-for-test' for this test

    int dir_cmake = is_dir(fd, "cmake-build-debug/");
    printf("is_dir (dir-for-test) returned %d, and should return 1\n", dir_cmake);

    int dir = is_dir(fd, "dir-for-test/");
    printf("is_dir (dir-for-test) returned %d, and should return 1\n", dir);

    size_t *no_entries = malloc(sizeof(size_t));
    size_t size = 5;
    *no_entries = size;
    char **entries = malloc(sizeof(char **) * *no_entries);
    for (int i = 0; i < *no_entries; ++i) { entries[i] = malloc(sizeof(char) * 100); }
    int list_lib_h = list(fd, "complex/sym_sym_dir1", entries, no_entries);
    printf("list returned %d\n", list_lib_h);
    printf("no_entries = %zu\n", *no_entries);
    printf("entries = \n");
    for (int i = 0; i < *no_entries; ++i) printf("\t%s\n", entries[i]);
    for (int i = 0; i < size; ++i) { free(entries[i]); }
    free(entries);
    free(no_entries);

    // read_file() tests
    printf("\n\n=======================\n|| read_file() tests ||\n=======================\n\n");

    struct stat *tar_stat = (struct stat *) malloc(sizeof(struct stat));
    if (tar_stat == NULL) return -3;
    if (fstat(fd, tar_stat) == -1) return -3;

    size_t tar_size = tar_stat->st_size;
    size_t overflow_offset = tar_size * 2;
    size_t good_offset = tar_size / 2;
    uint8_t *buffer = (uint8_t *) malloc(sizeof(uint8_t));
    size_t *len = (size_t *) malloc(sizeof(size_t));


    //printf("Is symlink : %i\n", is_symlink(fd, "symlink-makefile"));
    //printf("Symlink real path : %s\n", get_real_path(fd, "symlink-makefile"));

    ssize_t read_no_file = read_file(fd, "efgrhtyjku.zzdfegrh", good_offset, buffer, len);
    ssize_t read_symlink_file = read_file(fd, "symlink-makefile", good_offset, buffer, len);
    ssize_t read_symlink_dir = read_file(fd, "symlink-project", good_offset, buffer, len);
    ssize_t read_reg_file = read_file(fd, "lib_tar.h", good_offset, buffer, len);

    printf("read_file() with no file in tar should return -1 and it return %zi\n", read_no_file);
    printf("read_file() with symlink file in tar should return 0 and it return %zi\n", read_symlink_file);
    printf("read_file() with symlink dir in tar should return -1 and it return %zi\n", read_symlink_dir);
    printf("read_file() with regular file in tar should return 0 and it return %zi\n", read_reg_file);

    ssize_t read_makefile = read_file(fd, "Makefile", overflow_offset, buffer, len);
    ssize_t read_makefile_2 = read_file(fd, "Makefile", -overflow_offset, buffer, len);

    printf("read_file() with offset > tar size should return -2 and it returned %zi\n", read_makefile);
    printf("read_file() with offset < tar size should return -2 and it returned %zi\n", read_makefile_2);

    free(buffer);
    free(len);

    return 0;
}