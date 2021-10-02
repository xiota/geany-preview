#!/bin/sh
mkdir -p build-aux m4
autoreconf -vfi || exit $?
echo "Now run the \`./configure' script to configure the build."
echo "  See \`./configure --help' option for more information."

cd build-aux
../configure
