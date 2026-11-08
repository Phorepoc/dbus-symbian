/* phebuscall_main.c -- call a method on the session bus (what dbus-send does), through the public libdbus API. BSD-2-Clause (Phorepoc).
 *   phebuscall DEST PATH INTERFACE METHOD [TYPE:VALUE ...] [-- DEST PATH INTERFACE METHOD [TYPE:VALUE ...] ...]
 * With -a as first argument it does not wait for DEST to have an owner (the call itself activates it: service activation test).
 * TYPE is string, uint32, int32 or boolean (true / false). It waits (up to a minute) until DEST has an owner on the bus, then makes each call and logs the reply arguments or the error. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <dbus/dbus.h>

extern void phe_log (const char *msg);
#define LOG(...) do { char m_[400]; snprintf (m_, sizeof m_, __VA_ARGS__); phe_log (m_); } while (0)

static int
append_arg (DBusMessage *m, const char *spec)
{
  const char *colon = strchr (spec, ':');
  if (!colon) return 0;
  if (!strncmp (spec, "string", colon - spec)) { const char *v = colon + 1; return dbus_message_append_args (m, DBUS_TYPE_STRING, &v, DBUS_TYPE_INVALID); }
  if (!strncmp (spec, "uint32", colon - spec)) { dbus_uint32_t v = (dbus_uint32_t) strtoul (colon + 1, NULL, 0); return dbus_message_append_args (m, DBUS_TYPE_UINT32, &v, DBUS_TYPE_INVALID); }
  if (!strncmp (spec, "int32", colon - spec)) { dbus_int32_t v = (dbus_int32_t) strtol (colon + 1, NULL, 0); return dbus_message_append_args (m, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID); }
  if (!strncmp (spec, "boolean", colon - spec)) { dbus_bool_t v = !strcmp (colon + 1, "true"); return dbus_message_append_args (m, DBUS_TYPE_BOOLEAN, &v, DBUS_TYPE_INVALID); }
  return 0;
}

static void
log_reply (DBusMessage *r)
{
  DBusMessageIter it; int n = 0;
  if (!dbus_message_iter_init (r, &it)) { LOG ("[phebuscall]   (no reply arguments)"); return; }
  do {
    int t = dbus_message_iter_get_arg_type (&it);
    if (t == DBUS_TYPE_STRING) { const char *s; dbus_message_iter_get_basic (&it, &s); LOG ("[phebuscall]   reply[%d] string \"%s\"", n, s); }
    else if (t == DBUS_TYPE_UINT32) { dbus_uint32_t v; dbus_message_iter_get_basic (&it, &v); LOG ("[phebuscall]   reply[%d] uint32 %u", n, (unsigned) v); }
    else if (t == DBUS_TYPE_BOOLEAN) { dbus_bool_t v; dbus_message_iter_get_basic (&it, &v); LOG ("[phebuscall]   reply[%d] boolean %d", n, (int) v); }
    else LOG ("[phebuscall]   reply[%d] type %c", n, (char) t);
    n++;
  } while (dbus_message_iter_next (&it));
}

int
phe_buscall_main (int argc, char **argv)
{
  DBusError err; DBusConnection *c; int i = 1, rc = 0, tries, nowait = 0;
  static char fbuf[600], *fargv[24];
  if (argc < 2) {                                 /* no command line (booted as the start-up program of a test ROM): the arguments come from z:/resource/phebuscall.args */
    FILE *f = fopen ("z:/resource/phebuscall.args", "r");
    if (f) {
      size_t n = fread (fbuf, 1, sizeof fbuf - 1, f); char *p; fclose (f); fbuf[n] = 0;
      argc = 0; fargv[argc++] = (char *) "phebuscall";
      for (p = fbuf; *p && argc < 23; ) { while (*p == ' ' || *p == 10 || *p == 13) *p++ = 0; if (!*p) break; fargv[argc++] = p; while (*p && *p != ' ' && *p != 10 && *p != 13) p++; }
      fargv[argc] = NULL; argv = fargv;
    }
  }
  if (argc > 1 && !strcmp (argv[1], "-a")) { nowait = 1; i = 2; argc--; argv++; argv[0] = (char *) "phebuscall"; i = 1; }
  if (argc < 5) { LOG ("[phebuscall] usage: DEST PATH INTERFACE METHOD [TYPE:VALUE ...] [-- ...]"); return 2; }
  dbus_error_init (&err);
  for (tries = 0; tries < 60; tries++) {
    dbus_error_free (&err);
    c = dbus_bus_get_private (DBUS_BUS_SESSION, &err);
    if (c) break;
    if (tries == 0) {                           /* no session bus: start phedbusd on demand, as every client here */
      pid_t pid; char *dargv[] = { (char *) "phedbusd.exe", (char *) "--config-file=z:/resource/dbus/session.conf", (char *) "--nofork", (char *) "--nopidfile", NULL };
      LOG ("[phebuscall] no session bus: starting phedbusd -> %d", posix_spawn (&pid, "phedbusd.exe", NULL, NULL, dargv, NULL));
    }
    sleep (1);
  }
  if (!c) { LOG ("[phebuscall] no session bus: %s", err.message ? err.message : "?"); return 1; }
  dbus_connection_set_exit_on_disconnect (c, FALSE);
  while (i + 3 < argc) {
    const char *dest = argv[i], *path = argv[i + 1], *iface = argv[i + 2], *method = argv[i + 3]; DBusMessage *m, *r; int w;
    for (w = 0; !nowait && w < 60 && !dbus_bus_name_has_owner (c, dest, &err); w++) { dbus_error_free (&err); sleep (1); }
    LOG ("[phebuscall] %s has an owner: %d (waited %d s)", dest, dbus_bus_name_has_owner (c, dest, NULL) ? 1 : 0, w);
    m = dbus_message_new_method_call (dest, path, iface, method);
    for (i += 4; i < argc && strcmp (argv[i], "--"); i++) if (!append_arg (m, argv[i])) LOG ("[phebuscall] bad argument %s", argv[i]);
    LOG ("[phebuscall] call %s %s %s.%s", dest, path, iface, method);
    dbus_error_free (&err);
    r = dbus_connection_send_with_reply_and_block (c, m, 90000, &err);
    dbus_message_unref (m);
    if (r) { LOG ("[phebuscall]   reply ok"); log_reply (r); dbus_message_unref (r); }
    else { LOG ("[phebuscall]   error %s: %s", err.name ? err.name : "?", err.message ? err.message : "?"); rc = 3; }
    if (i < argc) i++;           /* skip the "--" */
  }
  LOG ("[phebuscall] done");
  dbus_connection_close (c);
  return rc;
}
