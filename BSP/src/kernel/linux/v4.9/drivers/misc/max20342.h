

#ifndef __MAX20342_H__
#define __MAX20342_H__

#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/fs.h>  
#include <linux/slab.h>  
#include <linux/init.h>  
#include <linux/list.h>  
#include <linux/i2c.h>  
#include <linux/i2c-dev.h>  
#include <linux/input.h>
#include <linux/jiffies.h>  
#include <asm/uaccess.h>  
#include <linux/delay.h>  
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
//#include <linux/sunxi-gpio.h>
#include <linux/input/mt.h>
#include <linux/mutex.h>
//#include  "../../usb/sunxi_usb/manager/usb_msg_center.h"
//#include  "../../usb/sunxi_usb/manager/usbc_platform.h"
//#include  "../../usb/sunxi_usb/include/sunxi_usb_board.h"

extern void mt_usb_connect(void);
extern void mt_usb_disconnect(void);
extern void mt_usb_host_connect(void);
extern void mt_usb_host_disconnect(void);

extern void battery_charge_current(int mode);
extern void config_battery_charger(int mode);

#define DEBUG
#ifdef DEBUG  
#define MAXIAM_DBG(x...) printk(x)  
#else   
#define MAXIAM_DBG(x...) (void)(0)  
#endif 

#define MAX20342_DEV_NAME       "max20342"

#define MAX20342_INT_TOTAL_NUM  7
#define MAX20342_DEVICE_ID      0x00

/* Interrupt Register */
#define MAX20342_COMMON_INT     0x01
#define MAX20342_CC_INT         0x02
#define CC_INT_DetAbrtInt       BIT(0) /* CC int */
#define CC_INT_VCONNOCPInt      BIT(1) /* CC int */
#define CC_INT_CCVcnStatInt     BIT(2) /* CC int */
#define CC_INT_CCIStatInt       BIT(3) /* CC int */
#define CC_INT_CCPinStatInt     BIT(4) /* CC int */
#define CC_INT_CCStatInt        BIT(5) /* CC int */
#define MAX20342_BC_INT         0x03
#define BC_INT_DCDTmoInt        BIT(0) /* BC int */
#define BC_INT_CHgDetRunFInt    BIT(1) /* BC int */
#define BC_INT_CHgDetRunRInt    BIT(2) /* BC int */
#define BC_INT_PrChgTypInt      BIT(3) /* BC int */
#define BC_INT_ChgTypInt        BIT(4) /* BC int */
#define MAX20342_OVP_INT        0x04
#define MAX20342_RES_INT1       0x05
#define MAX20342_RES_INT2       0x06

/* Status Register */
#define MAX20342_COMMON_STATUS  0x07
#define MAX20342_CC_STATUS1     0x08
#define MAX20342_CC_STATUS2     0x09
#define CC_STATUS2_CCIStat      0x03   /* CC Status 2 */
#define CC_STATUS2_CCPinStat    0x0C   /* CC Status 2 */
#define CC_STATUS2_CCStat       0x70   /* CC Status 2 */
#define MAX20342_BC_STATUS      0x0A
#define BC_STATUS_DCDTmo        BIT(0) /* BC Status */
#define BC_STATUS_CHgDetRun     BIT(1) /* BC Status */
#define BC_STATUS_PrChgType     0x1C   /* BC Status */
#define BC_STATUS_ChgType       0x60   /* BC Status */
#define MAX20342_OVP_STATUS     0x0B

/* Interrupt Mask Register */
#define MAX20342_COMMON_MASK    0x0C
#define MAX20342_CC_MASK        0x0D
#define CC_MASK_DetAbrtM        BIT(0) /* CC Mask */
#define CC_MASK_VCONNOCPM       BIT(1) /* CC Mask */
#define CC_MASK_CCVcnStatM      BIT(2) /* CC Mask */
#define CC_MASK_CCIStatM        BIT(3) /* CC Mask */
#define CC_MASK_CCPinStatM      BIT(4) /* CC Mask */
#define CC_MASK_CCStatM         BIT(5) /* CC Mask */
#define MAX20342_BC_MASK        0x0E
#define MAX20342_OVP_MASK       0x0F
#define MAX20342_RES_MASK1      0x10
#define MAX20342_RES_MASK2      0x11

/* User Commom Regoster */
#define MAX20342_COMM_CTRL1     0x15
#define COMM_CTRL1_ShdnMode     BIT(0) /* COMM Ctrl 1 */
#define COMM_CTRL1_LPUFP        BIT(1) /* COMM Ctrl 1 */
#define COMM_CTRL1_LPDRP        BIT(2) /* COMM Ctrl 1 */
#define COMM_CTRL1_VCCINTOnBAT  BIT(3) /* COMM Ctrl 1 */
#define COMM_CTRL1_AudioCPEn    BIT(4) /* COMM Ctrl 1 */
#define COMM_CTRL1_USBAuto      BIT(5) /* COMM Ctrl 1 */
#define COMM_CTRL1_FactAuto     BIT(6) /* COMM Ctrl 1 */
#define COMM_CTRL1_INTEn        BIT(7) /* COMM Ctrl 1 */
#define MAX20342_COMM_CTRL2     0x16
#define COMM_CTRL2_CEFrc        BIT(0) /* COMM Ctrl 2 */
#define COMM_CTRL2_CE           BIT(1) /* COMM Ctrl 2 */
#define COMM_CTRL2_DBFrc        BIT(3) /* COMM Ctrl 2 */
#define COMM_CTRL2_DB           BIT(4) /* COMM Ctrl 2 */
#define COMM_CTRL2_NotUSBCmpl   BIT(5) /* COMM Ctrl 2 */
#define COMM_CTRL2_USBSWC_1     BIT(6) /* COMM Ctrl 2 */
#define COMM_CTRL2_USBSWC_2     BIT(7) /* COMM Ctrl 2 */

/* CC Control Register */
#define MAX20342_CC_CTRL_0      0x20
#define MAX20342_CC_CTRL_1      0x21
#define MAX20342_CC_CTRL_2      0x22
#define CC_CTRL2_SNK_DET_EN     BIT(0) /* CC Ctrl 2 */
#define CC_CTRL2_SRC_DET_EN     BIT(1) /* CC Ctrl 2 */
#define CC_CTRL2_AUDIO_DET_EN   BIT(2) /* CC Ctrl 2 */
#define CC_CTRL2_DBG_SNK_DET_EN BIT(3) /* CC Ctrl 2 */
#define CC_CTRL2_DBG_SRC_DET_EN BIT(4) /* CC Ctrl 2 */
#define CC_CTRL2_TRY_SNK_EN     BIT(6) /* CC Ctrl 2 */
#define CC_CTRL2_AUTO_STATE_EN  BIT(7) /* CC Ctrl 2 */
#define MAX20342_CC_CTRL_3      0x23
#define MAX20342_CC_CTRL_4      0x24
#define MAX20342_CC_CTRL_5      0x25
#define MAX20342_CC_CTRL_6      0x26
#define MAX20342_VCONN_ILIM     0x28

/* BC Control Register */
#define MAX20342_BC_CTRL_0     0x2A
#define MAX20342_BC_CTRL_1     0x2B

#define MAX20342_NOT_EXIST				(-1)
#define MAX20342_CHARGER_NONE			0
#define MAX20342_CHARGER_DEF_CURRENT	1
#define MAX20342_CHARGER_MED_CURRENT	2
#define MAX20342_CHARGER_HIGH_CURRENT	3

enum USB_ROLE
{
	USB_DEFAULT	= 0,
	USB_DEVICE,
	USB_HOST,
	USB_NONE_CONNECT,
};

enum OTG_5V_STATUS
{
	OTG_5V_OFF = 0,
	OTG_5V_ON,    
};

/* USB config info */
enum usb_gpio_group_type {
	GPIO_GROUP_TYPE_PIO = 0,
	GPIO_GROUP_TYPE_POWER,
};

/* 0: device only; 1: host only; 2: otg */
enum usb_port_type {
	USB_PORT_TYPE_DEVICE = 0,
	USB_PORT_TYPE_HOST,
	USB_PORT_TYPE_OTG,
};

/* 0: dp/dm detect, 1: vbus/id detect 2: vbus/pmu detect*/
enum usb_detect_type {
	USB_DETECT_TYPE_DP_DM = 0,
	USB_DETECT_TYPE_VBUS_ID,
	USB_DETECT_TYPE_VBUS_PMU,
};

/* 0: thread scan mode; 1: gpio interrupt mode */
enum usb_detect_mode {
	USB_DETECT_MODE_THREAD = 0,
	USB_DETECT_MODE_INTR,
};

enum usb_det_vbus_type {
	USB_DET_VBUS_TYPE_NULL = 0,
	USB_DET_VBUS_TYPE_GPIO,
	USB_DET_VBUS_TYPE_AXP,
};

enum usb_id_type {
	USB_ID_TYPE_NULL = 0,
	USB_ID_TYPE_GPIO,
	USB_ID_TYPE_AXP,
};

/* pio info */
typedef struct usb_gpio {
	/* pio valid, 1 - valid, 0 - invalid */
	__u32 valid;
	__u32 gpio;
} usb_gpio_t;

typedef struct usb_port_info {
	__u32 enable;				/* port valid */

	__u32 port_no;				/* usb port number */
	enum usb_port_type port_type;		/* usb port type */
	enum usb_detect_type detect_type;	/* usb detect type */
	enum usb_detect_mode detect_mode;	/* usb detect mode */
	enum usb_det_vbus_type det_vbus_type;
	enum usb_id_type id_type;
	const char *det_vbus_name;
	const char *id_name;
	usb_gpio_t id;				/* usb id pin info */
	usb_gpio_t det_vbus;			/* usb vbus pin info */
	// usb_gpio_t drv_vbus;			/* usb drv_vbus pin info */
	// usb_gpio_t restrict_gpio_set;		/* usb drv_vbus pin info */
	__u32 usb_restrict_flag;		/* usb port number(?) */
	__u32 voltage;				/* usb port number(?) */
	__u32 capacity;				/* usb port number(?) */

	int id_irq_num;				/* id gpio irq num */

#if defined(CONFIG_DUAL_ROLE_USB_INTF)
	struct dual_role_phy_instance *dual_role;
	struct dual_role_phy_desc dr_desc;
#endif
#if defined(CONFIG_TYPEC)
	struct typec_port *typec_port;
	struct typec_partner *partner;
	struct typec_capability typec_caps;
	bool connected;
#endif
#if defined(CONFIG_POWER_SUPPLY)
	struct power_supply *pmu_psy;            /* pmu type*/
	union  power_supply_propval *pmu;
#endif
} usb_port_info_t;

typedef struct usb_cfg {
	u32 usb_global_enable;
	u32 usbc_num;
	struct platform_device *pdev;

	struct usb_port_info port;
} usb_cfg_t;



struct max20342_data {
	int intr_gpio;
	int use_irq;
	int host_power_on;             // 1: enable Host power on (OTG_5V) , 0: disable Host power on
	int irq;
	int _0a5_fault_irq;
	struct gpio_config irq_gpio;
	struct gpio_config otg_5v_gpio;
	struct gpio_config _0a5_fault;
	struct gpio_config det_db;
	struct gpio_config det_ce;
	struct i2c_client *client;
	struct input_dev *input;
	int is_hw_ready;
	int is_hw_enabled;
	struct mutex input_role_mutex;
	struct delayed_work	irq_work;
	struct delayed_work	change_usbmode;
	int mode_configuring ;
};

#endif
