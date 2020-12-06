#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h> 
#include "data.h"

static dev_t photopletismography_dev;
struct cdev photopletismography_cdev;
struct class *myclass = NULL;
static char buffer[64];
static int index = 0;
static const char    g_s_Hello_World_string[] = "Hello world from kernel mode!\n\0";
ssize_t photopletismography_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int original_value = ppg[++index];
	int value = original_value; 
	index = index % (sizeof(ppg) / sizeof(ppg[0]));
	
	int counter = 0;
	while(value != 0 && counter <= count){
		int vv = value & 0xFF;
		copy_to_user(buf, &vv, 1);
		printk(KERN_INFO "[photopletismography] read (v = %d)\n", value & 0xFF);
		value = value >> 8; 
		counter++;
	}
	copy_to_user(buf, &original_value, 4);
	
	printk(KERN_INFO "[photopletismography] read (value = %d, buf size = %d)\n", original_value, counter);
	return counter;
}

ssize_t photopletismography_write(struct file *filp, const char *buffer, size_t length, loff_t * offset)
{
	printk(KERN_INFO "[photopletismography] write (length=%d, offset=%d)\n", (int)length, (int)*offset );
	return length;
}

int photopletismography_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "[photopletismography] open" );
	return 0;
}

int photopletismography_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "[photopletismography] release" );
	return 0;
}

struct file_operations photopletismography_fops = {
	.owner = THIS_MODULE,
	.read = photopletismography_read,
	.write = photopletismography_write,
	.open = photopletismography_open,
	.release = photopletismography_release, 
};

static int __init photopletismography_module_init(void)
{
	printk(KERN_INFO "Loading photopletismography_module\n");

	alloc_chrdev_region(&photopletismography_dev, 0, 1, "photopletismography_dev");
	printk(KERN_INFO "%s\n", format_dev_t(buffer, photopletismography_dev));

	myclass = class_create(THIS_MODULE, "photopletismography_sys");
	device_create(myclass, NULL, photopletismography_dev, NULL, "photopletismography_dev");

	cdev_init(&photopletismography_cdev, &photopletismography_fops);
	photopletismography_cdev.owner = THIS_MODULE;
	cdev_add(&photopletismography_cdev, photopletismography_dev, 1);
	return 0;
}

static void __exit photopletismography_module_cleanup(void)
{
	printk(KERN_INFO "Cleaning-up photopletismography_dev.\n");
	cdev_del(&photopletismography_cdev);
	unregister_chrdev_region(photopletismography_dev, 1);
}

module_init(photopletismography_module_init);
module_exit(photopletismography_module_cleanup);
MODULE_AUTHOR("Matteo Battilana");
MODULE_LICENSE("GPL");
