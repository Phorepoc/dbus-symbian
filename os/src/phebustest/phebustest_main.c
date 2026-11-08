/* phebustest_main.c -- two processes talk over the session bus (phedbusd), through the public libdbus API only. BSD-2-Clause (Phorepoc).
 * Parent: connects to the session bus (address discovery as libdbus does it: DBUS_SESSION_BUS_ADDRESS, else the compiled-in default tcp:host=127.0.0.1,port=5371); if no bus answers it starts
 * phedbusd ON DEMAND (posix_spawn) and waits for it; owns the name org.phorepoc.BusTest, answers Echo, listens for the ChildDone signal; spawns the child.
 * Child (argument "child"): second process, second connection: sees the name has an owner, cannot take it (EXISTS), calls Echo on it, emits ChildDone. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <spawn.h>
#include <dirent.h>
#include <dbus/dbus.h>

extern void phe_log (const char *msg);
#define LOG(...) do { char m_[300]; snprintf (m_, sizeof m_, __VA_ARGS__); phe_log (m_); } while (0)
#define NAME "org.phorepoc.BusTest"
#define PATH "/org/phorepoc/BusTest"
#define IFACE "org.phorepoc.BusTest"

static int got_signal, echoes_served; static char signal_from[160];

static DBusHandlerResult
filter (DBusConnection *c, DBusMessage *m, void *d)
{
  (void) d;
  if (dbus_message_is_method_call (m, IFACE, "Echo")) {
    const char *in = NULL; char out[200]; DBusMessage *r; const char *p = out;
    dbus_message_get_args (m, NULL, DBUS_TYPE_STRING, &in, DBUS_TYPE_INVALID);
    snprintf (out, sizeof out, "echo: %s", in ? in : "");
    r = dbus_message_new_method_return (m);
    dbus_message_append_args (r, DBUS_TYPE_STRING, &p, DBUS_TYPE_INVALID);
    dbus_connection_send (c, r, NULL); dbus_connection_flush (c); dbus_message_unref (r);
    echoes_served++;
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  if (dbus_message_is_signal (m, IFACE, "ChildDone")) {
    const char *who = NULL;
    dbus_message_get_args (m, NULL, DBUS_TYPE_STRING, &who, DBUS_TYPE_INVALID);
    snprintf (signal_from, sizeof signal_from, "%s (sender %s)", who ? who : "?", dbus_message_get_sender (m) ? dbus_message_get_sender (m) : "?");
    got_signal = 1;
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusConnection *
open_bus (DBusError *err)
{
  DBusConnection *c = dbus_bus_get_private (DBUS_BUS_SESSION, err);
  if (c) dbus_connection_set_exit_on_disconnect (c, FALSE);
  return c;
}

static int
child (void)
{
  DBusError err; DBusConnection *c; DBusMessage *call, *reply; const char *text = "from the child"; int ok = 1, r;
  dbus_error_init (&err);
  c = open_bus (&err);
  if (!c) { LOG ("[phebustest] child: no bus: %s", err.message ? err.message : "?"); return 1; }
  LOG ("[phebustest] child: connected as %s", dbus_bus_get_unique_name (c));
  r = dbus_bus_name_has_owner (c, NAME, &err) ? 1 : 0;
  LOG ("[phebustest] child: %s has an owner: %d", NAME, r); if (!r) ok = 0;
  r = dbus_bus_request_name (c, NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
  LOG ("[phebustest] child: request_name -> %d (EXISTS is %d)", r, DBUS_REQUEST_NAME_REPLY_EXISTS); if (r != DBUS_REQUEST_NAME_REPLY_EXISTS) ok = 0;
  call = dbus_message_new_method_call (NAME, PATH, IFACE, "Echo");
  dbus_message_append_args (call, DBUS_TYPE_STRING, &text, DBUS_TYPE_INVALID);
  reply = dbus_connection_send_with_reply_and_block (c, call, 10000, &err);
  if (reply) {
    const char *t = NULL; dbus_message_get_args (reply, NULL, DBUS_TYPE_STRING, &t, DBUS_TYPE_INVALID);
    LOG ("[phebustest] child: reply \"%s\"", t ? t : "?"); if (!t || strcmp (t, "echo: from the child")) ok = 0; dbus_message_unref (reply);
  } else { LOG ("[phebustest] child: call failed: %s", err.message ? err.message : "?"); ok = 0; }
  call = dbus_message_new_signal (PATH, IFACE, "ChildDone");
  { const char *me = ok ? "ok" : "problems"; dbus_message_append_args (call, DBUS_TYPE_STRING, &me, DBUS_TYPE_INVALID); }
  dbus_connection_send (c, call, NULL); dbus_connection_flush (c); dbus_message_unref (call);
  LOG ("[phebustest] child: signal sent, exiting (%s)", ok ? "ok" : "problems");
  return ok ? 0 : 1;
}

#define SVC "org.phorepoc.TestService"
static int svc_calls;

static DBusHandlerResult
svc_filter (DBusConnection *c, DBusMessage *m, void *d)
{
  (void) d;
  if (dbus_message_is_method_call (m, SVC, "Hello")) {
    const char *t = "hello from the service the bus started"; DBusMessage *r = dbus_message_new_method_return (m);
    dbus_message_append_args (r, DBUS_TYPE_STRING, &t, DBUS_TYPE_INVALID);
    dbus_connection_send (c, r, NULL); dbus_connection_flush (c); dbus_message_unref (r);
    svc_calls++;
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* the service process (started by the bus from org.phorepoc.TestService.service: Exec=phebustest.exe service): owns the name, answers Hello, leaves a few seconds after the answer */
static int
service (void)
{
  DBusError err; DBusConnection *c; int r, i, idle = 0;
  dbus_error_init (&err);
  LOG ("[phebustest] service: started");
  c = open_bus (&err);
  if (!c) { LOG ("[phebustest] service: no bus: %s", err.message ? err.message : "?"); return 1; }
  r = dbus_bus_request_name (c, SVC, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
  LOG ("[phebustest] service: request_name(%s) -> %d as %s", SVC, r, dbus_bus_get_unique_name (c));
  dbus_connection_add_filter (c, svc_filter, NULL, NULL);
  for (i = 0; i < 600 && idle < 30; i++) { dbus_connection_read_write_dispatch (c, 100); if (svc_calls) idle++; }
  LOG ("[phebustest] service: leaving after %d call(s)", svc_calls);
  dbus_connection_close (c);
  return 0;
}

/* a call to a name nobody owns: the bus finds org.phorepoc.TestService.service, starts the program, and delivers the call to it */
static int
activation_test (DBusConnection *c)
{
  DBusError err; DBusMessage *call, *reply; int ok = 1, has, listed = 0, n = 0, i; char **names = NULL;
  dbus_error_init (&err);
  {
    static const char *dirs[] = { "z:/resource/dbus/services", "z:\resource\dbus\services", NULL }; int k;
    for (k = 0; dirs[k]; k++) {
      DIR *d = opendir (dirs[k]); struct dirent *e; char list[200]; list[0] = 0;
      if (d) { while ((e = readdir (d)) != NULL && strlen (list) < 150) { strcat (list, e->d_name); strcat (list, " "); } closedir (d); }
      LOG ("[phebustest] info: opendir(%s) -> %s: %s", dirs[k], d ? "ok" : "failed", list);
    }
  }
  has = dbus_bus_name_has_owner (c, SVC, &err) ? 1 : 0;
  LOG ("[phebustest] activation: %s has an owner before the call: %d", SVC, has); if (has) ok = 0;
  if (dbus_bus_name_has_owner (c, "org.freedesktop.DBus", &err)) {
    DBusMessage *lc = dbus_message_new_method_call ("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListActivatableNames"), *lr;
    lr = dbus_connection_send_with_reply_and_block (c, lc, 10000, &err); dbus_message_unref (lc);
    if (lr) {
      if (dbus_message_get_args (lr, NULL, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &names, &n, DBUS_TYPE_INVALID))
        for (i = 0; i < n; i++) if (!strcmp (names[i], SVC)) listed = 1;
      if (names) dbus_free_string_array (names);
      dbus_message_unref (lr);
    }
    LOG ("[phebustest] activation: ListActivatableNames has %d name(s), %s listed: %d", n, SVC, listed); if (!listed) ok = 0;
  }
  call = dbus_message_new_method_call (SVC, "/org/phorepoc/TestService", SVC, "Hello");
  reply = dbus_connection_send_with_reply_and_block (c, call, 30000, &err);
  dbus_message_unref (call);
  if (reply) {
    const char *t = NULL; dbus_message_get_args (reply, NULL, DBUS_TYPE_STRING, &t, DBUS_TYPE_INVALID);
    LOG ("[phebustest] activation: reply \"%s\" (sender %s)", t ? t : "?", dbus_message_get_sender (reply) ? dbus_message_get_sender (reply) : "?");
    if (!t || !strstr (t, "started")) ok = 0;
    dbus_message_unref (reply);
  } else { LOG ("[phebustest] activation: call failed: %s: %s", err.name ? err.name : "?", err.message ? err.message : "?"); ok = 0; }
  has = dbus_bus_name_has_owner (c, SVC, &err) ? 1 : 0;
  LOG ("[phebustest] activation: %s has an owner after the call: %d", SVC, has); if (!has) ok = 0;
  LOG ("[phebustest] activation: %s", ok ? "PASS" : "FAIL");
  return ok;
}

int
phe_bus_test_main (int argc, char **argv)
{
  DBusError err; DBusConnection *c = NULL; int i, r, ok = 1; pid_t pid;
  if (argc > 1 && !strcmp (argv[1], "child")) return child ();
  if (argc > 1 && !strcmp (argv[1], "service")) return service ();
  dbus_error_init (&err);
  for (i = 0; i < 40 && !c; i++) {
    dbus_error_free (&err);
    c = open_bus (&err);
    if (!c && i == 0) {
      char *dargv[] = { (char *) "phedbusd.exe", (char *) "--config-file=z:/resource/dbus/session.conf", (char *) "--nofork", (char *) "--nopidfile", NULL };
      LOG ("[phebustest] no session bus (%s): starting phedbusd on demand", err.message ? err.message : "?");
      r = posix_spawn (&pid, "phedbusd.exe", NULL, NULL, dargv, NULL);
      LOG ("[phebustest] posix_spawn(phedbusd) -> %d", r);
    }
    if (!c) sleep (1);
  }
  if (!c) { LOG ("[phebustest] FAIL: no session bus after %d tries: %s", i, err.message ? err.message : "?"); return 1; }
  LOG ("[phebustest] connected to the session bus as %s (after %d tries)", dbus_bus_get_unique_name (c), i);
  r = dbus_bus_request_name (c, NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
  LOG ("[phebustest] request_name(%s) -> %d (PRIMARY_OWNER is %d)", NAME, r, DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
  if (r != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) ok = 0;
  dbus_bus_add_match (c, "type='signal',interface='" IFACE "'", &err);
  dbus_connection_add_filter (c, filter, NULL, NULL);
  {
    char *cargv[] = { (char *) "phebustest.exe", (char *) "child", NULL };
    r = posix_spawn (&pid, "phebustest.exe", NULL, NULL, cargv, NULL);
    LOG ("[phebustest] posix_spawn(child) -> %d", r); if (r) ok = 0;
  }
  for (i = 0; i < 300 && !got_signal; i++) dbus_connection_read_write_dispatch (c, 100);
  LOG ("[phebustest] signal from the other process: %s; Echo calls served: %d", got_signal ? signal_from : "none", echoes_served);
  if (!got_signal || echoes_served != 1 || !strstr (signal_from, "ok")) ok = 0;
  if (ok && !activation_test (c)) ok = 0;
  LOG ("[phebustest] done: %s", ok ? "PASS" : "FAIL");
  dbus_connection_close (c);
  return ok ? 0 : 1;
}
