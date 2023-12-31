int u3phy_write_reg(void *i2c_port_base, unsigned char dev_id, unsigned char address, int value);
unsigned char u3phy_read_reg(void *i2c_port_base, unsigned char dev_id,  unsigned char address);
int u3phy_write_reg32(void *i2c_port_base, unsigned char dev_id, unsigned int addr, unsigned int data);
unsigned int u3phy_read_reg32(void *i2c_port_base, unsigned char dev_id, unsigned int addr);
unsigned int u3phy_read_reg32(void *i2c_port_base, unsigned char dev_id, unsigned int addr);
int u3phy_write_reg8(void *i2c_port_base, unsigned char dev_id, unsigned int addr, unsigned char data);
unsigned char u3phy_read_reg8(void *i2c_port_base, unsigned char dev_id, unsigned int addr);
unsigned int u3phy_readlmsk(void *i2c_port_base, unsigned char i2c_addr, unsigned int reg_addr32, unsigned int offset, unsigned int mask);
int u3phy_writelmsk(void *i2c_port_base, unsigned char i2c_addr, unsigned int reg_addr32, unsigned int offset, unsigned int mask, unsigned int data);