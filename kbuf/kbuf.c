/*
 * kbuf - Simple single-device character driver for a 1 KiB kernel buffer.
 * Author: Mohamed GALY
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>


#undef pr_fmt
#define pr_fmt(fmt)             KBUILD_MODNAME ": %s: " fmt, __func__

#define NO_OF_DEVICES           1
#define KBUF_DEV_NAME           "kbuf"
#define KBUF_CLASS_NAME         "kbuf_class"
#define BUFF_SIZE               1024

/* The main buffer */
static char kbuffer[BUFF_SIZE];
static dev_t kbuf_devNum;
static struct cdev kbuf_cdev;
static struct class *kbuf_class;
static struct device *kbuf_device;


/*=====================================
 * File operations
 *=====================================*/

static ssize_t kbuf_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    pr_info("Read requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n",*f_pos);

    if(*f_pos >= BUFF_SIZE)
	    return 0;	/* EOF */

    /* Limit count to bytes remaining from current position */
	if(*f_pos + count > BUFF_SIZE)
		count = BUFF_SIZE - *f_pos;

    if(copy_to_user(buf,(kbuffer + *f_pos),count))
	    return -EFAULT;

    /* Increment position */
    *f_pos += count;
	pr_info("read  %zu bytes, pos=%lld\n", count, *f_pos);

    return count;
}


static ssize_t kbuf_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
    pr_info("Write requested for %zu bytes\n", count);
    pr_info("Current file position = %lld\n",*f_pos);

    if(*f_pos >= BUFF_SIZE)
	    return 0;	/* EOF */

    /* Limit count to bytes remaining from current position */
	if (*f_pos + count > BUFF_SIZE)
		count = BUFF_SIZE - *f_pos;

	if (!count)
		return -ENOSPC;

	if (copy_from_user((kbuffer + *f_pos), buf, count))
		return -EFAULT;

    /* Increment position */
	*f_pos += count;
    pr_info("write %zu bytes, pos=%lld\n", count, *f_pos);
   
    return count;
}


static loff_t kbuf_llseek(struct file *file, loff_t offset, int whence)
{
	loff_t newpos;

	switch (whence) {
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = file->f_pos + offset;
		break;
	case SEEK_END:
		newpos = BUFF_SIZE + offset;
		break;
	default:
		return -EINVAL;
	}

	if (newpos < 0 || newpos > BUFF_SIZE)
		return -EINVAL;

	file->f_pos = newpos;
	pr_info("llseek -> %lld\n", newpos);
	return newpos;
}


static int kbuf_open(struct inode *inode, struct file *file)
{
	pr_info("open\n");
	return 0;
}


static int kbuf_release(struct inode *inode, struct file *file)
{
	pr_info("release\n");
	return 0;
}


static const struct file_operations kbuf_fops = {
	.owner   = THIS_MODULE,
	.read    = kbuf_read,
	.write   = kbuf_write,
	.llseek  = kbuf_llseek,
    .open    = kbuf_open,
	.release = kbuf_release,
};


static int __init kbuf_init(void)
{
    int ret;

    /* Allocate a [Major, Minor] range */
    ret = alloc_chrdev_region(&kbuf_devNum,0,NO_OF_DEVICES,KBUF_DEV_NAME);
    if(ret < 0) {
        pr_err("Allocation of major and minor number failed, error code:%d\n",ret);
        return ret;
    }

    /* Register cdev with the kernel */
    cdev_init(&kbuf_cdev,&kbuf_fops);
    kbuf_cdev.owner = THIS_MODULE;

    ret = cdev_add(&kbuf_cdev,kbuf_devNum, NO_OF_DEVICES);
    if(ret < 0) {
        pr_err("cdev_add failed, error code:%d\n",ret);
        goto err_unregister; 
    }

    kbuf_class = class_create(THIS_MODULE,KBUF_CLASS_NAME);
    if(IS_ERR(kbuf_class)) {
        /* Convert the pointer to an error value */
        ret = PTR_ERR(kbuf_class);
        pr_err("class_create failed, error code:%d\n",ret);
        goto err_cdev_del;
    }

    kbuf_device = device_create(kbuf_class,NULL,kbuf_devNum,NULL,KBUF_DEV_NAME);
    if(IS_ERR(kbuf_device)) {
        /* Convert the pointer to an error value */
        ret = PTR_ERR(kbuf_device);
        pr_err("device_create failed, error code:%d\n",ret);
        goto err_class_destroy;
    }
    
    pr_info("loaded, major=%d minor=%d\n", MAJOR(kbuf_devNum), MINOR(kbuf_devNum));
    return 0;


err_class_destroy:
	class_destroy(kbuf_class);
err_cdev_del:
    cdev_del(&kbuf_cdev);
err_unregister:
    unregister_chrdev_region(kbuf_devNum, NO_OF_DEVICES);
    return ret;
}


static void __exit kbuf_exit(void)
{
    device_destroy(kbuf_class, kbuf_devNum);
    class_destroy(kbuf_class);
    cdev_del(&kbuf_cdev);
    unregister_chrdev_region(kbuf_devNum, NO_OF_DEVICES);
    pr_info("unloaded\n");
}


module_init(kbuf_init);
module_exit(kbuf_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohamed GALY");
MODULE_DESCRIPTION("Simple single-device character driver for a 1 KiB kernel buffer");
MODULE_VERSION("1.0");
