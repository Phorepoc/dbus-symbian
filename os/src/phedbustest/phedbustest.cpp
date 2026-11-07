// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phedbustest.cpp -- D-Bus test: libdbus over the real transport (a TCP server on the loopback, a client, a method call and a signal), in phedbustest_main.c. This wrapper brings the network
// up the way an application on this system does: an RConnection started on the default access point, kept open while the test runs (a TCP/IP stack with no connection refuses to bind).
#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>
#include <es_sock.h>
#include <in_sock.h>
#include <stdlib.h>

extern "C" void phe_log(const char* aMsg);
extern "C" int phe_dbus_test_main(void);

extern "C" void phe_log(const char* aMsg) { RDebug::Printf("%s", aMsg); }


// What does the stack itself say about loopback with no connection up? (Open C refuses: its socket is opened lazily and needs a connection.)
static void ProbeNative(RSocketServ& aSs)
	{
	TInetAddr lo(KInetAddrLoop, 0);
	for (TInt variant = 0; variant < 2; variant++)
		{
		RConnection c;
		RSocket sock;
		TInt r = KErrNone;
		if (variant == 1) r = c.Open(aSs);
		RDebug::Printf("[phedbustest] native loopback variant %d (%s): connection open %d", variant, variant ? "RConnection opened, not started" : "no connection", r);
		r = (variant == 1) ? sock.Open(aSs, KAfInet, KSockStream, KProtocolInetTcp, c) : sock.Open(aSs, KAfInet, KSockStream, KProtocolInetTcp);
		RDebug::Printf("[phedbustest]   RSocket::Open -> %d", r);
		if (r == KErrNone)
			{
			r = sock.Bind(lo);
			TInetAddr got; sock.LocalName(got);
			RDebug::Printf("[phedbustest]   Bind(127.0.0.1:0) -> %d, local port %d", r, (TInt)got.Port());
			if (r == KErrNone) RDebug::Printf("[phedbustest]   Listen -> %d", sock.Listen(1));
			sock.Close();
			}
		c.Close();
		}
	}

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	RSocketServ ss;
	RConnection conn;
	TInt r = ss.Connect();
	if (r == KErrNone) ProbeNative(ss);
	if (r == KErrNone) r = conn.Open(ss);
	if (r == KErrNone) r = conn.Start();
	RDebug::Printf("[phedbustest] network connection started: %d", r);
	TInt rc = 0;
	TRAPD(err, rc = phe_dbus_test_main());
	RDebug::Printf("[phedbustest] exit rc=%d err=%d", rc, err);
	conn.Close();
	ss.Close();
	delete cleanup;
	return rc;
	}
