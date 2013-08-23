#!/bin/bash -x

BUILDDIR=$(dirname $0)
BUILDDIR="$(readlink -f $BUILDDIR)/build"
FILEDIR="$BUILDDIR/file"

if [ -n "$FILEDIR/lib/libfile.a" ]; then
    mkdir -p $FILEDIR && cd deps/file-5.14 && ./configure --prefix=$FILEDIR && make clean install
fi

