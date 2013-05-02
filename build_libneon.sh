#!/bin/bash -x

BUILDDIR=$(dirname $0)
BUILDDIR="$(readlink -f $BUILDDIR)/build"
NEONDIR="$BUILDDIR/neon"

if [ -e "$NEONDIR/lib/libneon.a" ]; then
    exit 0
fi

mkdir -p $NEONDIR && cd deps/subversion/neon && ./configure --prefix=$NEONDIR && make clean install
