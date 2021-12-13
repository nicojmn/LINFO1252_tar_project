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
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }

    int ret = check_archive(fd);
    printf("check_archive returned %d\n", ret);


    // Tests with soumission.tar

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

    // is_dir() test

    int dir_make = is_dir(fd, "Makefile");
    printf("is_dir (Makefile) returned %d, and should return 0\n", dir_make);

    // add directory CMake-build-debug for this test

    int dir_cmake = is_dir(fd, "cmake-build-debug/");
    printf("is_dir (cmake-build-debug) returned %d, and should return 1\n", dir_cmake);

    return 0;
}