#include <platform/mt_i2c.h>
#include <platform/m2296.h>

/**********************************************************
  *
  *   [I2C Function For Read/Write m2296]
  *
  *********************************************************/
u32 m2296_write_byte(u8 addr, u8 value)
{
	int ret_code = 0;
	u8 write_data[2];
	u16 len;

	write_data[0]= addr;
	write_data[1] = value;
	len = 2;

	ret_code = mtk_i2c_write(m2296_I2C_ID, m2296_SLAVE_ADDR, 400, write_data, len);

	if(ret_code == 0)
		return 0; // ok
	else
		return -1; // fail
}

u32 m2296_read_byte (u8 addr, u8 *dataBuffer)
{
	int ret_code = 0;
	u16 len;
	*dataBuffer = addr;

	len = 1;

	ret_code = mtk_i2c_write_read(m2296_I2C_ID, m2296_SLAVE_ADDR, 400,
								dataBuffer, dataBuffer, len, len);

	if(ret_code == 0)
		return 0; // ok
	else
		return -1; // fail
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
u32 pmic_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT)
{
	u8 m2296_reg = 0;
	u32 ret = 0;

	ret = m2296_read_byte(RegNum, &m2296_reg);

	m2296_reg &= (MASK << SHIFT);
	*val = (m2296_reg >> SHIFT);

	return ret;
}

u32 pmic_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT)
{
	u8 m2296_reg = 0;
	u32 ret = 0;

	ret = m2296_read_byte(RegNum, &m2296_reg);

	m2296_reg &= ~(MASK << SHIFT);
	m2296_reg |= (val << SHIFT);

	ret = m2296_write_byte(RegNum, m2296_reg);

	return ret;
}

bool m2296_check_version(void)
{
    u8 version = 0;
	int err = -1;

    err = m2296_read_byte(0x44, &version);
	if (err < 0) {
		printf("eds_check_version--i2c read failed:%d \n", err);
		return err;
	}
    printf("eds_check_version-->version=0x%02x\n", (version & 0xff));
	if ((version & 0xff) == M2296_CHIP_VER) {
		return true;
	}

	return false;
}

void mt2296_sw_reset(void)
{
	int ret;

	ret = pmic_config_interface(0x00, 0x0, 0xb8, 0);
	ret = pmic_config_interface(0x01, 0x0, 0x19, 0);
	ret = pmic_config_interface(0x02, 0x38, 0x38, 0);
	ret = pmic_config_interface(0x03, 0x19, 0x19, 0);
	ret = pmic_config_interface(0x0A, 0x12, 0x3f, 0);
	ret = pmic_config_interface(0x0D, 0x12, 0x3f, 0);
	if (ret)
		printf("mt2296_sw_reset fail\n");
}

int m2296_hw_init(void)
{
	int ret;

	printf("m2296_hw_init\n");
	if (!m2296_check_version()) {
		printf("eds_hw_init-->eds_check_version failed!\n");
		return -1;
	}

	ret = pmic_config_interface(0x07, 0x80, 0xFF, 0);
	ret = pmic_config_interface(0x0E, 0x3, 0x3, 4);  //set DCDC1 shutdown when SLEEP# pulled low
	if (ret)
		printf("m2296_hw_init fail\n");
	else
		printf("m2296_hw_init done\n");

	return 0;
}

