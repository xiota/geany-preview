#!/bin/sh
# Deletes a bunch of junk put in the tree by Auto-tools

rm -f *.o */*.a */*.so */*.dll */*.lib */*.dylib */*.lo */*.la config.*
rm -f */*.o */*.a */*.so */*.dll */*.lib */*.dylib */*.lo */*.la config.*
rm -f *~ *.o *.a *.so *.dll *.lib *.dylib *.lo *.la config.*
rm -f */configure */stamp-h1 */aclocal.m4 */libtool */Makefile */Makefile.in
rm -f configure stamp-h1 aclocal.m4 libtool Makefile Makefile.in
rm -rf */.libs */.deps .deps/ .libs/ autom4te.cache/ build-aux/ m4/
