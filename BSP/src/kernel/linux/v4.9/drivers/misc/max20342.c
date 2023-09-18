#include "max20342.h"
#include <linux/power/ntx_charger_type.h>
#include "../power/supply/ntx_charger_misc.h"

//extern int thread_stopped_flag; // For USBC

static volatile int giChargerType = MAX20342_NOT_EXIST;
static volatile int giChargerVendorType = 0;
struct usb_cfg g_usb_cfg;

static int g_AutoDetect = 1;
static int g_USB_role = USB_DEFAULT;	
static int g_USB_plug = 0;					// 0: unplug , 1:plug
static int g_USB_charger_detect_type = NO_CHARGER_PLUGGED;

static struct max20342_data max20342_board_data;

static int max20342_read_reg(struct i2c_client *client, int reg, u8 *val)
{
	s32 ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev,
			"i2c read fail: can't read from %02x: %d\n",
			reg, ret);
		return ret;
	}
	*val = ret;
	return 0;
}

static int max20342_write_reg(struct i2c_client *client, int reg, u8 val)
{
	s32 ret;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev,
			"i2c write fail: can't write %02x to %02x: %d\n",
			val, reg, ret);
		return ret;
	}
	return 0;
}

static int max20342_read_block_reg(struct i2c_client *client, u8 reg,
				  u8 length, u8 *val)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, length, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed to block read reg 0x%x: %d\n",
				reg, ret);
		return ret;
	}

	return 0;
}


static int max20342_get_charger_type(void)
{
	pr_info("%s() type=%d\n",__func__,g_USB_charger_detect_type);
	return g_USB_charger_detect_type;
}


/*******************************************************
Function: Control otg_5v_gpio on / off
Noted: 
********************************************************/
static void otg_5v_en(int en)
{
	mutex_lock(&max20342_board_data.input_role_mutex);
	
	if(max20342_board_data.host_power_on){
		if (en) {
			printk(KERN_ERR"[MAX20342] OPEN OTG_5V  1 \n");
			if( gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio) )
				gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 1);
			else
				printk(KERN_INFO"[MAX20342] OTG_5V is not set !!");
		} else {
			printk(KERN_ERR"[MAX20342] OPEN OTG_5V  0 \n");
			if( gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio) )
				gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 0);
			else
				printk(KERN_INFO"[MAX20342] OTG_5V is not set !!");
		}
	} else
		printk(KERN_ERR"[MAX20342] HOST_POWER is 0\n");
		
	mutex_unlock(&max20342_board_data.input_role_mutex);
	
	return;
}

static int fsa_usb_mode = 4;

/*******************************************************
Function: Set USB as Host Role (Kernel)

Noted: Copy from driver/usb/sunxi_usb/manager/usbc_platform.c
********************************************************/
static void usb_host_choose(void)
{
	#if 0
	thread_run_flag = 0;

	while (!thread_stopped_flag) {
		printk("%s() waiting for usb scanning thread stop ...\n",__func__);
		msleep(1000);
	}
	//g_usb_cfg.port.port_type = USB_PORT_TYPE_HOST;

	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);

	hw_insmod_usb_host();
	usb_msg_center(&g_usb_cfg);
	#endif

	switch(fsa_usb_mode) {
		case 0:
		case 3:
		case 4:
		default:
		case 2:
			break;
		case 1:
			printk(KERN_ERR"%s (%d) : usb_device_switch mt_usb_disconnect\n ",__func__,__LINE__);
			mt_usb_disconnect();
			break;
	}
	fsa_usb_mode = 0;
	printk(KERN_ERR"%s (%d) : usb_device_switch mt_usb_host_connect\n ",__func__,__LINE__);
	mt_usb_host_connect();
	fsa_usb_mode = 2;/*host mode*/


}

/*******************************************************
Function: Set USB as Device Role (Kernel)

Noted: Copy from driver/usb/sunxi_usb/manager/usbc_platform.c
********************************************************/
static void usb_device_choose(void)
{
	#if 0
	thread_run_flag = 0;

	while (!thread_stopped_flag) {
		printk("%s() waiting for usb scanning thread stop ...\n",__func__);
		msleep(1000);
	}

	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);

	hw_insmod_usb_device();
	usb_msg_center(&g_usb_cfg);
	#endif

	switch(fsa_usb_mode) {
		case 0:
		case 3:
		case 4:
		default:
		case 1:
			break;
		case 2:
			printk(KERN_ERR"%s (%d) : usb_device_switch mt_usb_host_disconnect\n ",__func__,__LINE__);
			mt_usb_host_disconnect();
			break;
	}
	fsa_usb_mode = 0;
	printk(KERN_ERR"%s (%d) : usb_device_switch mt_usb_connect\n ",__func__,__LINE__);
	//mt_usb_connect();
	config_battery_charger(g_USB_charger_detect_type);
	fsa_usb_mode = 1;/*device mode*/

}

/*******************************************************
Function: Set USB as Null Role

Noted: Copy from driver/usb/sunxi_usb/manager/usbc_platform.c
********************************************************/
static void usb_null_choose(void)
{
	max20342_board_data.mode_configuring = 1;
	#if 0
	thread_run_flag = 0;

	while (!thread_stopped_flag) {
		printk("%s() waiting for usb scanning thread stop ...\n",__func__);
		msleep(1000);
	}
	hw_rmmod_usb_host();
	hw_rmmod_usb_device();
	usb_msg_center(&g_usb_cfg);
	#endif

	if(fsa_usb_mode==2) {
		printk(KERN_ERR"%s (%d) : max20342_change_usbmode mt_usb_host_disconnect\n ",__func__,__LINE__);
		mt_usb_host_disconnect();
	}
	else if(fsa_usb_mode==1) {
		printk(KERN_ERR"%s (%d) : max20342_change_usbmode mt_usb_disconnect\n ",__func__,__LINE__);
		//mt_usb_disconnect();
		config_battery_charger(g_USB_charger_detect_type);
	}
	else {
		mt_usb_connect();
		mdelay(500);
		mt_usb_disconnect();
		mdelay(500);
		config_battery_charger(g_USB_charger_detect_type);
		printk(KERN_ERR"%s (%d) : max20342_change_usbmode nither host or device\n ",__func__,__LINE__);
	}
	fsa_usb_mode = 0;
	max20342_board_data.mode_configuring = 0;
}


static void max20342_change_usbmode(struct work_struct *work)
{
	if (g_USB_role == USB_HOST) {
		usb_host_choose();
	}
	else if (g_USB_role == USB_DEVICE) {
		usb_device_choose();
	}
	else {
		if (g_USB_role == USB_NONE_CONNECT)
			usb_null_choose();
	}
}

/*******************************************************
Function: usb device switch to host / device mode
Noted: 
********************************************************/
static void usb_device_switch(int sw_type)
{
	mutex_lock(&max20342_board_data.input_role_mutex);

	if(max20342_board_data.host_power_on){
		if( g_USB_role != sw_type) {
			g_USB_role = sw_type;
			if(fsa_usb_mode==4)
				schedule_delayed_work(&max20342_board_data.change_usbmode, msecs_to_jiffies(3000));
			else
				schedule_delayed_work(&max20342_board_data.change_usbmode, msecs_to_jiffies(500));
		}
	}
	
	mutex_unlock(&max20342_board_data.input_role_mutex);
	
	return;
}

void max20342_VCCINTOnBAT(void)
{
	unsigned char val = 0;

	if(!max20342_board_data.is_hw_ready) {
		pr_warn("%s skipped since max20342 not exist !\n",__func__);
		return ;
	}

	/* swithc to Low Power DRP mode (LPDRP) */
	if (max20342_read_reg(max20342_board_data.client, MAX20342_COMM_CTRL1, &val)) {
		pr_warn("%s COMM_CTRL1 reading failed !\n",__func__);
		return ;
	}

	val |= COMM_CTRL1_VCCINTOnBAT;
	max20342_write_reg(max20342_board_data.client, MAX20342_COMM_CTRL1, val);

	MAXIAM_DBG("set MAX20342_COMM_CTRL1 0x%x\n", val);
}
EXPORT_SYMBOL_GPL(max20342_VCCINTOnBAT);

void max20342_DisableUSBSWC(void)
{
	unsigned char val = 0;


	if(!max20342_board_data.is_hw_ready) {
		pr_warn("%s skipped since max20342 not exist !\n",__func__);
		return ;
	}

	if (max20342_read_reg(max20342_board_data.client, MAX20342_CC_STATUS2, &val)) {
		//return ;
	}

	if (max20342_read_reg(max20342_board_data.client, MAX20342_COMM_CTRL2, &val)) {
		pr_warn("%s COMM_CTRL2 reading failed !\n",__func__);
		return ;
	}

	val &= ~(COMM_CTRL2_USBSWC_2|COMM_CTRL2_USBSWC_1);
	max20342_write_reg(max20342_board_data.client, MAX20342_COMM_CTRL2, val);

	MAXIAM_DBG("max20342_DisableUSBSWC MAX20342_COMM_CTRL2 0x%x\n", val);
}
EXPORT_SYMBOL_GPL(max20342_DisableUSBSWC);

void max20342_EnableUSBSWC(void)
{
	unsigned char val = 0x00;

	if(!max20342_board_data.is_hw_ready) {
		pr_warn("%s skipped since max20342 not exist !\n",__func__);
		return ;
	}
	if (max20342_read_reg(max20342_board_data.client, MAX20342_COMM_CTRL2, &val)) {
		pr_warn("%s COMM_CTRL2 reading failed !\n",__func__);
		return ;
	}

	if(g_USB_charger_detect_type == CDP_CHARGER)
		val |= COMM_CTRL2_USBSWC_2;
	else
		val |= (COMM_CTRL2_USBSWC_2|COMM_CTRL2_USBSWC_1);
	max20342_write_reg(max20342_board_data.client, MAX20342_COMM_CTRL2, val);

	MAXIAM_DBG("max20342_EnableUSBSWC MAX20342_COMM_CTRL2 0x%x\n", val);
}
EXPORT_SYMBOL_GPL(max20342_EnableUSBSWC);

/*******************************************************
Function: workqueue interrupt handler
Noted: 
********************************************************/
static void max20342_irq_worker(struct work_struct *work)
{
	unsigned char val = 0, w_val = 0;
	unsigned char buf[MAX20342_INT_TOTAL_NUM];
	struct input_dev *input = max20342_board_data.input;

	max20342_read_block_reg(max20342_board_data.client, MAX20342_DEVICE_ID,
				MAX20342_INT_TOTAL_NUM, buf);
	
	if (buf[MAX20342_CC_INT] || !max20342_board_data.is_hw_enabled) {
		if (buf[MAX20342_CC_INT] & (CC_INT_CCVcnStatInt | CC_INT_VCONNOCPInt | CC_INT_DetAbrtInt)) {
			max20342_read_reg(max20342_board_data.client, MAX20342_CC_STATUS1, &val);
			MAXIAM_DBG("MAX20342_CC_STATUS1 0x%x\n", val);
		}
		
		if ((buf[MAX20342_CC_INT] & (CC_INT_CCIStatInt | CC_INT_CCPinStatInt | CC_INT_CCStatInt)) ||
			!max20342_board_data.is_hw_enabled) {
			max20342_read_reg(max20342_board_data.client, MAX20342_CC_STATUS2, &val);
			MAXIAM_DBG("MAX20342_CC_STATUS2 0x%x\n", val);
			switch(val & CC_STATUS2_CCIStat) {
				case 1: printk(KERN_INFO" Charging control : Default current mode\n");
					giChargerType = MAX20342_CHARGER_DEF_CURRENT;
					break;
				case 2: printk(KERN_INFO" Charging control : Medium current mode (1.5A)\n");
					giChargerType = MAX20342_CHARGER_MED_CURRENT;
					break;
				case 3: printk(KERN_INFO" Charging control : High Current mode (3.0A)\n");
					giChargerType = MAX20342_CHARGER_HIGH_CURRENT;
					break;
				default: printk(KERN_INFO" Charging control : Not in sink mode\n");
					giChargerType = MAX20342_CHARGER_NONE;
					break;
			}
			
			switch((val & CC_STATUS2_CCPinStat) >> 2) {
				case 1: printk(KERN_INFO" CC1 Active\n");
					break;
				case 2: printk(KERN_INFO" CC2 Active\n");
					break;
				case 3: printk(KERN_INFO" RFU\n");
					break;
				default: printk(KERN_INFO" No Determination\n");
					break;
			}
			
			
			switch((val & CC_STATUS2_CCStat) >> 4) {
				case 1: printk(KERN_INFO" Attatched Port : Device\n");
					g_USB_plug = 1;
					otg_5v_en(OTG_5V_OFF);
					usb_device_switch(USB_DEVICE);
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				case 2: printk(KERN_INFO" Attatched Port : Host\n");
					g_USB_plug = 1;
					otg_5v_en(OTG_5V_ON);
					usb_device_switch(USB_HOST);
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				case 3: printk(KERN_INFO" Attatched Port : Audio Adapter Accessory\n");
					g_USB_plug = 1;
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				case 4: printk(KERN_INFO" Attatched Port : Debug Host Accessory\n");
					g_USB_plug = 1;
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				case 5: printk(KERN_INFO" Attatched Port : Error\n");
					g_USB_plug = 0;
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				case 6: printk(KERN_INFO" Attatched Port : Disabled\n");
					g_USB_plug = 0;
					otg_5v_en(OTG_5V_OFF);
					usb_device_switch(USB_NONE_CONNECT);
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				case 7: printk(KERN_INFO" Attatched Port : Debug Device Accessory\n");
					g_USB_plug = 1;
					input_report_switch(input,SW_DOCK,1);
					input_sync(input);
					break;
				default: printk(KERN_INFO" Attatched Port : No Connection\n");
					g_USB_plug = 0;
					otg_5v_en(OTG_5V_OFF);
					usb_device_switch(USB_NONE_CONNECT);
					input_report_switch(input,SW_DOCK,0);
					input_sync(input);
					break;
			}
		}
	}
	
	if (buf[MAX20342_BC_INT] || !max20342_board_data.is_hw_enabled) {
		max20342_read_reg(max20342_board_data.client, MAX20342_BC_STATUS, &val);
		MAXIAM_DBG("MAX20342_BC_STATUS 0x%x\n", val);
		
		if(val & BC_STATUS_DCDTmo) {
			printk(KERN_INFO" DCD Detection Timeout\n");
		}
		
		if((val & BC_STATUS_CHgDetRun) >> 1)
			printk(KERN_INFO" Charger detection running or completed\n");
		else
			printk(KERN_INFO" No Charger detection running\n");

		giChargerVendorType = ((val & BC_STATUS_PrChgType) >> 2);
		switch(giChargerVendorType) {
			case 1: printk(KERN_INFO" Charger : Samsung 2A\n");
				break;
			case 2: printk(KERN_INFO" Charger : Apple 0.5A\n");
				break;
			case 3: printk(KERN_INFO" Charger : Apple 1A\n");
				break;
			case 4: printk(KERN_INFO" Charger : Apple 2A\n");
				break;
			case 5: printk(KERN_INFO" Charger : Apple 12W\n");
				break;
			case 6: printk(KERN_INFO" Charger : 3A DCP\n");
				break;
			case 7: printk(KERN_INFO" Charger : Unidentified\n");
				break;
			default: printk(KERN_INFO" Charger : Unknown\n");
				break;
		}

		if (((val & BC_STATUS_ChgType) >> 5) == 2) {
			/* charge type CDP set USBSWC to TDP/TDN position */
			w_val = (COMM_CTRL2_USBSWC_2);
			max20342_write_reg(max20342_board_data.client, MAX20342_COMM_CTRL2, w_val);
		} else {
			/* charge type Other set USBSWC to Follow the automatic hardware setting */
			w_val = (COMM_CTRL2_USBSWC_2 | COMM_CTRL2_USBSWC_1);
			max20342_write_reg(max20342_board_data.client, MAX20342_COMM_CTRL2, w_val);
		}

		switch((val & BC_STATUS_ChgType) >> 5) {
			case 1: printk(KERN_INFO" Charger Type : SDP\n");
				//battery_charge_current(1);
				if(giChargerVendorType)
					g_USB_charger_detect_type = DCP_CHARGER;
				else
					g_USB_charger_detect_type = SDP_CHARGER;
				break;
			case 2: printk(KERN_INFO" Charger Type : CDP\n");
				//battery_charge_current(1);
				if(giChargerVendorType)
					g_USB_charger_detect_type = DCP_CHARGER;
				else
					g_USB_charger_detect_type = CDP_CHARGER;
				break;
			case 3: printk(KERN_INFO" Charger Type : DCP\n");
				//battery_charge_current(2);
				g_USB_charger_detect_type = DCP_CHARGER;
				break;
			default: printk(KERN_INFO" Charger Type : Nothing attached\n");
				//battery_charge_current(0);
				if(giChargerType>0) {
					printk(KERN_INFO" Charger Type : Charging control %d,%d, Let for DCP\n", giChargerType, giChargerVendorType);
					if(giChargerType>1 && giChargerVendorType)
						g_USB_charger_detect_type = DCP_CHARGER;
					else
						g_USB_charger_detect_type = SDP_CHARGER;
				}
				else
					g_USB_charger_detect_type = NO_CHARGER_PLUGGED;
				break;
		}
	}
	
	if (!max20342_board_data.is_hw_enabled)
		max20342_board_data.is_hw_enabled = 1;
	
	return;
}

/*******************************************************
Function: usb interrupt handler
Noted: 
********************************************************/
static irqreturn_t max20342_irq_handler(int irq, void *dev)
{
	schedule_delayed_work(&max20342_board_data.irq_work, msecs_to_jiffies(1000));
	
	return IRQ_HANDLED;
}

/*******************************************************
Function: 0a5_fault interrupt handler
Noted: 
********************************************************/
static irqreturn_t max20342_05a_fault_irq_handler(int irq, void *dev)
{
/*	printk(KERN_ERR"[MAX20342] get 0.5A fault SET OTG_5V 0 %s \n",__FUNCTION__);
	if( gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio ))
		gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 0);
	else
		printk(KERN_INFO"[MAX20342] OTG_5V is not set !!");
*/	
	return IRQ_HANDLED;
}

/*******************************************************
Function: set usb device type
Noted:
********************************************************/
static void set_USB(int role)
{
	unsigned char val = 0;
	
	mutex_lock(&max20342_board_data.input_role_mutex);
	
	if( role == USB_DEVICE)
	{
		max20342_read_reg(max20342_board_data.client, MAX20342_CC_STATUS2, &val);
		if ( 1 == ((val & CC_STATUS2_CCStat) >> 4) || 0 == ((val & CC_STATUS2_CCStat) >> 4) )	// Not Detect device
		{
			if(max20342_board_data.host_power_on){
				printk(KERN_ERR"[MAX20342] SET OTG_5V 0 \n");
				if(gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio ))
					gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 0);
				else
					printk(KERN_INFO"[MAX20342] OTG_5V is not set !!");

				g_USB_role = USB_DEVICE;
				usb_device_choose();  
			}
			else
				printk(KERN_ERR"[MAX20342] HOST_POWER is 0\n");
		}
		else
			printk(KERN_ERR"MAX20342 can't detect host , so can't set DEVICE mode\n");
	}
	else if (role == USB_HOST){
		if ( 2 == ((val & CC_STATUS2_CCStat) >> 4 ) )	// Detect device
		{
			if(max20342_board_data.host_power_on){
				printk(KERN_ERR"[MAX20342] OPEN OTG_5V  1 \n");
				if( gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio ))
					gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 1);
				else
					printk(KERN_INFO"[MAX20342] OTG_5V is not set !!");
				g_USB_role = USB_HOST;
				usb_host_choose(); 
			}
			else
				printk(KERN_ERR"[MAX20342] HOST_POWER is 0\n");
		}
		else
			printk(KERN_ERR"MAX20342 can't detect device , so can't set HOST mode\n");
	}
	
	mutex_unlock(&max20342_board_data.input_role_mutex);
	
	return;
}

/*******************************************************
Function: get usb plug status
Noted: 
********************************************************/
static ssize_t get_usb_plug(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",g_USB_plug);
}

/*******************************************************
Function: get role usb type
Noted: 
********************************************************/
static ssize_t get_role_usb(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",g_USB_role);
}

/*******************************************************
Function: set role usb type
Noted:
********************************************************/
static ssize_t set_role_usb(struct device *dev,
				struct device_attribute *attr,const char *buf, size_t len)
{
	int value ;
	char *pHead = NULL;

	if(g_AutoDetect == 1)
	{
		printk(KERN_ERR" Can't set USB role , USB role is auto switch  now !! \n");
		return len;
	}

	if(( pHead = strstr(buf,"0x")) )
		value = simple_strtol(buf, NULL, 16);
	else
		value = simple_strtoul(buf, NULL, 10);

	if(value == USB_HOST)
		set_USB(value);
	else if(value == USB_DEVICE)
		set_USB(value);
	else
		printk(KERN_ERR"[MAX20342] set set_usb_role failed , value:%d !!!\n",value);
	
	return len;
}

/*******************************************************
Function: get auto detect status
Noted: 
********************************************************/
static ssize_t get_auto_status(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if(g_AutoDetect == 1)
		printk(KERN_ERR" USB role auto switch !! \n");
	else if(g_AutoDetect == 0)
		printk(KERN_ERR" Close USB role auto switch !! \n");
	
	return sprintf(buf, "%d\n",g_AutoDetect);
}

/*******************************************************
Function: set auto detect status
Noted:
********************************************************/
static ssize_t set_auto_status(struct device *dev,
				struct device_attribute *attr,const char *buf, size_t len)
{
	int value ;
	char *pHead = NULL;

	if(( pHead = strstr(buf,"0x") ))
		value = simple_strtol(buf, NULL, 16);
	else
		value = simple_strtoul(buf, NULL, 10);

	if(value == 1){
		g_AutoDetect = 1 ;
		printk(KERN_ERR" USB role auto switch !! \n");
	}
	else if(value == 0){
		g_AutoDetect = 0 ;
		printk(KERN_ERR" Close USB role auto switch !! \n");
	}
	else
		printk(KERN_ERR"[MAX20342] set auto_status failed , value:%d !!!\n",value);
	
	return len;
}

/*******************************************************
Function: get OTG_5V_EN pull status
Noted: max20342 must be read sequentially
********************************************************/
static ssize_t get_OTG_5V_EN(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val = 0;
	
	if(gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio ))
		val = gpio_get_value(max20342_board_data.otg_5v_gpio.gpio);
	else
		printk(KERN_INFO"[P15USB30216C] OTG_5V is not set !!");
	
	return sprintf(buf, "%d\n",val);
}

/*******************************************************
Function: set OTG_5V_EN pull high/low
Noted:
********************************************************/
static ssize_t set_OTG_5V_EN(struct device *dev,
				struct device_attribute *attr,const char *buf, size_t len)
{
	int value;
	char *pHead = NULL;

	if(( pHead = strstr(buf,"0x") ))
		value = simple_strtol(buf, NULL, 16);
	else
		value = simple_strtoul(buf, NULL, 10);

	if( gpio_is_valid (max20342_board_data.otg_5v_gpio.gpio )){
		printk(KERN_ERR"[ERROR] OTG_5V is not set !!");
		return -1;
	}

	if(value == 1)
		gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 1);
	else if(value == 0)
		gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio, 0);
	else
		printk(KERN_ERR"[MAX20342] set OTG_5V_EN failed , value:%d !!!\n",value);
	
	return len;
}

/*******************************************************
Function: Read register from the i2c slave device.

Noted: Max20342 must be read sequentially
********************************************************/
static ssize_t get_int_gpio(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val=-1;
	if(max20342_board_data.irq_gpio.gpio > 0)
	{
		val = gpio_get_value(max20342_board_data.irq_gpio.gpio);
		printk(KERN_ERR"[MAX20342] GPIO Val:%d , gpio:%d \n",val,max20342_board_data.irq_gpio.gpio);
	}

	return sprintf(buf, "\n");
}

static DEVICE_ATTR(USB_PLUG, S_IRUGO , get_usb_plug,NULL);
static DEVICE_ATTR(USB_ROLE, S_IRUGO | S_IWUSR, get_role_usb, set_role_usb);
static DEVICE_ATTR(AUTO_ROLE, S_IRUGO | S_IWUSR, get_auto_status, set_auto_status);
static DEVICE_ATTR(OTG_5V_EN, S_IRUGO | S_IWUSR, get_OTG_5V_EN, set_OTG_5V_EN);
static DEVICE_ATTR(int_gpio, S_IRUGO , get_int_gpio, NULL);

static const struct attribute *sysfs_max20342_attrs[] = {
	&dev_attr_int_gpio.attr,
	&dev_attr_OTG_5V_EN.attr,
	&dev_attr_USB_ROLE.attr,
	&dev_attr_USB_PLUG.attr,
	&dev_attr_AUTO_ROLE.attr,
	NULL,
};

static int max20342_hw_init(struct i2c_client *client)
{
	unsigned char val = 0;
	
	if (max20342_read_reg(client, MAX20342_DEVICE_ID, &val))
		return -ENODEV;
		
	dev_info(&client->dev, "detected revision %d\n", val);
	
	if (max20342_read_reg(client, MAX20342_CC_MASK, &val))
		return -ENODEV;
	
	/* set */
	val = (CC_MASK_CCPinStatM | CC_MASK_CCStatM | CC_MASK_CCIStatM);
	max20342_write_reg(client, MAX20342_CC_MASK, val);
	
	MAXIAM_DBG("set MAX20342_CC_MASK 0x%x\n", val);
	
	if (max20342_read_reg(client, MAX20342_CC_CTRL_1, &val))
		return -ENODEV;
		
	/* enable detect mode */
	val = (CC_CTRL2_AUTO_STATE_EN | CC_CTRL2_DBG_SRC_DET_EN | CC_CTRL2_AUDIO_DET_EN | 
            CC_CTRL2_SRC_DET_EN | CC_CTRL2_SNK_DET_EN);
	max20342_write_reg(client, MAX20342_CC_CTRL_1, val);
	
	MAXIAM_DBG("set MAX20342_CC_CTRL_1 0x%x\n", val);
	
	/* swithc to Low Power DRP mode (LPDRP) */
	if (max20342_read_reg(client, MAX20342_COMM_CTRL1, &val))
		return -ENODEV;
		
	val = (COMM_CTRL1_INTEn | COMM_CTRL1_FactAuto | COMM_CTRL1_USBAuto | COMM_CTRL1_LPDRP);
	max20342_write_reg(client, MAX20342_COMM_CTRL1, val);
	
	MAXIAM_DBG("set MAX20342_COMM_CTRL1 0x%x\n", val);

	if (max20342_read_reg(client, MAX20342_COMM_CTRL2, &val))
		return -ENODEV;

	val = (COMM_CTRL2_USBSWC_2 | COMM_CTRL2_USBSWC_1);
	max20342_write_reg(client, MAX20342_COMM_CTRL2, val);

	MAXIAM_DBG("set MAX20342_COMM_CTRL2 0x%x\n", val);

	return 0;
}

/*******************************************************
Function: Parse dts

Noted:
********************************************************/
static int max20342_parse_dt(struct device *dev)
{
	int ret = 0;
	
	struct device_node; //*usbc_np = NULL;
	
	of_property_read_u32_array(dev->of_node, "host_power", &max20342_board_data.host_power_on, 1);
	if (ret) {
		dev_err(dev, "get host_power failed, %d\n", ret);
		ret = -1;
	} else 
		dev_err(dev, "host_power = %d\n", max20342_board_data.host_power_on);
	
	max20342_board_data.irq_gpio.gpio = of_get_named_gpio(dev->of_node, "int_port", 0);
	if (!gpio_is_valid(max20342_board_data.irq_gpio.gpio)) {
		dev_err(dev, "irq_gpio is invalid.\n");
		ret = -1;
	}

		dev_err(dev, "irq_gpio.gpio = %d\n", max20342_board_data.irq_gpio.gpio);
	
	max20342_board_data.otg_5v_gpio.gpio = of_get_named_gpio(dev->of_node, "otg_5v_en", 0);
	if (!gpio_is_valid(max20342_board_data.otg_5v_gpio.gpio)) {
		dev_err(dev, "otg_5v_en is invalid.\n");
		ret = -1;
	}
		dev_err(dev, "otg_5v_gpio.gpio = %d\n", max20342_board_data.otg_5v_gpio.gpio);
	
	max20342_board_data._0a5_fault.gpio = of_get_named_gpio(dev->of_node, "0a5_fault", 0);
	if (!gpio_is_valid(max20342_board_data._0a5_fault.gpio))
		dev_err(dev, "0a5_fault is invalid.\n");
	
	/* get USB_PORT_TYPE */
	//usbc_np = of_find_node_by_type(NULL, SET_USB0);	
	//usbc_np = of_find_node_by_type(NULL, "usbc0");	
	//if (!usbc_np)
	//	dev_err(dev, "ERROR! get usbc0 failed\n");
	/* usbc port type */
	//of_property_read_u32(usbc_np, KEY_USB_PORT_TYPE, &g_usb_cfg.port.port_type);
	//of_property_read_u32(usbc_np, "usb_port_type", &g_usb_cfg.port.port_type);
	
	return ret;
}

/*******************************************************
Function: Probe

Noted:
********************************************************/
static int max20342_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	int err;
	
	MAXIAM_DBG("probe:name = %s,flag =%d,addr = %d,adapter = %d\n",client->name,  
	client->flags,client->addr,client->adapter->nr );
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&client->dev, "no algorithm associated to the i2c bus\n");
		return -ENODEV;
	}
	max20342_board_data.is_hw_ready = 0;
	max20342_board_data.client = client ;

	max20342_board_data.input = input_allocate_device();
	if (max20342_board_data.input == NULL) {
		return -ENOMEM;
	}
	max20342_board_data.mode_configuring = 0;
	strlcpy(client->name, MAX20342_DEV_NAME, I2C_NAME_SIZE);
	max20342_board_data.input->name = MAX20342_DEV_NAME;
	max20342_board_data.input->id.bustype = BUS_I2C;
	max20342_board_data.input->id.vendor = 0x0416;
	max20342_board_data.input->id.product = 0x1001;
	max20342_board_data.input->id.version = 10427;
    
	__set_bit(EV_SW, max20342_board_data.input->evbit);
	__set_bit(SW_DOCK, max20342_board_data.input->swbit);
	__set_bit(SW_TABLET_MODE, max20342_board_data.input->swbit);
	
	ret = input_register_device(max20342_board_data.input);
	if (ret < 0) {
		dev_err(&client->dev, "input_register_device failed\n");
		return ret;
	}
	ret = sysfs_create_files(&max20342_board_data.input->dev.kobj, sysfs_max20342_attrs);
	if (ret) {
		dev_err(&client->dev, "Can't create device file!\n");
		return -ENODEV;
	}
	
	mutex_init(&max20342_board_data.input_role_mutex);
	mutex_lock(&max20342_board_data.input_role_mutex);
	
	max20342_board_data.is_hw_ready = 0;
	max20342_board_data.is_hw_enabled = 0;
	if (client->dev.of_node) {
		ret = max20342_parse_dt(&client->dev);
		if (ret) {
			dev_err(&client->dev, "Unable to parse dt data err=%d\n", ret);
			ret = -EINVAL;
			goto free_probe;
		}
	}
	
	ret = max20342_hw_init(client);
	if (ret)
		goto free_probe;
		
	msleep(100);
	
	INIT_DELAYED_WORK(&max20342_board_data.irq_work, max20342_irq_worker);
	
	/*if(gpio_is_valid(max20342_board_data.irq_gpio.gpio))
	{

		max20342_board_data.irq = gpio_to_irq(max20342_board_data.irq_gpio.gpio);
		printk(KERN_ERR"[%s_%d] irq:%d \n",__FUNCTION__,__LINE__,max20342_board_data.irq);
		
		if (max20342_board_data.irq) {
			ret = request_threaded_irq(max20342_board_data.irq, NULL, max20342_irq_handler, IRQF_TRIGGER_LOW | IRQF_ONESHOT, 
												MAX20342_DEV_NAME, &max20342_board_data);
		}
	}*/

	/* gpio max20342_otg_5v */
	if (gpio_is_valid(max20342_board_data.otg_5v_gpio.gpio)) {
		err = gpio_request(max20342_board_data.otg_5v_gpio.gpio, "max20342_otg_5v");
		if (err) {
			printk(KERN_ERR
				"Unable to request gpio max20342_OTG_5V %d\n",
				max20342_board_data.otg_5v_gpio.gpio);
			goto free_probe;
		}

		err = gpio_direction_output(max20342_board_data.otg_5v_gpio.gpio,0);
		if (err) {
			printk(KERN_ERR
				"Unable to set direction formax20342_OTG_5V %d\n",
				max20342_board_data.otg_5v_gpio.gpio);
			goto free_probe;
		}
		
	}
	/* gpio max20342_otg_5v */


	/*max20342_cd_int gpio to irq */
	if (gpio_is_valid(max20342_board_data.irq_gpio.gpio)) {
		err = gpio_request(max20342_board_data.irq_gpio.gpio, "max20342_cd_int");
		if (err) {
			printk(KERN_ERR
				"Unable to request interrupt max20342_CD_INT %d\n",
				max20342_board_data.irq_gpio.gpio);
			goto free_probe;
		}

		err = gpio_direction_input(max20342_board_data.irq_gpio.gpio);
		if (err) {
			printk(KERN_ERR
				"Unable to set direction for max20342_CD_INT %d\n",
				max20342_board_data.irq_gpio.gpio);
			goto free_probe;
		}
		max20342_board_data.irq = gpio_to_irq(max20342_board_data.irq_gpio.gpio);

		if (max20342_board_data.irq) {
			ret = request_threaded_irq(max20342_board_data.irq, NULL, max20342_irq_handler, IRQF_TRIGGER_FALLING|IRQF_ONESHOT, 
												MAX20342_DEV_NAME, &max20342_board_data);
		}
	}
	/* gpio to irq */
	
	/*if(gpio_is_valid(max20342_board_data._0a5_fault.gpio))
	{
		max20342_board_data._0a5_fault_irq = gpio_to_irq(max20342_board_data._0a5_fault.gpio);
		printk(KERN_ERR"[%s_%d] 0a5_dfault irq:%d \n",__FUNCTION__,__LINE__,max20342_board_data._0a5_fault_irq);
		
		if (max20342_board_data._0a5_fault_irq) {
			ret = request_threaded_irq(max20342_board_data._0a5_fault_irq, NULL, max20342_05a_fault_irq_handler, IRQF_TRIGGER_LOW | \
												IRQF_ONESHOT,MAX20342_DEV_NAME, &max20342_board_data);
		}
	}*/

	/* max20342_0a5_fault gpio to irq */
	if (gpio_is_valid(max20342_board_data._0a5_fault.gpio)) {
		err = gpio_request(max20342_board_data._0a5_fault.gpio, "max20342_0a5_fault");
		if (err) {
			printk(KERN_ERR
				"Unable to request interrupt max20342_0a5_fault %d\n",
				max20342_board_data._0a5_fault.gpio);
			goto free_probe;
		}

		err = gpio_direction_input(max20342_board_data._0a5_fault.gpio);
		if (err) {
			printk(KERN_ERR
				"Unable to set direction for max20342_0a5_fault %d\n",
				max20342_board_data._0a5_fault.gpio);
			goto free_probe;
		}
		max20342_board_data._0a5_fault_irq = gpio_to_irq(max20342_board_data._0a5_fault.gpio);

		if (max20342_board_data._0a5_fault_irq) {
			ret = request_threaded_irq(max20342_board_data._0a5_fault_irq, NULL, max20342_05a_fault_irq_handler, IRQF_TRIGGER_LOW | IRQF_ONESHOT, 
												MAX20342_DEV_NAME, &max20342_board_data);
		}
	}
	/* gpio to irq */
	
	max20342_board_data.is_hw_ready = 1;
	ntx_charger_register_cc_detector(max20342_get_charger_type);

	
	schedule_delayed_work(&max20342_board_data.irq_work, msecs_to_jiffies(5000));
	
	INIT_DELAYED_WORK(&max20342_board_data.change_usbmode, max20342_change_usbmode);

free_probe:
	mutex_unlock(&max20342_board_data.input_role_mutex);
	
	return ret;
}

/*******************************************************
Function: remove

Noted:
********************************************************/
static int max20342_remove(struct i2c_client *client)
{
	int ret = 0;
	
	MAXIAM_DBG("shutdown:name = %s,flag =%d,addr = %d,adapter = %d\n",client->name,  
	client->flags,client->addr,client->adapter->nr );
	max20342_board_data.is_hw_ready = 0;
	return ret;
}


/*******************************************************
Function: shutdown

Noted:
********************************************************/
static void max20342_shutdown(struct i2c_client *client)
{
	unsigned char val = 0;
	
	//disable_irq(max20342_board_data.irq);
	//disable_irq(max20342_board_data._0a5_fault_irq);
	
	/* switch to Low power Shutdown mode */
	//if (max20342_read_reg(client, MAX20342_COMM_CTRL1, &val))
	//	return -ENODEV;	
		
	//val = (COMM_CTRL1_INTEn | COMM_CTRL1_FactAuto | COMM_CTRL1_USBAuto | COMM_CTRL1_ShdnMode);
	//max20342_write_reg(client, MAX20342_COMM_CTRL1, val);
	
	MAXIAM_DBG("set MAX20342_COMM_CTRL1 0x%x\n", val);
	
	MAXIAM_DBG("shutdown:name = %s,flag =%d,addr = %d,adapter = %d\n",client->name,  
	client->flags,client->addr,client->adapter->nr );
	
	return;
}

static const struct i2c_device_id max20342_id[] = {
	{MAX20342_DEV_NAME, 0},
	{},
};

static const struct of_device_id maxiam_dt_match[] = {
	{ .compatible = "maxiam,max20342" , },
	{},
};
MODULE_DEVICE_TABLE(of,maxiam_dt_match);


static int __maybe_unused max20342_suspend(struct device *dev)
{
	pr_debug("%s()\n",__func__);
	if (delayed_work_pending(&max20342_board_data.irq_work)) {
		printk(KERN_ERR"%s skipped ! irq_work is pending !\n",__func__);
		return -1;
	}
	if (delayed_work_pending(&max20342_board_data.change_usbmode)) {
		printk(KERN_ERR"%s skipped ! change_usbmode is pending !\n",__func__);
		return -2;
	}
	if(max20342_board_data.mode_configuring) {
		printk(KERN_ERR"%s skipped ! mode configuring ...\n",__func__);
		return -3;
	}
	return 0;
}

static int __maybe_unused max20342_resume(struct device *dev)
{
	pr_debug("%s()\n",__func__);
	return 0;
}

static const struct dev_pm_ops max20342_pm_ops = {
	.suspend = max20342_suspend,
	.resume = max20342_resume,
};

static struct i2c_driver max20342_driver = {
	.class = I2C_CLASS_HWMON ,
	.probe    = max20342_probe,
	.remove   = max20342_remove,
	.shutdown   = max20342_shutdown,
	.id_table = max20342_id,
	.driver = {
			.owner = THIS_MODULE,
			.name  = MAX20342_DEV_NAME,
			.of_match_table = maxiam_dt_match,
			.pm = &max20342_pm_ops,
		  },
};

static int __init max20342_init(void)
{
	/* register driver */
	int ret;

	memset(&max20342_board_data,sizeof(max20342_board_data),0);	
	ret = i2c_add_driver(&max20342_driver);
	if (ret < 0) {
		printk(KERN_INFO "add max20342 i2c driver failed\n");
		return -ENODEV;
	}
	
	return ret;
}

static void __exit max20342_exit(void)
{
	i2c_del_driver(&max20342_driver);
	
	return;
}

module_init(max20342_init);  
module_exit(max20342_exit); 

MODULE_AUTHOR("Netronix");  
MODULE_DESCRIPTION("max20342 driver");  
MODULE_LICENSE("GPL");  

