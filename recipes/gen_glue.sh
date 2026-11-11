#!/usr/bin/env bash
# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
# gen_glue.sh PREFIX MODE IN.xml OUT.h -- the dbus-glib glue of a D-Bus interface, made by the REAL dbus-binding-tool of the pinned dbus-glib tarball, built NATIVELY (it runs on the build host's
# container, it is not part of any image): dbus-binding-tool --prefix=PREFIX --mode=glib-server|glib-client IN.xml > OUT.h.
# IN PHOREPOC-X86 (volume at /wk, this repo at /h, glib-symbian at /glib). The tool is compiled from dbus/dbus-binding-tool-glib.c + dbus-glib-tool.c + the IDL parser of dbus-glib 0.114 against
# the container's own glib, libdbus and expat development packages (installed with apt on first use: dbus-binding-tool only reads XML and writes C, so the libraries it links natively
# are not what the image gets); the build tree is cached in /wk/dbusglib-nat.
set -eu
. /glib/recipes/pins.sh; . /dbus/recipes/pins.sh
PREFIX=$1; MODE=$2; IN=$3; OUT=$4
export DEBIAN_FRONTEND=noninteractive
if ! dpkg -s libglib2.0-dev libdbus-1-dev libexpat1-dev make pkg-config >/dev/null 2>&1; then
  apt-get update -qq >/dev/null
  apt-get install -y -qq --no-install-recommends libglib2.0-dev libdbus-1-dev libexpat1-dev make pkg-config >/dev/null
fi
S=/wk/dbus-glib-$DBUSGLIB_VER; N=/wk/dbusglib-nat
[ -d $S ] || { cd /wk; tar xzf /dbus/dl/dbus-glib-$DBUSGLIB_VER.tar.gz; }
mkdir -p $N
if [ ! -x $N/dbus/dbus-binding-tool ]; then
  cd $N
  $S/configure --disable-gtk-doc --disable-tests --disable-static --disable-bash-completion CFLAGS="-O1 -w" > configure.log 2>&1 || { tail -20 configure.log; exit 1; }
  { make -C dbus-gmain -j4 && make -C dbus -j4 dbus-binding-tool; } > make.log 2>&1 || { tail -20 make.log; exit 1; }
fi
mkdir -p "$(dirname "$OUT")"
$N/dbus/dbus-binding-tool --prefix="$PREFIX" --mode="$MODE" --output="$OUT" "$IN"
echo "gen_glue: $IN -> $OUT ($(wc -l < "$OUT") lines, $MODE, prefix $PREFIX)"
