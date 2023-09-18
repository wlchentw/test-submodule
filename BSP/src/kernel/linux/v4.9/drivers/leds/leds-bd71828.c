// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2019 ROHM Semiconductors

#include <linux/device.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/mfd/rohm-bd71828.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include "leds.h"

static struct bd71828_leds * gBd71828Leds = NULL;
static int suspend_status = 0;

#define BD71828_LED_TO_DATA(l) ((l)->id == ID_GREEN_LED ? \
	container_of((l), struct bd71828_leds, green) : \
	container_of((l), struct bd71828_leds, amber))

enum {
	ID_GREEN_LED,
	ID_AMBER_LED,
	ID_NMBR_OF,
};

struct bd71828_led {
	int id;
	struct led_classdev l;
	enum led_brightness set_brightness;
	struct work_struct led_work;
	struct workqueue_struct *led_wq;
	u8 force_mask;
};

struct bd71828_leds {
	struct rohm_regmap_dev *bd71828;
	struct bd71828_led green;
	struct bd71828_led amber;
};



static void bd71828_led_brightness_set(
	struct led_classdev *led_cdev, enum led_brightness value)
{
	struct bd71828_led *l = container_of(led_cdev, struct bd71828_led, l);
	struct bd71828_leds *data;
	unsigned int val = BD71828_LED_OFF;

	pr_debug("%s() \"%s\" b=%d,blink_b=%d,flags=0x%x\n",__func__,
			l->l.name,value,l->l.blink_brightness,l->l.flags);

	l->l.flags |= LED_BLINK_BRIGHTNESS_CHANGE;
	l->l.blink_brightness = 1;

	data = BD71828_LED_TO_DATA(l);
	if(value == 2) {
		regmap_update_bits(data->bd71828->regmap, BD71828_REG_LED_CTRL, (BD71828_MASK_LED_AMBER|BD71828_MASK_LED_GREEN), 0x0);
		data->amber.l.brightness = 2;
		data->green.l.brightness = 2;
	}
	else {
		if (value != LED_OFF)
			val = BD71828_LED_ON;
		regmap_update_bits(data->bd71828->regmap, BD71828_REG_LED_CTRL,
					l->force_mask, val);
		if(l->id == ID_GREEN_LED) {
			if(value == LED_OFF) {
				regmap_update_bits(data->bd71828->regmap, BD71828_REG_LED_CTRL, BD71828_MASK_LED_AMBER, BD71828_MASK_LED_AMBER);
				data->amber.l.brightness = 1;
			}
		}
		else {
			if(value == LED_OFF) {
				regmap_update_bits(data->bd71828->regmap, BD71828_REG_LED_CTRL, BD71828_MASK_LED_GREEN, BD71828_MASK_LED_GREEN);
				data->green.l.brightness = 1;
			}
		}
	}
}

static void led_work_func(struct work_struct *work)
{
	struct bd71828_led *l = container_of(work, struct bd71828_led, led_work);
	//printk(KERN_DEBUG"%s() id=%d,brightness=%d\n",__func__,l->id,l->set_brightness);
	
	if (suspend_status) {
		queue_work(l->led_wq, &l->led_work);
		return;
	}

	bd71828_led_brightness_set(&l->l,l->set_brightness);
}

static void bd71828_led_brightness_set_work(
	struct led_classdev *led_cdev, enum led_brightness value)
{
	struct bd71828_led *l = container_of(led_cdev, struct bd71828_led, l);
	//printk(KERN_DEBUG"%s() id=%d,value=%d\n",__func__,l->id,value);
	l->set_brightness = value;
	queue_work(l->led_wq, &l->led_work);
}

int bd71828_led_suspend(struct platform_device *pdev, pm_message_t state)
{

	pr_debug("%s()\n",__func__);

	if(!gBd71828Leds) {
		printk(KERN_WARNING"%s() skipped ! probe failed !?\n",__func__);
		return 0;
	}

	flush_work(&gBd71828Leds->green.led_work);
	flush_work(&gBd71828Leds->amber.led_work);

	if(gBd71828Leds->green.l.trigger) {
		pr_debug("%s() green trigger is \"%s\"\n",__func__,gBd71828Leds->green.l.trigger->name);
		if(!strcmp(gBd71828Leds->green.l.trigger->name,"timer")) {
			bd71828_led_brightness_set(&gBd71828Leds->green.l,0);
		}
	}
	
	suspend_status = 1;

	return 0;
}

int bd71828_led_resume(struct platform_device *pdev)
{
	pr_debug("%s()\n",__func__);

	if(!gBd71828Leds) {
		printk(KERN_WARNING"%s() skipped ! probe failed !?\n",__func__);
		return 0;
	}
	
	suspend_status = 0;
	return 0;
}


static void bd71828_led_shutdown(struct platform_device *pdev)
{
	if(!gBd71828Leds) {
		printk(KERN_WARNING"%s() skipped ! probe failed !?\n",__func__);
		return ;
	}

	led_stop_software_blink(&gBd71828Leds->green.l);
	led_stop_software_blink(&gBd71828Leds->amber.l);
	
	cancel_work_sync(&gBd71828Leds->green.led_work);
	cancel_work_sync(&gBd71828Leds->amber.led_work);

	bd71828_led_brightness_set(&gBd71828Leds->green.l,0);

	//regmap_update_bits(gBd71828Leds->bd71828->regmap, BD71828_REG_LED_CTRL, BD71828_MASK_LED_GREEN, 0);
	//regmap_update_bits(gBd71828Leds->bd71828->regmap, BD71828_REG_LED_CTRL, BD71828_MASK_LED_AMBER, BD71828_MASK_LED_AMBER);
	//msleep(5);
}

static int bd71828_led_probe(struct platform_device *pdev)
{
	struct rohm_regmap_dev *bd71828;
	struct bd71828_leds *l;
	struct bd71828_led *g, *a;
	static const char *GNAME = "bd71828-green-led";
	static const char *ANAME = "bd71828-amber-led";
	int ret = 0;

	pr_info("bd71828 LED driver probed\n");

	bd71828 = dev_get_drvdata(pdev->dev.parent);
	l = devm_kzalloc(&pdev->dev, sizeof(*l), GFP_KERNEL);
	if (!l)
		return -ENOMEM;
	l->bd71828 = bd71828;

	a = &l->amber;
	g = &l->green;

	a->led_wq = create_singlethread_workqueue("aled_wq");
	INIT_WORK(&a->led_work, led_work_func);
	a->id = ID_AMBER_LED;
	a->force_mask = BD71828_MASK_LED_AMBER;
	a->l.flags = LED_BLINK_BRIGHTNESS_CHANGE;// |LED_CORE_SUSPENDRESUME
	a->l.blink_brightness = 1;
	a->l.name = ANAME;
	a->l.brightness = 2;// controlled by rohm .
	//a->l.brightness_set = bd71828_led_brightness_set;
	a->l.brightness_set = bd71828_led_brightness_set_work;
	a->l.max_brightness = 2;

	g->led_wq = create_singlethread_workqueue("gled_wq");
	INIT_WORK(&g->led_work, led_work_func);
	g->id = ID_GREEN_LED;
	g->force_mask = BD71828_MASK_LED_GREEN;
	g->l.flags = LED_BLINK_BRIGHTNESS_CHANGE;// |LED_CORE_SUSPENDRESUME
	g->l.blink_brightness = 1;
	g->l.name = GNAME;
	g->l.brightness = 2;// controlled by rohm . 
	//g->l.brightness_set = bd71828_led_brightness_set;
	g->l.brightness_set = bd71828_led_brightness_set_work;
	g->l.max_brightness = 2;
	g->l.default_trigger = "timer";

	ret |= devm_led_classdev_register(&pdev->dev, &g->l);

	ret |= devm_led_classdev_register(&pdev->dev, &a->l);

	if (0==ret) {
		gBd71828Leds = l ;
	}

	return ret;
}

static struct platform_driver bd71828_led_driver = {
	.driver = {
		.name  = "bd71828-led",
	},
	.probe  = bd71828_led_probe,
	.shutdown	= bd71828_led_shutdown,
	.suspend = bd71828_led_suspend,
	.resume = bd71828_led_resume,
};

module_platform_driver(bd71828_led_driver);

MODULE_AUTHOR("Matti Vaittinen <matti.vaittinen@fi.rohmeurope.com>");
MODULE_DESCRIPTION("ROHM BD71828 LED driver");
MODULE_LICENSE("GPL");
