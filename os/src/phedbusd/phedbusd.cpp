// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phedbusd.cpp -- the Symbian entry point of dbus-daemon (bus/main.c of the pinned 1.16.2 tarball, built with -Dmain=phedbusd_main, see recipes/build_dbusd_x32.sh): the process command line is split into
// arguments the way a shell would, and the daemon's own main runs. Typical use:  phedbusd --config-file=z:\resource\dbus\session.conf --nofork --nopidfile --print-address
#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>
#include <string.h>
#include <stdlib.h>

extern "C" int phedbusd_main(int argc, char** argv);

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TBuf<512> cl;
	User::CommandLine(cl);
	TBuf8<512> cl8;
	cl8.Copy(cl);
	static char buf[520];
	static char* argv[32];
	memcpy(buf, cl8.Ptr(), cl8.Length());
	buf[cl8.Length()] = 0;
	int argc = 0;
	argv[argc++] = (char*)"dbus-daemon";
	for (char* p = buf; *p && argc < 31; )
		{
		while (*p == ' ') p++;
		if (!*p) break;
		argv[argc++] = p;
		while (*p && *p != ' ') p++;
		if (*p) *p++ = 0;
		}
	argv[argc] = 0;
	RDebug::Printf("[phedbusd] start, %d argument(s)", argc - 1);
	TInt rc = 0;
	TRAPD(err, rc = phedbusd_main(argc, argv));
	RDebug::Printf("[phedbusd] exit rc=%d err=%d", rc, err);
	delete cleanup;
	return rc;
	}
