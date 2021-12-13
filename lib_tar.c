#include "lib_tar.h"

// TODO : clean code
int check_version_and_magic(char *address_tar) {
//    printf("MAGIC:   %s\n", address_tar+257);
//    printf("VERSION: %s\n", address_tar+263);
    if (!(strcmp(address_tar+257, TMAGIC) == 0 || address_tar+257 == NULL)) return -1;
    if (!(strcmp(address_tar+263, TVERSION) == 0 || address_tar+263 != NULL)) return -2;
    return 0;
}

int check_chksum(char *address_tar) {
    u_int chksum_val;
    u_int chksum_calc = 0;

    for (int i = 0; i < 512; ++i) {
        if (i == 148) { // chksum all bytes = char [SPACE] (32 == 0x20) == BLANK
            chksum_val = TAR_INT((address_tar+i));
            chksum_calc += 32*8;
            i += 7;
        } else {
            chksum_calc += address_tar[i];
        }
    }

//    printf("chksum_val  : %d\n", chksum_val);
//    printf("chksum_calc : %d\n", chksum_calc);

    if (chksum_val != chksum_calc) return -3;
    return 0;
}

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file

    int res;
    size_t header_size = sizeof(tar_header_t);
    char *address_tar = mmap(NULL, header_size, PROT_READ, MAP_SHARED, tar_fd, 0);

    res = check_version_and_magic(address_tar);
    if (res == 0) res = check_chksum(address_tar);

    munmap(address_tar, header_size);
    return res;
}

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));

    while (read(tar_fd, tar_header, sizeof(tar_header_t)) != 0) {
        if (strcmp(tar_header->name, path) == 0) return 1;
        lseek(tar_fd, TAR_INT(tar_header->size + sizeof(tar_header_t) + sizeof(tar_header->padding)), SEEK_CUR);
    }

    free(tar_header);
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file
    return 0;
}


/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file
    return 0;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the file
    return 0;
}