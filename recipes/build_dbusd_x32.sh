#!/usr/bin/env bash
# build_dbusd_x32.sh -- IN phorepoc-x86 (volume at /wk, repo :ro at /repo, this repo at /h, glib-symbian at /glib): dbus-daemon 1.16.2 (the bus: bus/*.c of the pinned tarball, no change to the source) for x32 over Open C.
# The bus code uses libdbus's internals, so the daemon is ONE executable: these objects + the whole of libphedbus.a (build_dbus_x32.sh) + expat (gtk-symbian's libpheexpat.a). No configure:
# the source list is bus/CMakeLists.txt's; SELinux, AppArmor, audit, systemd, inotify and the statistics interface are off; service activation uses os/src/phedbusd/phedbus_spawn.c (a process start in place of fork/exec) (the daemon is started on demand or from a
# start-up list by something else; services are not activated by the bus).
# Out: /wk/dbusd-x32/*.o, /wk/out/libphedbusd.a
set -eu
. /glib/recipes/pins.sh; . /gtk/recipes/pins.sh; . /h/recipes/pins.sh
CC=/wk/lsc/bin/pp-gcc
S=/wk/dbus-$DBUS_VER; D=/wk/dbus-x32; O=/wk/dbusd-x32; mkdir -p $O /wk/out
EXPAT=/wk/expat-$EXPAT_VER/lib
[ -d $EXPAT ] || { echo "no $EXPAT: build gtk-symbian's fontconfig step first (it unpacks expat)"; exit 1; }
SRC="activation apparmor audit bus config-loader-expat config-parser config-parser-common connection desktop-file dir-watch-default dispatch driver expirelist policy selinux services signals utils"
FF="-O2 -g0 -w -fno-pic -I/glib/overlay -include /glib/overlay/phe_glib_compat.h -include /h/os/src/phedbus/phedbus_compat.h -I/h/os/src/phedbus/inc -DHAVE_CONFIG_H -DDBUS_COMPILATION -DXML_STATIC -DENABLE_TRADITIONAL_ACTIVATION=1 -I$D -I$D/dbus -I$S -I$S/dbus -I$S/bus -I$EXPAT"
rm -f $O/*.o $O/errs.*; export CC FF O S
one() { f=$1; $CC $FF -c $S/bus/$f.c -o $O/$f.o 2> $O/errs.$f || echo "FAIL $f"; }
export -f one
fails=$(echo $SRC | tr ' ' '\n' | xargs -P "${JOBS:-8}" -I{} bash -c 'one {}' | tee $O/failed.txt | wc -l)
# main.c: its main() is called by the Symbian entry point (os/src/phedbusd/phedbusd.cpp)
$CC $FF -Dmain=phedbusd_main -c $S/bus/main.c -o $O/main.o 2> $O/errs.main || { echo "FAIL main"; echo main >> $O/failed.txt; }
# the spawn half of libdbus (no fork here): service activation as a process start, see the file
$CC $FF -c /h/os/src/phedbusd/phedbus_spawn.c -o $O/phedbus_spawn.o 2> $O/errs.spawn || { echo "FAIL phedbus_spawn"; echo spawn >> $O/failed.txt; }
cat $O/errs.* > $O/errors.txt 2>/dev/null || true; rm -f $O/errs.*
echo "dbus-daemon: $(ls $O/*.o 2>/dev/null | wc -l) objects, $(cat $O/failed.txt | wc -l) failed"; head -30 $O/failed.txt | tr '\n' ' '; echo
grep -a "error" $O/errors.txt | sed "s|$S/||" | cut -c1-200 | head -${ERRN:-30}
rm -f /wk/out/libphedbusd.a; ar rcs /wk/out/libphedbusd.a $O/*.o; echo "libphedbusd.a: $(ar t /wk/out/libphedbusd.a | wc -l) members"
nm -u $O/*.o | grep " U " | awk '{print $2}' | sort -u > $O/und_all.txt
nm --defined-only $O/*.o /wk/dbus-x32/*.o /wk/out/libpheexpat.a 2>/dev/null | awk 'NF==3{print $3}' | sort -u > $O/def_all.txt
comm -23 $O/und_all.txt $O/def_all.txt | grep -vx -f /wk/libc_syms.txt > $O/undefined.txt || true
echo "symbols the daemon needs that neither it, libdbus, expat nor Open C define: $(wc -l < $O/undefined.txt)"; head -40 $O/undefined.txt | tr '\n' ' '
