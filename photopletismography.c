#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

static dev_t photopletismography_dev;
struct cdev photopletismography_cdev;
struct class *myclass = NULL;
static char buffer[64];

ssize_t photopletismography_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	if (count > 0)
		*buf++ = 'T';
	if (count > 1)
		*buf++ = 'S';
	
	printk(KERN_INFO "[photopletismography] read (count=%d, offset=%d)\n", (int)count, (int)*f_pos );
	return count;
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
