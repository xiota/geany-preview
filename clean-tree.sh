#!/bin/sh
# Deletes a bunch of junk put in the tree by Auto-tools

make clean

rm -f config.*
rm -rf autom4te.cache/ build-aux/ m4/
rm -f configure stamp-h1 aclocal.m4 libtool Makefile Makefile.in
rm -f compile_commands.json
