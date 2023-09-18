#include <sys/types.h>

#define SHA256_BLOCK_SIZE       64

struct sha256_context {
    u32 state[8];
    u64 count;
    u8 buf[SHA256_BLOCK_SIZE];
};

int sha256_start(struct sha256_context *s_ctx);
int sha256_process(struct sha256_context *s_ctx, const u8 *input, unsigned int len);
int sha256_end(struct sha256_context *s_ctx, u8 *out);
