// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phebuscall.cpp -- entry point of the session bus method call tool (dbus-send like) (phebuscall_main.c); the command line is split into arguments (dest path interface method [type:value ...] [-- more calls]).
#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>
#include <string.h>

extern "C" void phe_log(const char* aMsg);
extern "C" int phe_buscall_main(int argc, char** argv);

extern "C" void phe_log(const char* aMsg) { RDebug::Printf("%s", aMsg); }

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TBuf<256> cl;
	User::CommandLine(cl);
	TBuf8<256> cl8;
	cl8.Copy(cl);
	static char buf[270];
	static char* argv[16];
	memcpy(buf, cl8.Ptr(), cl8.Length());
	buf[cl8.Length()] = 0;
	int argc = 0;
	argv[argc++] = (char*)"phebuscall";
	for (char* p = buf; *p && argc < 15; )
		{
		while (*p == ' ') p++;
		if (!*p) break;
		argv[argc++] = p;
		while (*p && *p != ' ') p++;
		if (*p) *p++ = 0;
		}
	argv[argc] = 0;
	TInt rc = 0;
	TRAPD(err, rc = phe_buscall_main(argc, argv));
	RDebug::Printf("[phebuscall] exit rc=%d err=%d (%s)", rc, err, argc > 1 ? "child" : "parent");
	delete cleanup;
	return rc;
	}
