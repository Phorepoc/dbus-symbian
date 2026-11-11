# Copyright The Phorepoc Authors
# SPDX-License-Identifier: BSD-2-Clause
#
# stack_dbus.sh -- sourced after glib-symbian's recipes/stack_env.sh: the D-Bus layer of the Hildon stack (libdbus, dbus-glib, the bus
# daemon, phebuscall) -> the archives in /o. Needs stack_glib and gtk-symbian's "fetch fontconfig" steps first (the daemon links expat).
stack_dbus() {
  stack_use dbus-symbian
  stack_step dbus fetch "bash /h/recipes/fetch.sh"
  stack_step dbus dbus "bash /h/recipes/build_dbus_x32.sh"
  stack_step dbus dbusglib "bash /h/recipes/build_dbusglib_x32.sh"
  stack_step dbus dbusd "bash /h/recipes/build_dbusd_x32.sh"
  stack_step dbus archives "bash /h/recipes/package_x32.sh"
  stack_need libphedbus.a libphedbusglib.a libphedbusd.a libpheexpat.a libphebuscall.a
}
