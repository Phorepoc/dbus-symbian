// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phedbusd_proc.cpp -- processes for the bus's service activation (phedbus_spawn.c): start an executable with a command line, wait for it to end, kill it. A process cannot be forked
// here; the service is a Symbian process (RProcess) whose command line is the Exec= arguments.
#include <e32std.h>
#include <string.h>

extern "C" int phe_proc_start(const char* aExe, const char* aArgs, void** aHandle)
	{
	TBuf<256> exe;
	TBuf<512> args;
	exe.Copy(TPtrC8((const TUint8*)aExe, strlen(aExe)));
	args.Copy(TPtrC8((const TUint8*)aArgs, strlen(aArgs)));
	RProcess* p = new RProcess;
	if (!p)
		return KErrNoMemory;
	TInt r = p->Create(exe, args);
	if (r != KErrNone)
		{
		delete p;
		return r;
		}
	p->Resume();
	*aHandle = p;
	return KErrNone;
	}

// blocks the calling thread until the process has ended
extern "C" void phe_proc_wait(void* aHandle, int* aType, int* aReason)
	{
	RProcess* p = static_cast<RProcess*>(aHandle);
	TRequestStatus st;
	p->Logon(st);
	User::WaitForRequest(st);
	*aType = (int)p->ExitType();
	*aReason = (int)p->ExitReason();
	}

extern "C" void phe_proc_kill(void* aHandle)
	{
	RProcess* p = static_cast<RProcess*>(aHandle);
	if (p->ExitType() == EExitPending)
		p->Kill(KErrAbort);
	}

extern "C" void phe_proc_close(void* aHandle)
	{
	RProcess* p = static_cast<RProcess*>(aHandle);
	p->Close();
	delete p;
	}
