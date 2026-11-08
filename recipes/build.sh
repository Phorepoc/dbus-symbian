#!/usr/bin/env bash
# build.sh -- libdbus 1.16.2 + dbus-glib 0.114 for the x32 PC image (needs glib-symbian built into the same VOL). Host side, Git Bash, Docker Desktop up.
#
#   REPO=<Symbian-ReverseEngineered checkout>  bash recipes/build.sh [steps]
# steps (default: all, in order): fetch dbus dbusglib archives defs images
# The sibling repositories (glib-symbian, gtk-symbian, dbus-symbian, hildon-ui) are expected next to this one; they are mounted at /glib /gtk /dbus (read-only) in every container,
# this repository at /h. The docker volume (VOL) is the shared build tree of the whole stack; OUTD (the archives) is $REPO/.phorepoc-x86-build/hildon.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd -W); REPO=${REPO:-C:/Users/memph/Git/Symbian-ReverseEngineered}; VOL=${VOL:-hildon-wk}; PARENT=${HERE%/*}; SIB="-v $PARENT/glib-symbian:/glib:ro -v $PARENT/gtk-symbian:/gtk:ro -v $PARENT/dbus-symbian:/dbus:ro"
STEPS=${*:-fetch dbus dbusglib archives defs images}
OUTD=$REPO/.phorepoc-x86-build/hildon; mkdir -p "$OUTD"
IMG=phorepoc-x86
dr() { MSYS_NO_PATHCONV=1 docker.exe run --rm --name "hildon-$1" -e JOBS="${JOBS:-8}" -v "$HERE":/h $SIB -v "$REPO":/repo:ro -v "$OUTD":/o -v "$VOL":/wk $IMG "${@:2}"; }
has() { case " $STEPS " in *" $1 "*) return 0;; esac; return 1; }
[ "${CLEAN:-0}" = 1 ] && { docker.exe volume rm "$VOL" >/dev/null 2>&1 || true; }
docker.exe volume create "$VOL" >/dev/null
has fetch    && { echo "== fetch";    bash "$HERE/recipes/fetch.sh"; }
has dbus      && { echo "== dbus";       dr db bash /h/recipes/build_dbus_x32.sh | tail -12; }
has dbusglib  && { echo "== dbus-glib";  dr dg bash /h/recipes/build_dbusglib_x32.sh | tail -8; }
has archives && { echo "== archives"; dr a1 bash /h/recipes/package_x32.sh; }
has defs     && { echo "== defs";     dr d1 python3 /h/recipes/gen_defs.py /o /h/os/src; }
has images   && { echo "== images";   ( cd "$REPO" && ./ph build phedbus phedbusglib phedbustest ) | tail -3; }
echo "build.sh: done ($STEPS)"
