/* phebuscall_main.c -- call a method on the session bus (what dbus-send does), through the public libdbus API. BSD-2-Clause (Phorepoc).
 *   phebuscall DEST PATH INTERFACE METHOD [TYPE:VALUE ...] [-- DEST PATH INTERFACE METHOD [TYPE:VALUE ...] ...]
 * TYPE is string, uint32, int32 or boolean (true / false). It waits (up to a minute) until DEST has an owner on the bus, then makes each call and logs the reply arguments or the error. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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
  DBusError err; DBusConnection *c; int i = 1, rc = 0, tries;
  if (argc < 5) { LOG ("[phebuscall] usage: DEST PATH INTERFACE METHOD [TYPE:VALUE ...] [-- ...]"); return 2; }
  dbus_error_init (&err);
  for (tries = 0; tries < 60; tries++) {
    dbus_error_free (&err);
    c = dbus_bus_get_private (DBUS_BUS_SESSION, &err);
    if (c) break;
    sleep (1);
  }
  if (!c) { LOG ("[phebuscall] no session bus: %s", err.message ? err.message : "?"); return 1; }
  dbus_connection_set_exit_on_disconnect (c, FALSE);
  while (i + 3 < argc) {
    const char *dest = argv[i], *path = argv[i + 1], *iface = argv[i + 2], *method = argv[i + 3]; DBusMessage *m, *r; int w;
    for (w = 0; w < 60 && !dbus_bus_name_has_owner (c, dest, &err); w++) { dbus_error_free (&err); sleep (1); }
    LOG ("[phebuscall] %s has an owner: %d (waited %d s)", dest, dbus_bus_name_has_owner (c, dest, NULL) ? 1 : 0, w);
    m = dbus_message_new_method_call (dest, path, iface, method);
    for (i += 4; i < argc && strcmp (argv[i], "--"); i++) if (!append_arg (m, argv[i])) LOG ("[phebuscall] bad argument %s", argv[i]);
    LOG ("[phebuscall] call %s %s %s.%s", dest, path, iface, method);
    dbus_error_free (&err);
    r = dbus_connection_send_with_reply_and_block (c, m, 20000, &err);
    dbus_message_unref (m);
    if (r) { LOG ("[phebuscall]   reply ok"); log_reply (r); dbus_message_unref (r); }
    else { LOG ("[phebuscall]   error %s: %s", err.name ? err.name : "?", err.message ? err.message : "?"); rc = 3; }
    if (i < argc) i++;           /* skip the "--" */
  }
  LOG ("[phebuscall] done");
  dbus_connection_close (c);
  return rc;
}
