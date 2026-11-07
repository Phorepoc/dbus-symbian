/* phedbus_compat.h -- what libdbus expects of the C library and Open C lacks. Included with -include. BSD-2-Clause (Phorepoc). */
#ifndef PHEDBUS_COMPAT_H
#define PHEDBUS_COMPAT_H
#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/resource.h>
/* the type with the strictest fundamental alignment (C11); Open C predates it */
typedef union { long long phe_ll; long double phe_ld; void *phe_p; } max_align_t;
/* getrandom (phedbus_posix.c, on arc4random): libdbus includes <sys/random.h> only under HAVE_SYS_RANDOM_H */
#define GRND_NONBLOCK 1
#ifdef __cplusplus
extern "C"
#endif
ssize_t getrandom (void *buf, size_t buflen, unsigned flags);
/* setrlimit (phedbus_posix.c): Open C has the structure and RLIMIT_NOFILE but no functions (getrlimit comes from the runtime library); the descriptor table is fixed at OPEN_MAX */
int setrlimit (int resource, const struct rlimit *rlp);
#endif
