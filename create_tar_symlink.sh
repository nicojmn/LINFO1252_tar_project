#!/usr/bin/env bash

ln -s Makefile ./symlink-makefile
ln -s . ./symlink-project
ln -s ./lib_tar.c ./symlink-lib.c

mkdir -p "dir-for-test/subdir-for-test/"
touch file_test.txt

printf "Cette tâche va vous permettre de soumettre votre projet et de l'évaluer. Un logiciel de détection de plagiat sera utilisé une fois tous les projets rendus.

Ce projet est à réaliser en binôme. Il vous faut inscrire votre groupe via Moodle.

Vous avez vu au dernier cours les concepts inhérents aux systèmes de fichiers. Le projet que vous aurez à réaliser utilisera plusieurs de ces concepts. Nous nous intéresserons aux archives tar et à leur format. Toutes les informations sur le format utiles à la réalisation du projet sont disponibles au lien suivant. Votre objectif sera d'implémenter plusieurs fonctions qui manipulent une archive tar.

Un squelette du projet avec les spécifications des fonctions à implémenter est disponible ici. Elle contient :

   Un fichier header lib_tar.h avec la définition de la structure POSIX utilisée par Tar et les signatures des fonctions à implémenter.
   Un fichier lib_tar.c où vous écrirez vos implémentations.
   Un fichier tests.c que vous utiliserez pour écrire des tests.
   Un Makefile. La cible submit (make submit) permet de préparer une archive de votre projet pour la soumission. C'est cette archive que vous uploaderez sur la tâche INGInious.

Favorisez les tests locaux avant de soumettre votre solution ! Pour créer des archives tar sur votre machine :" > file_test.txt

tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c *.h *.c \
symlink-makefile symlink-project Makefile ./dir-for-test ./cmake-build-debug ./file_test.txt  > symlink.tar

rm ./symlink-makefile
rm ./symlink-project
rm ./symlink-lib.c
rm -r ./dir-for-test
rm ./file_test.txt

exit 0