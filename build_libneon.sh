#!/bin/bash -x

BUILDDIR=$(dirname $0)
BUILDDIR="$(readlink -f $BUILDDIR)/build"
NEONDIR="$BUILDDIR/neon"

if [ -n "$NEONDIR/lib/libneon.a" ]; then
    mkdir -p $NEONDIR && cd deps/subversion/neon && ./configure --prefix=$NEONDIR && make clean install
fi

