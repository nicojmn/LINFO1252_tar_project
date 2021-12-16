#include "lib_tar.h"
#include <stdbool.h>

//void debug_dump(const uint8_t *bytes, size_t len) {
//    for (int i = 0; i < len;) {
//        printf("%04x:  ", (int) i);
//
//        for (int j = 0; j < 16 && i + j < len; j++) {
//            printf("%02x ", bytes[i + j]);
//        }
//        printf("\t");
//        for (int j = 0; j < 16 && i < len; j++, i++) {
//            printf("%c ", bytes[i]);
//        }
//        printf("\n");
//    }
//}

/**
 * Check if the current header has an empty name.
 *
 * @param tar_header The header of the current file.
 * @return 0 if the current header has an empty name,
 *         1 if not.
 */
int is_tar_eof(tar_header_t* tar_header) {
    int i = 0; int len = 0;
    while (i < 100 && len == 0) {len += tar_header->name[i]; i++;}
    return (len == 0) ? true : false;
}

/**
 * Find the next offset header.
 *
 * @param tar_header The header of the current file.
 * @return a zero or positive value representing the offset of the next file in the archive
 */
off_t next_offset_header(const tar_header_t *const tar_header) {
    size_t size_file = TAR_INT(tar_header->size);
    size_t size_header = sizeof(tar_header_t);
    u_long average_size = (size_file + size_header-1)/size_header;
    return (off_t) (size_header * average_size);
}

/**
 * Find the offset header of the file/directory/symlink in path (FROM START).
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 * @param path A path to an entry in the archive.
 *
 * @return a zero or positive value if the file exists, representing the offset of the file,
 *         -1 if the file doesn't exist
 */
off_t offset_header(int tar_fd, char *path) {
    lseek(tar_fd, 0, SEEK_SET);
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));
    for (off_t offset = 0; read(tar_fd, tar_header, sizeof(tar_header_t)) > 0; offset++) {
        if (strcmp(tar_header->name, path) == 0) {free(tar_header); return (off_t) sizeof(tar_header_t) * offset;}
    }
    free(tar_header);
    return -1;
}

int check_magic_and_version(tar_header_t* tar_header) {
    char curr_version[3] = {tar_header->version[0], tar_header->version[1], '\0'};
    if (strcmp(tar_header->magic, TMAGIC) != 0) return -1;
    if (strcmp(curr_version, TVERSION) != 0) return -2;
    return 0;
}

int check_chksum(char *address_tar) {
    u_int chksum_val = 0;
    u_int chksum_calc = 0;

    for (int i = 0; i < 512; ++i) {
        if (i == 148) {chksum_val = TAR_INT(address_tar+i); chksum_calc += 32*8; i+=7;}
        else chksum_calc += *(address_tar+i);
    }

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
    lseek(tar_fd, 0, SEEK_SET); // Point at the beginning of the archive
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));

    int nbr_files = 0;
    for (; read(tar_fd, tar_header, sizeof(tar_header_t)) > 0; nbr_files++) {
        if (is_tar_eof(tar_header)) break;
//        debug_dump((uint8_t *) tar_header, sizeof(tar_header_t));

        int res = check_magic_and_version(tar_header);
        if (res == 0) res = check_chksum((char *) tar_header);
        if (res != 0) { nbr_files = res; break; }
        lseek(tar_fd, next_offset_header(tar_header), SEEK_CUR);
    }

    free(tar_header);
    return nbr_files;
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
    lseek(tar_fd, offset_header(tar_fd, path), SEEK_SET); // Point at the beginning of the file
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));
    bool checking = read(tar_fd, tar_header, sizeof(tar_header_t)) > 0;
    free(tar_header);
    return checking;
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
    lseek(tar_fd, offset_header(tar_fd, path), SEEK_SET); // Point at the beginning of the file
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));
    bool checking = read(tar_fd, tar_header, sizeof(tar_header_t)) > 0 && strcmp(tar_header->name, path) == 0 && tar_header->typeflag == DIRTYPE;
    free(tar_header);
    return checking;
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
    lseek(tar_fd, offset_header(tar_fd, path), SEEK_SET); // Point at the beginning of the file
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));
    bool checking = read(tar_fd, tar_header, sizeof(tar_header_t)) > 0 && strcmp(tar_header->name, path) == 0 && tar_header->typeflag == REGTYPE;
    free(tar_header);
    return checking;
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
    lseek(tar_fd, offset_header(tar_fd, path), SEEK_SET); // Point at the beginning of the file
    tar_header_t *tar_header = (tar_header_t *) malloc(sizeof(tar_header_t));
    bool checking = read(tar_fd, tar_header, sizeof(tar_header_t)) > 0 && strcmp(tar_header->name, path) == 0 && tar_header->typeflag == SYMTYPE;
    free(tar_header);
    return checking;
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