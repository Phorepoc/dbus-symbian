/* sys/random.h -- getrandom() for libdbus over Open C (implemented in phedbus_posix.c on the platform's arc4random). BSD-2-Clause (Phorepoc). */
#ifndef PHEDBUS_SYS_RANDOM_H
#define PHEDBUS_SYS_RANDOM_H
#include <stddef.h>
#include <sys/types.h>
#define GRND_NONBLOCK 1
#define GRND_RANDOM 2
#ifdef __cplusplus
extern "C" {
#endif
ssize_t getrandom (void *buf, size_t buflen, unsigned flags);
#ifdef __cplusplus
}
#endif
#endif
