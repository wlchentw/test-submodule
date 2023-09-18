#include <platform/mt8512.h>
#include <platform/pwm.h>
#include <platform/mt_reg_base.h>
#include <platform/pll.h>
#include <reg.h>

void pwm_dump(int pwm_no);

#define u32		unsigned int

#define PRINTF_I	printf
#define PRINTF_W	printf
#define PRINTF_E	printf

#ifndef BIT
#define BIT(_bit_)		(u32)(1U << (_bit_))
#endif

#if 0
#define DRV_Reg32(addr)			(*(volatile u32 *)(addr))
#define DRV_WriteReg32(addr,data)	((*(volatile u32 *)(addr)) = (u32)data)
#define DRV_SetReg32(REG,BS)		((*(volatile u32*)(REG)) |= (u32)(BS))
#define DRV_ClrReg32(REG,BS)		((*(volatile u32*)(REG)) &= ~((u32)(BS)))
#else
#define DRV_Reg32(addr)			readl(addr)
#define DRV_WriteReg32(addr,data)	writel(data, addr)
#define DRV_SetReg32(addr,BS)		writel((readl(addr)) | (BS), addr)
#define DRV_ClrReg32(addr,BS)		writel((readl(addr)) & ~(BS), addr)
#endif

#define PWM_EN_REG          	0x0000
#define PWMCON              	0x00
#define PWMGDUR             	0x0c
#define PWMWAVENUM          	0x28
#define PWMDWIDTH           	0x2c
#define PWMTHRES            	0x30
#define PWM_SEND_WAVENUM	0x34
#define PWM_CK_SEL		0x210

#define PWM_BASE_ADDR		PWM_BASE
#define CLK_BASE_ADDR		CKSYS_BASE

#define PWM_CLK_DIV_MAX     	7
#define PWM_NUM_MAX         	7

#define PWM_CLK_NAME_MAIN   "main"

#define ENOMEM          12    /* Out of memory */
#define ENODEV          19    /* No such device */
#define EINVAL          22      /* Invalid argument */

#define CLK_API_IS_READY	0

/*
static const char * const pwm_clk_name[PWM_NUM_MAX] = {
	"pwm0", "pwm1", "pwm2", "pwm3", "pwm4", "pwm5", "pwm6",
};
*/

/*==========================================*/
static const unsigned long pwm_com_register[] = {
	0x0010, 0x0050, 0x0090, 0x00d0,	0x0110, 0x0150, 0x0190
};
/*==========================================*/

static int pwm_duties[PWM_NUM_MAX];
static int pwm_periods[PWM_NUM_MAX];

static int pwm_flag;

static int pwm_get_clk_rate(int pwm_no)
{
	return 26000000;
}

static void pwm_enable_main_clock(void)
{
#if CLK_API_IS_READY
#else
	DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(21));
	DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(15));
#endif
}

static void pwm_disable_main_clock(void)
{
#if CLK_API_IS_READY
#else
	DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(15));
	DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(21));
#endif
}

static void pwm_enable_fbclk(int pwm_no)
{
#if CLK_API_IS_READY
#else
	switch(pwm_no) {
		case 0:
			DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(16));
			break;

		case 1:
			DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(17));
			break;

		case 2:
			DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(18));
			break;

		case 3:
			DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(19));
			break;

		case 4:
			DRV_SetReg32(CLK_BASE_ADDR + 0x84, BIT(20));
			break;

		case 5:
			DRV_SetReg32(CLK_BASE_ADDR + 0xA8, BIT(0));
			break;

		case 6:
			DRV_SetReg32(CLK_BASE_ADDR + 0xA8, BIT(1));
			break;

		default:
			break;
	}
#endif
}

static void pwm_disable_fbclk(int pwm_no)
{
#if CLK_API_IS_READY
#else
	switch(pwm_no) {
		case 0:
			DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(16));
			break;

		case 1:
			DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(17));
			break;

		case 2:
			DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(18));
			break;

		case 3:
			DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(19));
			break;

		case 4:
			DRV_SetReg32(CLK_BASE_ADDR + 0x80, BIT(20));
			break;

		case 5:
			DRV_SetReg32(CLK_BASE_ADDR + 0xA4, BIT(0));
			break;

		case 6:
			DRV_SetReg32(CLK_BASE_ADDR + 0xA4, BIT(1));
			break;

		default:
			break;
	}
#endif
}

static int pwm_clk_enable(int pwm_no)
{
	int ret = 0;

	pwm_enable_main_clock();
	pwm_enable_fbclk(pwm_no);
	pwm_flag |= BIT(pwm_no);

	PRINTF_I("pwm_clk_enable:main:0x%x, top:0x%x, fbclk0:%d, fbclk1:%d, fbclk2:%d, fbclk3:%d\n",
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(21), 
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(15),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(16),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(17),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(18),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(19));
	return ret;
}

static void pwm_clk_disable(int pwm_no)
{
	pwm_disable_fbclk(pwm_no);
	pwm_flag &= (~ BIT(pwm_no));

	if(pwm_flag == 0)
		pwm_disable_main_clock();

	PRINTF_I("pwm_clk_enable:main:0x%x, top:0x%x, fbclk0:%d, fbclk1:%d, fbclk2:%d, fbclk3:%d\n",
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(21), 
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(15),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(16),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(17),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(18),
		DRV_Reg32(CLK_BASE_ADDR + 0x90) & BIT(19));
}

static inline u32 pwm_readl(int pwm_no, unsigned long offset)
{
	void  *reg = (void *)(PWM_BASE_ADDR + pwm_com_register[pwm_no] + offset);

	return DRV_Reg32(reg);
}

static inline void pwm_writel(int pwm_no, unsigned long offset, unsigned int val)
{
	void  *reg = (void *)(PWM_BASE_ADDR + pwm_com_register[pwm_no] + offset);

	DRV_WriteReg32(reg, val);
}

#if 0
static int pwm_config(int pwm_no, int duty_ns, int period_ns)
{
	u32 value;
	int resolution;
	u32 clkdiv = 0;
	u32 clksrc_rate;

	int data_width, thresh;

	pwm_clk_enable(pwm_no);

	/* this use pwm clock, not fixed 26M: so the period_ns and duty_ns is not as what you want from 26M clock...*/
	clksrc_rate = pwm_get_clk_rate(pwm_no);
	resolution = 1000000000 / clksrc_rate;

	while (period_ns / resolution  > 8191) {
		clkdiv++;
		resolution *= 2;
	}

	if (clkdiv > PWM_CLK_DIV_MAX) {
		PRINTF_E("period %d not supported\n", period_ns);
		return -EINVAL;
	}

	data_width = period_ns / resolution;
	thresh = duty_ns / resolution;

	if(data_width > 1)
		--data_width;
	if(thresh >= 1)
		--thresh;

	value = pwm_readl(pwm_no, PWMCON);
	value = value | BIT(15) | clkdiv;
	pwm_writel(pwm_no, PWMCON, value);

	pwm_writel(pwm_no, PWMDWIDTH, data_width);
	pwm_writel(pwm_no, PWMTHRES, thresh);

	pwm_dump(pwm_no);

	pwm_clk_disable(pwm_no);

	return 0;
}
#endif

int pwm_config_freq(int pwm_no, int freq)
{
	u32 value;
	u32 resolution = 1;
	u32 clkdiv = 0;
	u32 clksrc_rate;

	u32 data_width, thresh;

	if (freq <= 0 || freq > 26000000) {
		PRINTF_E("freq %d not supported\n", freq);
		return -EINVAL;
	}

	pwm_clk_enable(pwm_no);

	/* currently: fixed 26M */
	clksrc_rate = pwm_get_clk_rate(pwm_no);
	data_width = clksrc_rate / freq;

	while (data_width > 8191) {
		clkdiv++;
		resolution *= 2;
	}

	if (clkdiv > PWM_CLK_DIV_MAX) {
		PRINTF_E("clkdiv %d not supported\n", clkdiv);
		return -EINVAL;
	}

	data_width = data_width / resolution;
	thresh = 0;

	pwm_periods[pwm_no] = data_width;
	pwm_duties[pwm_no] = 1;

	if(data_width > 1)
		--data_width;

	value = pwm_readl(pwm_no, PWMCON);
	value = value | BIT(15) | clkdiv;
	pwm_writel(pwm_no, PWMCON, value);

	pwm_writel(pwm_no, PWMDWIDTH, data_width);
	pwm_writel(pwm_no, PWMTHRES, thresh);

	pwm_dump(pwm_no);

	pwm_clk_disable(pwm_no);

	return 0;
}

int pwm_set_duty(int pwm_no, int duty)
{
	if (duty < 0 || duty > 8191 || (pwm_periods[pwm_no] > 0 && duty > pwm_periods[pwm_no])) {
		PRINTF_E("duty %d not supported(period:%u)\n", duty, pwm_periods[pwm_no]);
		return -EINVAL;
	}

	pwm_clk_enable(pwm_no);

	pwm_duties[pwm_no] = duty;
	if(duty >= 1)
		--duty;

	PRINTF_E("pwm%d: set duty:%d\n", pwm_no, duty);
	pwm_writel(pwm_no, PWMTHRES, duty);

	pwm_dump(pwm_no);

	pwm_disable(pwm_no);

	return 0;
}

int pwm_get_period(int pwm_no)
{
	if(pwm_no < 0 || pwm_no >= PWM_NUM_MAX) {
		PRINTF_E("pwm_no: %d is too big!\n", pwm_no);
		return -1;
	}
	return pwm_periods[pwm_no];
}

int pwm_get_duty_cycle(int pwm_no)
{
	if(pwm_no < 0 || pwm_no >= PWM_NUM_MAX) {
		PRINTF_E("pwm_no: %d is too big!\n", pwm_no);
		return -1;
	}

	return pwm_duties[pwm_no];
}

int pwm_enable(int pwm_no)
{
	u32 val;

	if(pwm_no < 0 || pwm_no >= PWM_NUM_MAX) {
		PRINTF_E("pwm_no: %d is too big!\n", pwm_no);
		return -1;
	}

	pwm_clk_enable(pwm_no);

	val = DRV_Reg32(PWM_BASE_ADDR + PWM_EN_REG);
	val |= BIT(pwm_no);
	DRV_WriteReg32(PWM_BASE_ADDR + PWM_EN_REG, val);

	return 0;
}

int pwm_disable(int pwm_no)
{
	u32 val;

	if(pwm_no < 0 || pwm_no >= PWM_NUM_MAX) {
		PRINTF_E("pwm_no: %d is too big!\n", pwm_no);
		return -1;
	}

	val = DRV_Reg32(PWM_BASE_ADDR + PWM_EN_REG);
	val &= ~ BIT(pwm_no);
	DRV_WriteReg32(PWM_BASE_ADDR + PWM_EN_REG, val);

	pwm_clk_disable(pwm_no);
	
	return 0;
}

int pwm_get_send_wavenums(int pwm_no)
{
	u32 val;

	if(pwm_no < 0 || pwm_no >= PWM_NUM_MAX) {
		PRINTF_E("pwm_no: %d is too big!\n", pwm_no);
		return -1;
	}

	val = pwm_readl(pwm_no, PWM_SEND_WAVENUM);
	PRINTF_I("pwm%d: send wavenum:%u\n", pwm_no, val);

	return (int)val;
}

void pwm_dump(int pwm_no)
{
	u32 value;

	if(pwm_no < 0 || pwm_no >= PWM_NUM_MAX) {
		PRINTF_E("pwm_no: %d is too big!\n", pwm_no);
		return;
	}

	value = pwm_readl(pwm_no, PWM_SEND_WAVENUM);

	PRINTF_I("pwm%d: send wavenum:%u, duty/period:%d%%\n", pwm_no, value,
		(pwm_readl(pwm_no, PWMTHRES) + 1) * 100 / (pwm_readl(pwm_no, PWMDWIDTH) + 1));
	PRINTF_I("\tDATA_WIDTH:%u, THRESH:%u, CON:0x%x, EN:0x%x, CLK_SEL:0x%x\n",
		pwm_readl(pwm_no, PWMDWIDTH),
		pwm_readl(pwm_no, PWMTHRES),
		pwm_readl(pwm_no, PWMCON),
		DRV_Reg32(PWM_BASE_ADDR + PWM_EN_REG),
		DRV_Reg32(PWM_BASE_ADDR + PWM_CK_SEL));
}

void pwm_dump_all(void)
{
	u32 value;
	int i;

	for(i = 0; i < PWM_NUM_MAX; ++i) {
		value = pwm_readl(i, PWM_SEND_WAVENUM);
		if(value > 0) {
			pwm_dump(i);
		} else
			PRINTF_W("pwm %d: no waves!\n", i);
	}
}

// set gpio_no to mode
// return value: if return 0, means set susscessful, if return is not 0, means failed
static int pwm_set_gpio_mode(int gpio_no, int mode)
{
	u32 uval;
	PRINTF_I("pwm_set_gpio_mode +: sizeof(uintptr_t):%zu\n", sizeof(uintptr_t));
	if(gpio_no == 88 && mode == 5) {
		PRINTF_I("pwm_set_gpio_mode 88 5\n");
		// switch GPIO88 to pwm mode 5:  bit[11~9]: value: 5
		uval = DRV_Reg32(GPIO_BASE + 0x410);
		PRINTF_I("0x10005410: 0x%x\n", uval);
		
		DRV_WriteReg32(GPIO_BASE + 0x410, (uval & 0xFFFFF1FF) | 0xA00);

		uval = DRV_Reg32(GPIO_BASE + 0x410);
		PRINTF_I("after set to gpio%d to pwm mode %d: 0x10005410: 0x%x\n", gpio_no, mode, uval);

		// verify gpio88 is mode 5
		if((uval & 0xA00) != 0xA00) {
			PRINTF_E("gpio 88 is not mode 5!\n");
			return -1;
		}
		return 0;
	}
	return -2;
}

// set the specified pwm to specified freq: eg: 0, 32000 means: pwm0, 32K
// return value: if return 0, means set susscessful, if return is not 0, means failed
static int pwm_set_pwm_freq(int pwm_no, int freq)
{
	int ret;
	int val;
	
	PRINTF_I("pwm_set_pwm_freq: pwm_no:%d, freq:%d\n", pwm_no, freq);
	
	ret = pwm_config_freq(pwm_no, freq);
	if(ret == 0) {
		val = pwm_get_period(pwm_no);
		ret = pwm_set_duty(pwm_no, val / 2);	// duty is set to period / 2
		if(ret == 0)
			ret = pwm_enable(0);
		else {
			PRINTF_E("set duty failed!\n");
			goto error;
		}

		if(ret == 0) {
			PRINTF_I("enable pwm %d successfully!\n", 0);
 		} else {
			PRINTF_E("enable pwm %d failed!\n", 0);
			goto error;
		}
	} else {
		PRINTF_E("config freq failed!\n");
		goto error;
	}

	return 0;
	
error:
	return -1;
}

// #define PWM_TEST
void pwm_init (void)
{
#ifdef PWM_TEST
	int ret;
#endif

	PRINTF_I("[pwm_init] Init Start..................\n");

	#if CLK_API_IS_READY
		// get clock here
	#endif

#ifdef PWM_TEST
	ret = pwm_set_gpio_mode(88, 5);
	if (ret != 0) {
		PRINTF_E("set gpio 88 to mode 5 failed!\n");
		return;
	}
	ret = pwm_set_pwm_freq(0, 1000000);
	if (ret != 0) {
		PRINTF_E("pwm_set_pwm_freq: set pwm_no:%d to freq:%d failed!\n", 0, 1000000);
		return;
	}
	
	PRINTF_I("Now, we stop here to let you check: the pwm wave should be working!\n");

	#if 0
		while(1)
			;
	#endif
#endif	
}

