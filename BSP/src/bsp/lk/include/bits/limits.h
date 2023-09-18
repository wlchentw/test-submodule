#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
#define LONG_BIT 64
#endif

#if ARCH_ARM64
#define LONG_MAX  0x7fffffffffffffffL
#else
#define LONG_MAX  0x7fffffffL
#endif

#define LLONG_MAX  0x7fffffffffffffffLL
