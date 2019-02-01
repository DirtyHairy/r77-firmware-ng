/*
 * A Virtual Mouse Driver to send fake events from userspace.
 *
 * Written by Fred Chien <fred@ullab.org>
 *
 */
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/input.h>
#include <linux/platform_device.h>

struct input_dev *virmouse_input_dev;
static struct platform_device *virmouse_dev; /* Device structure */

/* Sysfs method to input simulated coordinates */
static ssize_t write_virmouse(struct device *dev,
                              struct device_attribute *attr,
                              const char *buffer, size_t count)
{
        int x, y, key;

        /* parsing input data */
        sscanf(buffer, "%d%d%d", &x, &y, &key);

        /* Report relative coordinates */
        input_report_rel(virmouse_input_dev, REL_X, x);
        input_report_rel(virmouse_input_dev, REL_Y, y);

        printk ("virmouse_event: X:%d Y:%d %d\n", x, y, key);

        /* Report key event */
        if (key>0) {
                if (key==1)
                        input_report_key(virmouse_input_dev, BTN_LEFT, 1);
                else if (key==2)
                        input_report_key(virmouse_input_dev, BTN_MIDDLE, 1);
                else
                        input_report_key(virmouse_input_dev, BTN_RIGHT, 1);
        }

        input_sync(virmouse_input_dev);

        return count;

}

/* Attach the sysfs write method */
DEVICE_ATTR(vmevent, 0644, NULL, write_virmouse);

/* Attribute Descriptor */
static struct attribute *virmouse_attrs[] = {
        &dev_attr_vmevent.attr,
        NULL
};

/* Attribute group */
static struct attribute_group virmouse_attr_group = {
        .attrs = virmouse_attrs,
};

/* Driver Initializing */
int __init virmouse_init(void)
{
        /* Register a platform device */
        virmouse_dev = platform_device_register_simple("virmouse", -1, NULL, 0);
        if (IS_ERR(virmouse_dev)){
                printk ("virmouse_init: error\n");
                return PTR_ERR(virmouse_dev);
        }

        /* Create a sysfs node to read simulated coordinates */
        sysfs_create_group(&virmouse_dev->dev.kobj, &virmouse_attr_group);

        /* Allocate an input device data structure */
        virmouse_input_dev = input_allocate_device();
        if (!virmouse_input_dev) {
                printk("Bad input_allocate_device()\n");
                return -ENOMEM;
        }

        /* Announce that the virtual mouse will generate relative coordinates */
        set_bit(EV_REL, virmouse_input_dev->evbit);
        set_bit(REL_X, virmouse_input_dev->relbit);
        set_bit(REL_Y, virmouse_input_dev->relbit);
//        set_bit(REL_WHEEL, virmouse_input_dev->relbit);


        /* Announce key event */
        set_bit(EV_KEY, virmouse_input_dev->evbit);
        set_bit(BTN_LEFT, virmouse_input_dev->keybit);
        set_bit(BTN_MIDDLE, virmouse_input_dev->keybit);
        set_bit(BTN_RIGHT, virmouse_input_dev->keybit);

        /* Register with the input subsystem */
        input_register_device(virmouse_input_dev);

        /* print messages in the dmesg */
        printk("Virtual Mouse Driver Initialized.\n");

        return 0;
}

/* Driver Uninitializing */
void virmouse_uninit(void)
{
        /* Unregister from the input subsystem */
        input_unregister_device(virmouse_input_dev);

        /* Remove sysfs node */
        sysfs_remove_group(&virmouse_dev->dev.kobj, &virmouse_attr_group);

        /* Unregister driver */
        platform_device_unregister(virmouse_dev);

        return;
}

module_init(virmouse_init);
module_exit(virmouse_uninit);

//MODULE_AUTHOR("");
//MODULE_DESCRIPTION("");
//MODULE_LICENSE("GPL");
