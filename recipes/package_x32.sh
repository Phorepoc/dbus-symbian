#!/usr/bin/env bash
# package_x32.sh -- IN phorepoc-x86 (/wk, repo:/repo:ro, this repo:/h, glib-symbian:/glib, gtk-symbian:/gtk, dbus-symbian:/dbus, output dir :/o): libdbus + dbus-glib archives and the transport test
set -u; . /glib/recipes/pins.sh; . /h/recipes/pins.sh
CC=/wk/lsc/bin/pp-gcc; X=/wk/x32; S=/wk/glib-2.56.4; O=/o; mkdir -p $O /wk/out/t
INC="-I$X -I$X/glib -I$S -I$S/glib -I$S/gobject -I$S/gmodule -I/wk/ffi-x32 -I/glib/overlay -include /glib/overlay/phe_glib_compat.h"
CF="-O2 -g0 -w -fno-pic -DHAVE_CONFIG_H -DGLIB_STATIC_COMPILATION -DGOBJECT_STATIC_COMPILATION"
# D-Bus (build_dbus_x32.sh) as libphedbus.a, and its transport test (public libdbus API) as libphedbustest.a
if [ -f /wk/out/libphedbus.a ]; then
  cp /wk/out/libphedbus.a $O/libphedbus.a; echo "libphedbus.a: $(ar t $O/libphedbus.a | wc -l) members"
  mkdir -p $O/include/dbus; cp /wk/dbus-1.16.2/dbus/dbus*.h /wk/dbus-x32/dbus/dbus-arch-deps.h $O/include/dbus/ 2>/dev/null
  mkdir -p /wk/out/db; rm -f /wk/out/db/*.o $O/libphedbustest.a
  $CC $CF $INC -I/wk/dbus-1.16.2 -I/wk/dbus-x32 -I/wk/dbus-x32/dbus -I/wk/dbus-glib-$DBUSGLIB_VER -I/wk/dbusglib-x32/inc -c /h/os/src/phedbustest/phedbustest_main.c -o /wk/out/db/phedbustest_main.o || echo "DBUS TEST COMPILE FAIL"
  ar rcs $O/libphedbustest.a /wk/out/db/*.o; echo "libphedbustest.a built"
  # the two-process session bus test (phebustest) and the daemon objects (build_dbusd_x32.sh)
  $CC $CF $INC -I/wk/dbus-1.16.2 -I/wk/dbus-x32 -I/wk/dbus-x32/dbus -c /h/os/src/phebustest/phebustest_main.c -o /wk/out/phebustest_main.o || echo "BUSTEST COMPILE FAIL"
  rm -f $O/libphebustest.a; ar rcs $O/libphebustest.a /wk/out/phebustest_main.o; echo "libphebustest.a built"
  [ -f /wk/out/libphedbusd.a ] && cp /wk/out/libphedbusd.a $O/libphedbusd.a && echo "libphedbusd.a copied"
  [ -f /wk/out/libpheexpat.a ] && cp /wk/out/libpheexpat.a $O/libpheexpat.a
fi
[ -f /wk/out/libphedbusglib.a ] && cp /wk/out/libphedbusglib.a $O/libphedbusglib.a
