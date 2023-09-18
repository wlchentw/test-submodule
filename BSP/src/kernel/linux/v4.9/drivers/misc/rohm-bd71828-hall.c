// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2019 ROHM Semiconductors

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/rohm-bd71828.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/input.h>
#include <linux/slab.h>

#define MAGIC_GRACE_TIME	3000000000ULL
#define CLOSE_DELAY	250000000ULL

struct bd71828_hall {
	struct regmap *regmap;
	struct device *dev;
	unsigned int open_state;
	bool enabled;
	struct delayed_work delayed_event;
	struct input_dev *input;
};

static void hall_send_open_event(struct device *dev,
					int open, struct bd71828_hall *hall)
{
	//char *envp[] = {"HALLSENSOR=opened", NULL};
	struct input_dev *input = hall->input;

	dev_info(dev, "KERNEL: send event: open\n");
	//kobject_uevent_env(&dev->kobj, KOBJ_ONLINE, envp);
	input_report_switch(input, SW_LID,
		open ? 0 : 1);
	input_sync(input);
}

static void hall_send_close_event(struct device *dev,
					int open, struct bd71828_hall *hall)
{
	//char *envp[] = {"HALLSENSOR=opened", NULL};
	struct input_dev *input = hall->input;

	dev_info(dev, "KERNEL: send event: close\n");
	input_report_switch(input, SW_LID,
		open ? 0 : 1);
	input_sync(input);
	//kobject_uevent_env(&dev->kobj, KOBJ_OFFLINE, envp);
}

static int lid_is_open(struct bd71828_hall *hall)
{
	unsigned int reg;
	int ret;

	ret = regmap_read(hall->regmap, BD71828_REG_IO_STAT, &reg);
	if (ret) {
		dev_err(hall->dev, "getting HALL status failed\n");
		return ret;
	}

	return (hall->open_state == (reg & BD71828_HALL_STATE_MASK));
}

static irqreturn_t hall_hndlr(int irq, void *data)
{
	struct bd71828_hall *hall = data;
	int open;
	static u64 open_time;

	/*
	 * Do we need this state flag or can we simply disable irq if hall is
	 * not enabled? Do we need to keep the IRQ enabled for wake to work?
	 */
	if (!hall->enabled)
		return IRQ_HANDLED;

	cancel_delayed_work_sync(&hall->delayed_event);

	/*
	 * We return IRQ none if reading fails as this may be a sign
	 * of broken HW and we want to avoid irq-storm which prevents debugging
	 */
	open = lid_is_open(hall);
	if (open < 0)
		return IRQ_NONE;

	if (!open) {
		hall_send_open_event(hall->dev, !open, hall);
		/*
		 * We have IRQF_ONESHOT specified - it should be safe to omit
		 * locking
		 */
		open_time = ktime_get_ns();
	} else {
		u64 now = ktime_get_ns();
		unsigned int left;

		now -= open_time;
		left = ((unsigned int)(MAGIC_GRACE_TIME - now)) / 1000000;
		if (now < MAGIC_GRACE_TIME && left)
			schedule_delayed_work(&hall->delayed_event,
					      msecs_to_jiffies(left));
		else
			hall_send_close_event(hall->dev, !open, hall);
	}

	return IRQ_HANDLED;
}

static ssize_t hall_hw_state_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret;
	int open;
	struct bd71828_hall *hall = dev_get_drvdata(dev);

	open = lid_is_open(hall);
	if (open < 0)
		return open;

	ret = sprintf(buf, "%d", open);

	return ret;
}

static DEVICE_ATTR(hall_hw_state, 0444, hall_hw_state_show, NULL);

static ssize_t hall_enable_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int ret;
	unsigned long int val;
	struct bd71828_hall *hall = dev_get_drvdata(dev);

	/*
	 *  Could we just enable/disable the IRQ here? Or do we need IRQ enabled
	 * for wake?
	 */
	ret = kstrtoul(buf, 0, &val);

	if (ret)
		return ret;

	hall->enabled = !!val;

	return size;
}

static ssize_t hall_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct bd71828_hall *hall = dev_get_drvdata(dev);

	/*
	 *  Could we just use IRQ enable/disable state here? Or do we need IRQ
	 *  enabled for wake?
	 */
	return sprintf(buf, "%d\n", hall->enabled);
}

static DEVICE_ATTR(hall_enable, 0644, hall_enable_show, hall_enable_store);

struct attribute *bd71828_attrs[] = {
	&dev_attr_hall_hw_state.attr,
	&dev_attr_hall_enable.attr,
	NULL,
};

struct attribute_group bd71828_att_grp = {
	.attrs = bd71828_attrs,
};

static int bd71828_remove(struct platform_device *pdev)
{
	struct bd71828_hall *hall = dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &bd71828_att_grp);
	cancel_delayed_work_sync(&hall->delayed_event);

	return 0;
}

static void hall_close_work_fun(struct work_struct *work)
{
	struct bd71828_hall *hall = container_of(work, struct bd71828_hall,
						 delayed_event.work);

	hall_send_close_event(hall->dev, 1, hall);
}

struct input_dev *hall_input_device_create(void)
{
	int err = 0;
	struct input_dev *input;

	input = input_allocate_device();
	if (!input) {
		err = -ENOMEM;
		goto exit;
	}
	input->name = "bd71828-lid";
	set_bit(EV_SW, input->evbit);
	set_bit(SW_LID, input->swbit);

	err = input_register_device(input);
	if (err)
		goto exit_input_free;

	return input;

exit_input_free:
	input_free_device(input);
exit:
	return NULL;
}

static int bd71828_probe(struct platform_device *pdev)
{
	int irq, ret = 0;
	struct bd71828_hall *hall;
	struct rohm_regmap_dev *mfd;
	struct input_dev *input;

	mfd = dev_get_drvdata(pdev->dev.parent);
	if (!mfd) {
		dev_err(&pdev->dev, "No MFD driver data\n");
		return -EINVAL;
	}
	hall = devm_kzalloc(&pdev->dev, sizeof(*hall), GFP_KERNEL);
	if (!hall)
		return -ENOMEM;

	input = hall_input_device_create();
	if (!input) {
		dev_info(&pdev->dev, "Failed to allocate input device\n");
		ret = -ENOMEM;
		goto fail1;
	}

	input->dev.parent = &pdev->dev;
	hall->input = input;

	hall->regmap = mfd->regmap;
	hall->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, hall);

	if (of_property_read_bool(pdev->dev.parent->of_node,
				  "rohm,lid-open-high"))
		hall->open_state = BD71828_HALL_STATE_MASK;

	hall->enabled = true;

	INIT_DELAYED_WORK(&hall->delayed_event, hall_close_work_fun);

	irq = platform_get_irq_byname(pdev, "bd71828-hall");
	ret = devm_request_threaded_irq(&pdev->dev, irq, NULL, &hall_hndlr,
					IRQF_ONESHOT, "bd70528-hall", hall);
	if (ret)
		return ret;

	ret = sysfs_create_group(&pdev->dev.kobj, &bd71828_att_grp);
	return ret;
fail1:
	kfree(hall);
	return ret;
}

static struct platform_driver bd71828_hall = {
	.driver = {
		.name = "bd71828-lid"
	},
	.probe = bd71828_probe,
	.remove = bd71828_remove,
};

module_platform_driver(bd71828_hall);

MODULE_AUTHOR("Matti Vaittinen <matti.vaittinen@fi.rohmeurope.com>");
MODULE_DESCRIPTION("BD71828 LID event driver");
MODULE_LICENSE("GPL");
