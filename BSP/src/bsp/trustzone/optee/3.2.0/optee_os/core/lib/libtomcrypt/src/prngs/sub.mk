cflags-y += -Wno-unused-parameter -Wno-unused-variable

srcs-y += rng_get_bytes.c
srcs-y += rng_make_prng.c
srcs-y += sprng.c
srcs-y += rc4.c
srcs-y += yarrow.c
srcs-$(_CFG_CRYPTO_WITH_FORTUNA_PRNG) += fortuna.c
