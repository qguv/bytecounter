/*
 * bytecounter: counts bytes written to a special file and reports on read
 *
 * Create a character special file with major number 62 to get started:
 *
 *     # mknode /dev/bytecounter c 62 0
 *     # chmod 666 /dev/bytecounter
 *     # echo -n "hello world" > /dev/bytecounter
 *     # echo "$(cat /dev/bytecounter) characters written"
 *     11 characters written
 */

MODULE_DESCRIPTION("bytecounter: counts bytes written to a special file and reports on read");
MODULE_AUTHOR("Quint Guvernator <quint at guvernator.net>");
MODULE_LICENSE("GPL");

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h> /* size_t */
#include <asm/uaccess.h> /* copy_from/to_user */

#define MODNAME "bytecounter"

/* stores up to ten SI-exabytes */
#define BUFSIZE 20

int bc_open(struct inode *inode, struct file *filp);
int bc_release(struct inode *inode, struct file *filp);
ssize_t bc_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t bc_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos);
void bc_exit(void);
int bc_init(void);

module_init(bc_init);
module_exit(bc_exit);

static struct file_operations bc_fops = {
	read: bc_read,
	write: bc_write,
	open: bc_open,
	release: bc_release
};

static int bc_major = 62;

/* to prevent data races */
static int is_open = 0;

/* running count of number of writes */
static unsigned long int bytecounter = 0;

int bc_init(void)
{
	int result = register_chrdev(bc_major, MODNAME, &bc_fops);
	if (result) {
		printk(KERN_WARNING MODNAME ": can't obtain major number %d\n", bc_major);
		return result;
	}

	printk(KERN_INFO MODNAME ": inserting module\n");
	return 0;
}

void bc_exit(void)
{
	unregister_chrdev(bc_major, MODNAME);
	printk(KERN_INFO MODNAME ": removing module\n");
}

int bc_open(struct inode *inode, struct file *filp)
{
	if (is_open)
		return -EBUSY;

	is_open = 1;
	return 0;
}

int bc_release(struct inode *inode, struct file *filp)
{
	is_open = 0;
	return 0;
}

/* turn bytecounter into a string, then read that out like a file */
ssize_t bc_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	const int bufsize = 20;
	if (count == 0)
		return 0;

	char *msg = kmalloc(bufsize, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	/* turn the uint counter into a string of ASCII numeric characters */
	int len = snprintf(msg, bufsize, "%lu", bytecounter);
	int written = 0;

	/* move forward to the fpos-th element and copy */
	len -= *f_pos;
	len = min(len, count);
	if (len > 0) {
		msg += *f_pos;
		copy_to_user(buf, msg, len);
		written = len;
	}

	kfree(msg);

	*f_pos += written;
	return written;
}

ssize_t bc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	bytecounter += count;
	return count;
}
