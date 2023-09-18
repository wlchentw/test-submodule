
int rpmb_key_init(unsigned char *data, unsigned int size);
int rpmb_hmac_init(unsigned char *buf, unsigned int size);
int rpmb_hmac_process(unsigned char *buf, unsigned int size);
int rpmb_hmac_done(unsigned char *outmac, unsigned int *size);
int rpmb_set_key(int (*set_key_func)(u8 *));
void rpmb_init(void);
