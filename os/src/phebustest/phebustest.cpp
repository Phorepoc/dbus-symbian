// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phebustest.cpp -- entry point of the two-process session bus test (phebustest_main.c); the command line is split into arguments (the child is started with the argument "child").
#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>
#include <string.h>

extern "C" void phe_log(const char* aMsg);
extern "C" int phe_bus_test_main(int argc, char** argv);

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
	argv[argc++] = (char*)"phebustest";
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
	TRAPD(err, rc = phe_bus_test_main(argc, argv));
	RDebug::Printf("[phebustest] exit rc=%d err=%d (%s)", rc, err, argc > 1 ? "child" : "parent");
	delete cleanup;
	return rc;
	}
