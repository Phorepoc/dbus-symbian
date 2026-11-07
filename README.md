# dbus-symbian

libdbus 1.16.2 (`phedbus.dll`) and dbus-glib 0.114 (`phedbusglib.dll`) for Phorepoc (x32). libdbus is built from its meson source lists with a hand-written config; the unix-socket
transport is replaced by the TCP transport (`tcp:host=127.0.0.1`, ANONYMOUS authentication: Open C has no unix sockets and no peer credentials). `os/src/phedbus/phedbus_posix.c`
supplies the few POSIX calls Open C lacks (socketpair over loopback TCP, pwrite, environ; fork/exec answer ENOSYS). The bus daemon is not ported yet.
Test: `phedbustest` (libdbus's own in-process transport and the TCP transport, method call + signal).
Requires: glib-symbian (build time: the glib headers/objects in the shared volume).

Nothing here is in an OS image; it is a Catalogue download (SIS with licences and the source offer). Recipes are BSD-2-Clause, the libraries LGPL (see NOTICE).
Build: `REPO=<Symbian-ReverseEngineered checkout> bash recipes/build.sh [steps]` (Git Bash, Docker up). The four repositories sit side by side and share one docker
volume (`VOL`, default hildon-wk) as the build tree and `$REPO/.phorepoc-x86-build/hildon/` as the archive directory; the sibling repositories are mounted read-only at
/glib /gtk /dbus in every container. Plan and findings: `docs/phorepoc/hildon-port.md` in Symbian-ReverseEngineered.

Dependency graph:  glib-symbian  <-  gtk-symbian  <-  hildon-symbian  ->  dbus-symbian  ->  glib-symbian
