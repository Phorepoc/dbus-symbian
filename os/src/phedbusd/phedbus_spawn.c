/* Copyright The Phorepoc Authors */
/* SPDX-License-Identifier: BSD-2-Clause */
/* phedbus_spawn.c -- the spawn half of libdbus (dbus-spawn.h) for this system: what the bus uses to start a service it has been asked to activate. BSD-2-Clause (Phorepoc).
 *
 * libdbus's own dbus-spawn-unix.c forks, execs through a babysitter process and reads three pipes; a process cannot be forked here. This file keeps the interface the bus uses
 * (_dbus_spawn_async_with_babysitter, the babysitter's watch, result function, exit status and error) and does the work the platform's way: the service executable is started
 * as a process (RProcess, phedbusd_proc.cpp) with the Exec= arguments as its command line, and a thread waits for that process to end and tells the bus's main loop through a
 * socket pair, which the bus watches exactly like the babysitter pipe. Compiled with the daemon (build_dbusd_x32.sh); the daemon is the only user. */
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include "dbus/dbus-internals.h"
#include "dbus/dbus-sysdeps.h"
#include "dbus/dbus-spawn.h"
#include "dbus/dbus-watch.h"

/* phedbusd_proc.cpp */
extern int phe_proc_start (const char *exe, const char *args, void **handle);
extern void phe_proc_wait (void *handle, int *type, int *reason);
extern void phe_proc_kill (void *handle);
extern void phe_proc_close (void *handle);

/* what the waiting thread and the babysitter share. state: 0 running, 1 abandoned by the babysitter (the thread frees it), 2 ended (the babysitter frees it) */
struct waiter
{
  void *proc; int wfd;
  volatile int state; volatile int exit_type, exit_reason;
};

struct DBusBabysitter
{
  int refcount;
  char *log_name;
  DBusWatchList *watches;
  DBusWatch *watch;
  int rfd, wfd;                 /* the socket pair the waiting thread signals on */
  struct waiter *w;             /* the process and the thread waiting for it */
  pthread_t thread; int thread_started;
  int exit_type, exit_reason;
  int exited;                   /* seen by the main loop */
  int exec_errnum;
  DBusBabysitterFinishedFunc finished_cb;
  void *finished_data;
};

static void *
wait_thread (void *arg)
{
  struct waiter *w = arg; int type = 0, reason = 0; char b = 1;
  phe_proc_wait (w->proc, &type, &reason);
  w->exit_type = type; w->exit_reason = reason;
  if (__sync_val_compare_and_swap (&w->state, 0, 2) == 0) {
    (void) write (w->wfd, &b, 1);          /* the babysitter is still there: tell its main loop */
    return NULL;
  }
  /* abandoned (the bus is done with this activation and left the service running): the service has ended now, clean up */
  phe_proc_close (w->proc); free (w);
  return NULL;
}

static void
drop_watch (DBusBabysitter *s)
{
  if (s->watch) {
    _dbus_watch_list_remove_watch (s->watches, s->watch);
    _dbus_watch_invalidate (s->watch);
    _dbus_watch_unref (s->watch);
    s->watch = NULL;
  }
}

static dbus_bool_t
handle_watch (DBusWatch *watch, unsigned int condition, void *data)
{
  DBusBabysitter *s = _dbus_babysitter_ref (data);
  (void) watch;
  if (condition & (DBUS_WATCH_READABLE | DBUS_WATCH_HANGUP | DBUS_WATCH_ERROR)) {
    char b;
    if (read (s->rfd, &b, 1) == 1 || (s->w && s->w->state == 2)) {
      s->exited = TRUE;
      if (s->w) { s->exit_type = s->w->exit_type; s->exit_reason = s->w->exit_reason; }
    }
  }
  if (s->exited) drop_watch (s);
  if (s->exited && s->finished_cb) {
    DBusBabysitterFinishedFunc cb = s->finished_cb;
    s->finished_cb = NULL;
    cb (s, s->finished_data);
  }
  _dbus_babysitter_unref (s);
  return TRUE;
}

dbus_bool_t
_dbus_spawn_async_with_babysitter (DBusBabysitter **sitter_p, const char *log_name, char *const *argv, char *const *env, DBusSpawnFlags flags,
                                   DBusSpawnChildSetupFunc child_setup, void *user_data, DBusError *error)
{
  DBusBabysitter *s; char args[512]; int i, fds[2], r; size_t n = 0;
  (void) env; (void) flags; (void) child_setup; (void) user_data;
  *sitter_p = NULL;
  if (!argv || !argv[0]) { dbus_set_error (error, DBUS_ERROR_BAD_ADDRESS, "no program to run"); return FALSE; }
  s = dbus_new0 (DBusBabysitter, 1);
  if (!s) { dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL); return FALSE; }
  s->refcount = 1; s->rfd = s->wfd = -1;
  s->log_name = _dbus_strdup (log_name ? log_name : argv[0]);
  s->watches = _dbus_watch_list_new ();
  if (!s->log_name || !s->watches) { dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL); goto fail; }
  args[0] = 0;
  for (i = 1; argv[i]; i++) {
    size_t l = strlen (argv[i]);
    if (n + l + 2 >= sizeof args) break;
    if (n) args[n++] = ' ';
    memcpy (args + n, argv[i], l); n += l; args[n] = 0;
  }
  if (socketpair (AF_INET, SOCK_STREAM, 0, fds) != 0) { dbus_set_error (error, _dbus_error_from_errno (errno), "Failed to create the exit notification socket: %s", _dbus_strerror (errno)); goto fail; }
  s->rfd = fds[0]; s->wfd = fds[1];
  s->watch = _dbus_watch_new (s->rfd, DBUS_WATCH_READABLE, TRUE, handle_watch, s, NULL);
  if (!s->watch || !_dbus_watch_list_add_watch (s->watches, s->watch)) {
    if (s->watch) { _dbus_watch_invalidate (s->watch); _dbus_watch_unref (s->watch); s->watch = NULL; }
    dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL); goto fail;
  }
  s->w = calloc (1, sizeof *s->w);
  if (!s->w) { dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL); goto fail; }
  s->w->wfd = s->wfd;
  r = phe_proc_start (argv[0], args, &s->w->proc);
  if (r != 0) {
    s->exec_errnum = ENOENT;
    dbus_set_error (error, DBUS_ERROR_SPAWN_EXEC_FAILED, "Failed to execute program %s: error %d", s->log_name, r);
    goto fail;
  }
  if (pthread_create (&s->thread, NULL, wait_thread, s->w) != 0) {
    phe_proc_kill (s->w->proc);
    dbus_set_error (error, DBUS_ERROR_SPAWN_FORK_FAILED, "Failed to create the thread that waits for %s", s->log_name);
    goto fail;
  }
  s->thread_started = 1;
  *sitter_p = s;
  return TRUE;
fail:
  _dbus_babysitter_unref (s);
  return FALSE;
}

void
_dbus_babysitter_set_result_function (DBusBabysitter *sitter, DBusBabysitterFinishedFunc finished, void *user_data)
{
  sitter->finished_cb = finished; sitter->finished_data = user_data;
}

DBusBabysitter *
_dbus_babysitter_ref (DBusBabysitter *sitter)
{
  sitter->refcount++;
  return sitter;
}

void
_dbus_babysitter_kill_child (DBusBabysitter *sitter)
{
  if (sitter->w && sitter->w->proc && sitter->w->state == 0) phe_proc_kill (sitter->w->proc);
}

void
_dbus_babysitter_unref (DBusBabysitter *sitter)
{
  if (--sitter->refcount > 0) return;
  drop_watch (sitter);
  if (sitter->w) {
    if (sitter->thread_started) {
      /* the service may still be running (the bus lets it run once it has claimed its name): leave it to the thread, which frees the shared part when the service ends */
      if (__sync_val_compare_and_swap (&sitter->w->state, 0, 1) == 0) { pthread_detach (sitter->thread); sitter->w = NULL; }
      else { pthread_join (sitter->thread, NULL); }
    }
    if (sitter->w) { if (sitter->w->proc) phe_proc_close (sitter->w->proc); free (sitter->w); sitter->w = NULL; }
  }
  if (sitter->rfd >= 0) close (sitter->rfd);
  if (sitter->wfd >= 0) close (sitter->wfd);
  if (sitter->watches) _dbus_watch_list_free (sitter->watches);
  dbus_free (sitter->log_name);
  dbus_free (sitter);
}

dbus_bool_t
_dbus_babysitter_get_child_exited (DBusBabysitter *sitter)
{
  return sitter->exited;
}

dbus_bool_t
_dbus_babysitter_get_child_exit_status (DBusBabysitter *sitter, int *status)
{
  if (!sitter->exited || sitter->exit_type != 0) return FALSE;     /* 0 = EExitKill: the process ended by itself */
  *status = sitter->exit_reason & 0xff;
  return TRUE;
}

void
_dbus_babysitter_set_child_exit_error (DBusBabysitter *sitter, DBusError *error)
{
  if (!sitter->exited) return;
  if (sitter->exec_errnum)
    dbus_set_error (error, DBUS_ERROR_SPAWN_EXEC_FAILED, "Failed to execute program %s: %s", sitter->log_name, _dbus_strerror (sitter->exec_errnum));
  else if (sitter->exit_type == 0)
    dbus_set_error (error, DBUS_ERROR_SPAWN_CHILD_EXITED, "Process %s exited with status %d", sitter->log_name, sitter->exit_reason & 0xff);
  else
    dbus_set_error (error, DBUS_ERROR_SPAWN_CHILD_SIGNALED, "Process %s %s (reason %d)", sitter->log_name, sitter->exit_type == 2 ? "panicked" : "was terminated", sitter->exit_reason);
}

dbus_bool_t
_dbus_babysitter_set_watch_functions (DBusBabysitter *sitter, DBusAddWatchFunction add_function, DBusRemoveWatchFunction remove_function,
                                      DBusWatchToggledFunction toggled_function, void *data, DBusFreeFunction free_data_function)
{
  return _dbus_watch_list_set_functions (sitter->watches, add_function, remove_function, toggled_function, data, free_data_function);
}

void
_dbus_babysitter_block_for_child_exit (DBusBabysitter *sitter)
{
  if (sitter->thread_started && sitter->w) {
    pthread_join (sitter->thread, NULL); sitter->thread_started = 0; sitter->exited = TRUE;
    sitter->exit_type = sitter->w->exit_type; sitter->exit_reason = sitter->w->exit_reason;
  }
}
