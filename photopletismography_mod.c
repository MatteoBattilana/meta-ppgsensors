#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

static dev_t mymod_dev;
struct cdev mymod_cdev;
struct class *myclass = NULL;
static char buffer[64];

ssize_t mymod_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	if (count > 0)
		*buf++ = 'T';
	if (count > 1)
		*buf++ = 'S';
	
	printk(KERN_INFO "[mymod] read (count=%d, offset=%d)\n", (int)count, (int)*f_pos );
	return count;
}

ssize_t mymod_write(struct file *filp, const char *buffer, size_t length, loff_t * offset)
{
	printk(KERN_INFO "[mymod] write (length=%d, offset=%d)\n", (int)length, (int)*offset );
	return length;
}

int mymod_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "[mymod] open" );
	return 0;
}

int mymod_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "[mymod] release" );
	return 0;
}

struct file_operations mymod_fops = {
	.owner = THIS_MODULE,
	.read = mymod_read,
	.write = mymod_write,
	.open = mymod_open,
	.release = mymod_release, 
};

static int __init mymod_module_init(void)
{
	printk(KERN_INFO "Loading mymod_module\n");

	alloc_chrdev_region(&mymod_dev, 0, 1, "mymod_dev");
	printk(KERN_INFO "%s\n", format_dev_t(buffer, mymod_dev));

	myclass = class_create(THIS_MODULE, "mymod_sys");
	device_create(myclass, NULL, mymod_dev, NULL, "mymod_dev");

	cdev_init(&mymod_cdev, &mymod_fops);
	mymod_cdev.owner = THIS_MODULE;
	cdev_add(&mymod_cdev, mymod_dev, 1);
	return 0;
}

static void __exit mymod_module_cleanup(void)
{
	printk(KERN_INFO "Cleaning-up mymod_dev.\n");
	cdev_del(&mymod_cdev);
	unregister_chrdev_region(mymod_dev, 1);
}

module_init(mymod_module_init);
module_exit(mymod_module_cleanup);
MODULE_AUTHOR("Massimo Violante");
MODULE_LICENSE("GPL");
