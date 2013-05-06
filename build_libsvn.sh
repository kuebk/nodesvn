#!/bin/bash -x

BUILDDIR=$(dirname $0)
BUILDDIR="$(readlink -f $BUILDDIR)/build"
SVNDIR="$BUILDDIR/subversion"
NEONDIR="$BUILDDIR/neon"

if [ -n "$SVNDIR/lib/libsvn_client-1.a" ]; then
    mkdir -p $SVNDIR && cd deps/subversion && ./configure --with-pic --disable-shared --without-shared --enable-debug --prefix=$SVNDIR --with-neon=$NEONDIR && make clean install
fi

