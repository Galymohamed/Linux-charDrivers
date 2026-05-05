/*
 * multi_kbuf - Simple multi-device character driver managing 4 kernel buffers.
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
#define pr_fmt(fmt) 				KBUILD_MODNAME ": %s: " fmt, __func__

#define NO_OF_DEVICES				4
#define KBUF_DEV_NAME           	"multiKbuf"
#define KBUF_CLASS_NAME         	"multiKbuf_class"

/* Device permissions */
#define RDONLY						0x01
#define WRONLY						0x02
#define RDWR						(RDONLY | WRONLY)

/* Devices memory areas */
#define MEM_SIZE_MAX_DEV1			1024
#define MEM_SIZE_MAX_DEV2			512
#define MEM_SIZE_MAX_DEV3			1024
#define MEM_SIZE_MAX_DEV4			512

static char device1_buf[MEM_SIZE_MAX_DEV1];
static char device2_buf[MEM_SIZE_MAX_DEV2];
static char device3_buf[MEM_SIZE_MAX_DEV3];
static char device4_buf[MEM_SIZE_MAX_DEV4];

/* Device data */
struct kbuf_dev 
{
	char *buffer;
	unsigned int size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
	struct device *device;
};

/* Driver data */
struct kbuf_drv
{
	int total_devices;
	dev_t device_number;
	struct class *class;
	struct kbuf_dev devs[NO_OF_DEVICES];
};

/* Initialize Driver data structure */
static struct kbuf_drv kbufDrv = 
{
	.total_devices = NO_OF_DEVICES,
	.devs = {
		[0] = {
			.buffer = device1_buf,
			.size = MEM_SIZE_MAX_DEV1,
			.serial_number = "SERIAL-NO-DEV1",
			.perm = RDONLY
		},
		[1] = {
			.buffer = device2_buf,
			.size = MEM_SIZE_MAX_DEV2,
			.serial_number = "SERIAL-NO-DEV2",
			.perm = WRONLY
		},
		[2] = {
			.buffer = device3_buf,
			.size = MEM_SIZE_MAX_DEV3,
			.serial_number = "SERIAL-NO-DEV3",
			.perm = RDWR
		},
		[3] = {
			.buffer = device4_buf,
			.size = MEM_SIZE_MAX_DEV4,
			.serial_number = "SERIAL-NO-DEV4",
			.perm = RDWR
		},
	}
};




/*=====================================
 * Helpers
 *=====================================*/

static int check_permission(int dev_perm, fmode_t acc_mode)
{
	if (dev_perm == RDWR)
		return 0;

	/* Read-only device: opener must want read but not write */
	if ((dev_perm == RDONLY) &&
	    (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE))
		return 0;

	/* Write-only device: opener must want write but not read */
	if ((dev_perm == WRONLY) &&
	    (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ))
		return 0;

	return -EPERM;
}


/*=====================================
 * File operations
 *=====================================*/

static ssize_t kbuf_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct kbuf_dev *currDev = filp->private_data;
	int max_size = currDev->size;

	if (*f_pos >= max_size)
	 	return 0;	/* EOF */

	if (*f_pos + count > max_size)
		count = max_size - *f_pos;

	if (copy_to_user(buf, (currDev->buffer + *f_pos), count))
	 	return -EFAULT;

	*f_pos += count;
	pr_info("read  %zu bytes, pos=%lld\n", count, *f_pos);
	return count;
}


static ssize_t kbuf_write(struct file *filp, const char __user *buf,
			 size_t count, loff_t *f_pos)
{
	struct kbuf_dev *currDev = filp->private_data;
	int max_size = currDev->size;

	if (*f_pos >= max_size)
	 	return -ENOSPC;

	if (*f_pos + count > max_size)
	 	count = max_size - *f_pos;

	if (copy_from_user(currDev->buffer + *f_pos, buf, count))
	 	return -EFAULT;

	*f_pos += count;
	pr_info("write %zu bytes, pos=%lld\n", count, *f_pos);
	return count;
}


static loff_t kbuf_llseek(struct file *filp, loff_t offset, int whence)
{
	struct kbuf_dev *currDev = filp->private_data;
	int max_size = currDev->size;
	loff_t newpos;

	switch (whence) {
		case SEEK_SET:
			newpos = offset;
			break;
		case SEEK_CUR:
			newpos = filp->f_pos + offset;
			break;
		case SEEK_END:
			newpos = max_size + offset;
			break;
		default:
			return -EINVAL;
	}

	if (newpos < 0 || newpos > max_size)
		return -EINVAL;

	filp->f_pos = newpos;
	pr_info("llseek -> %lld\n", newpos);
	return newpos;
}


static int kbuf_open(struct inode *inode, struct file *filp)
{
	struct kbuf_dev *currDev;
	int minor_n = MINOR(inode->i_rdev);
	int ret;

	/* Recover the per-device struct from its embedded cdev */
	currDev = container_of(inode->i_cdev, struct kbuf_dev, cdev);
	/* Save the device data into the file structure private data member 
	   so it can be used later in Read/Write and all other file operations
	 */
	filp->private_data = currDev;

	ret = check_permission(currDev->perm, filp->f_mode);
	if (ret) {
		pr_info("open denied on minor %d\n", minor_n);
		return ret;
	}

	pr_info("open ok on minor %d\n", minor_n);
	return 0;
}


static int kbuf_release(struct inode *inode, struct file *filp)
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


/*=====================================
 * Module init / exit 
 *=====================================*/

static int __init kbufMulti_init(void)
{
	int ret, i;

	/* Allocate [Major, Minor] range */
	ret = alloc_chrdev_region(&kbufDrv.device_number, 0, NO_OF_DEVICES, KBUF_DEV_NAME);
	if(ret < 0) {
		pr_err("Allocation of major and minor number failed, error code:%d\n",ret);
        return ret;
	}

	/* Create a class for all devices */
	kbufDrv.class = class_create(THIS_MODULE, KBUF_CLASS_NAME);
	if(IS_ERR(kbufDrv.class)) {
		ret = PTR_ERR(kbufDrv.class);
        pr_err("class_create failed, error code:%d\n",ret);
		goto err_unregister;
	}

	/* Register cdev + create /dev node for each device */
	for(i = 0; i < NO_OF_DEVICES; i++)  {

		dev_t currDev_num = kbufDrv.device_number + i;
		pr_info("device %d: major:minor = %d:%d\n", i, MAJOR(currDev_num), MINOR(currDev_num));

		/*== cDev ------------------------*/
		cdev_init(&kbufDrv.devs[i].cdev, &kbuf_fops);
		kbufDrv.devs[i].cdev.owner = THIS_MODULE;

		ret = cdev_add(&kbufDrv.devs[i].cdev, currDev_num, 1);
		if (ret < 0) {
			pr_err("cdev_add failed for minor %d: %d\n", i, ret);
			goto err_rollback;
		}
		/*==------------------------------*/

		kbufDrv.devs[i].device = device_create(kbufDrv.class, NULL, currDev_num, NULL, "multiKbuf-%d", (i+1));
		if (IS_ERR(kbufDrv.devs[i].device)) {
			ret = PTR_ERR(kbufDrv.devs[i].device);
			pr_err("device_create failed for minor %d: %d\n", i, ret);
			/* cdev_add succeeded for this index — undo it
			 * before falling into the rollback for 0..i-1 */
			cdev_del(&kbufDrv.devs[i].cdev);
			goto err_rollback;
		}
	}

	pr_info("loaded, %d devices, major=%d\n", NO_OF_DEVICES, MAJOR(kbufDrv.device_number));
	return 0;

err_rollback:
	/* Tear down only the devices fully created (indices 0..i-1) */
	for (i = i - 1; i >= 0; i--) {
		device_destroy(kbufDrv.class, kbufDrv.device_number + i);
		cdev_del(&kbufDrv.devs[i].cdev);
	}
	class_destroy(kbufDrv.class);
err_unregister:
	unregister_chrdev_region(kbufDrv.device_number, NO_OF_DEVICES);
	pr_info("module insertion failed\n");
	return ret;
}


static void __exit kbufMulti_exit(void)
{
	int i;
	for (i = 0; i < NO_OF_DEVICES; i++) {
		device_destroy(kbufDrv.class, kbufDrv.device_number + i);
		cdev_del(&kbufDrv.devs[i].cdev);
	}
	class_destroy(kbufDrv.class);
	unregister_chrdev_region(kbufDrv.device_number, NO_OF_DEVICES);
	pr_info("unloaded\n");
}


module_init(kbufMulti_init);
module_exit(kbufMulti_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mohamed GALY");
MODULE_DESCRIPTION("Simple multi-device character driver managing 4 kernel buffers");
MODULE_VERSION("1.0");