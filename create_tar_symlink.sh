#!/usr/bin/env bash

ln -s Makefile ./symlink-makefile
ln -s . ./symlink-project
ln -s ./lib_tar.c ./symlink-lib.c

mkdir -p "dir-for-test/subdir-for-test/"

tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c *.h *.c \
symlink-makefile symlink-project Makefile ./dir-for-test ./cmake-build-debug  > symlink.tar

rm ./symlink-makefile
rm ./symlink-project
rm ./symlink-lib.c
rm -r ./dir-for-test

exit 0