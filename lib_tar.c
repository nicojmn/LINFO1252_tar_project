#include "lib_tar.h"
#include <stdbool.h>

void debug(const uint8_t *bytes, size_t len) {
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

/**
 * Check if the magic and version values are valid.
 *
 * @param tar_header The header of the current file.
 * @return -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *          0 otherwise
 */
int check_magic_and_version(tar_header_t* tar_header) {
    char curr_version[3] = {tar_header->version[0], tar_header->version[1], '\0'};
    if (strcmp(tar_header->magic, TMAGIC) != 0) return -1;
    if (strcmp(curr_version, TVERSION) != 0) return -2;
    return 0;
}

/**
 * Calculate the checksum of the current header file in the tar archive.
 *
 * @param address_tar the address of the current file header
 * @return -3 if the archive contains a header with an invalid checksum value,
 *          0 otherwise
 */
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
 * Check if the elem_path is in the main_path
 *
 * @param main_path a path
 * @param elem_path a path
 * @return true if elem_path is in main_path,
 *         false otherwise
 */
bool is_in_folder(const char* main_path, const char* elem_path) {
    for (int i = 0; main_path[i] != '\0' ; ++i) {
        if (elem_path[i] == '\0') return false;
        if (main_path[i] != elem_path[i]) return false;
    }
    return true;
}

/**
 * Find the last backslash '/' in the path
 *
 * @param path a path
 * @return a positive number which represents the index of the last '/' in the path.
 */
int index_last_backslash(char *path) {
    u_int last = strlen(path)-1; int i;
    for (i = (int) last; (i >= 0 && path[i] != '/'); i--);
//    if (last == i) return i;
    return i;
}

/**
 * Add a '/' at the end of the path.
 *
 * @param path a path
 */
void dir_parser(char *path) {
    strcat(path, "/");
}

/**
 * Redirect the linked path pointed by the symlink path
 *
 * @param sym_path a path of a symlink
 * @param link_name the filename of the file linked
 * @return the new full path to the link file.
 */
char* redirect_linked_path(char *sym_path, char *link_name) {
    int parent_path_index = index_last_backslash(sym_path);
    uint len_link_name = strlen(link_name);
    if (len_link_name + parent_path_index+1 >= 100) return NULL;

    char *link_path = malloc(sizeof(char)*100);
    memcpy(link_path, sym_path, parent_path_index+1);
    memcpy(link_path+parent_path_index+1, link_name, len_link_name);
    return link_path;
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
    lseek(tar_fd, offset_header(tar_fd, path), SEEK_SET); // Point at the beginning of the file
    tar_header_t *tar_header = malloc(sizeof(tar_header_t));

    char *main_path; bool was_sym = false;
    if (read(tar_fd, tar_header, sizeof(tar_header_t)) <= 0) { // path not found
        free(tar_header);
        *no_entries = 0;
        return 0;

    } else if (tar_header->typeflag != DIRTYPE) { // path is not a directory
        if (tar_header->typeflag == SYMTYPE || tar_header->typeflag == LNKTYPE) { // path is a symlink
            char* link_path = redirect_linked_path(path, tar_header->linkname);
            lseek(tar_fd, offset_header(tar_fd, link_path), SEEK_SET);
            long result = read(tar_fd, tar_header, sizeof(tar_header_t));

            if (result <= 0) { // link_path not found
                dir_parser(link_path); // Maybe a directory
                lseek(tar_fd, offset_header(tar_fd, link_path), SEEK_SET);
                result = read(tar_fd, tar_header, sizeof(tar_header_t));

                if (result <= 0 || tar_header->typeflag != DIRTYPE) { // link_path not found || not a directory
                    free(tar_header); free(link_path); *no_entries = 0; return 0;
                }
            }
            was_sym = true;
            main_path = link_path;

        } else { free(tar_header); *no_entries = 0; return 0;}

    } else { main_path = path;}

    /** !!! AT THIS STATE : tar_header exists && is a directory !!! **/

    size_t nbr_curr_files = 0;
    char *sub_dir = malloc(sizeof(char)*100); bool curr_sub_dir_flag = false;

    lseek(tar_fd, next_offset_header(tar_header), SEEK_CUR);
    while (read(tar_fd, tar_header, sizeof(tar_header_t)) > 0){// || !end_folder || nbr_folder_needed > 0) {
        if (!is_in_folder(main_path, tar_header->name) || nbr_curr_files >= *no_entries) break;
        /** !!! AT THIS STATE : tar_header exists && is in the directory analysed !!! **/

        if (curr_sub_dir_flag) {if (!is_in_folder(sub_dir, tar_header->name)) curr_sub_dir_flag = false;} // Out of the sub-dir
        if (!curr_sub_dir_flag) {
            if (tar_header->typeflag == DIRTYPE) { // Is sub-dir
                memcpy(sub_dir, tar_header->name, sizeof(char)*100);
                curr_sub_dir_flag = true;
                memcpy(entries[nbr_curr_files], tar_header->name, sizeof(char)*100); nbr_curr_files++;

            } else if (tar_header->typeflag == REGTYPE || tar_header->typeflag == AREGTYPE || tar_header->typeflag == SYMTYPE || tar_header->typeflag == LNKTYPE) { // Is file
                memcpy(entries[nbr_curr_files], tar_header->name, sizeof(char)*100); nbr_curr_files++;
            }
        }

        lseek(tar_fd, next_offset_header(tar_header), SEEK_CUR);
    }

    *no_entries = nbr_curr_files;
    free(sub_dir);
    free(tar_header);
    if (was_sym) { free(main_path); main_path = NULL;}
    return 1;
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