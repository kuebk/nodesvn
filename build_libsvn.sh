#!/bin/bash -x

BUILDDIR=$(dirname $0)
BUILDDIR="$(readlink -f $BUILDDIR)/build"
SVNDIR="$BUILDDIR/subversion"
NEONDIR="$BUILDDIR/neon"

if [ -e "$SVNDIR/lib/libsvn_client-1.a" ]; then
    exit 0
fi

mkdir -p $SVNDIR && cd deps/subversion && ./configure --with-pic --disable-shared --without-shared --enable-debug --prefix=$SVNDIR --with-neon=$NEONDIR && make clean install
