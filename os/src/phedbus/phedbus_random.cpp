// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phedbus_random.cpp -- the platform's random numbers for libdbus (server UUID, cookies). Open C has no usable /dev/urandom here (open fails), so getrandom() (phedbus_posix.c) takes the
// kernel's random generator, Math::Random (SHA-256 based, 256-bit strength). ph_random32 is what the runtime library's arc4random expects the program to provide.
#include <e32std.h>
#include <e32math.h>

extern "C" unsigned ph_random32(void)
	{
	return (unsigned)Math::Random();
	}

extern "C" void phedbus_random_bytes(void* aBuf, unsigned aLen)
	{
	TUint8* p = static_cast<TUint8*>(aBuf);
	while (aLen)
		{
		const TInt n = aLen > 0x4000 ? 0x4000 : (TInt)aLen;
		TPtr8 d(p, n, n);
		Math::Random(d);
		p += n;
		aLen -= n;
		}
	}
