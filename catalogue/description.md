> The D-Bus session bus, libdbus and dbus-glib: how programs on the phone talk to each other.

D-Bus 1.16.2 for Phorepoc OS: libdbus, dbus-glib 0.114 and the session bus (`phedbusd.exe`, dbus-daemon on a local TCP address) with its configuration and a service directory, so that a program can be started on demand when someone calls it. Also the small command-line client `phebuscall.exe`. It needs the GLib package.

The libraries are LGPL and the like, kept in DLLs of their own that you can replace. The package installs every licence text and the source offer in `\resource\dbus-symbian\licences\`. Needs: GLib (glib-symbian).
