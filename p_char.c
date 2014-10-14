#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "p_char.h"

MODULE_LICENSE("Dual BSD/GPL");

module_init(buf_init);
module_exit(buf_exit);

struct BufStruct Buffer;
struct Buf_Dev BDev;
struct file_operations Buf_fops ={
    .owner      =   THIS_MODULE,
    .open       =   buf_open,
    .release    =   buf_release,
    .read       =   buf_read,
    .write      =   buf_write,
    .unlocked_ioctl =   buf_ioctl,
};

//**************************************** PILOTE ********************************************
static int __init buf_init(void){
	
	//printk(KERN_ALERT "init");
	//printk(KERN_ALERT "Hello, world\n");
	alloc_chrdev_region(&BDev.dev,DEV_MAJOR,DEV_MINOR, MOD_NAME);
	BDev.mclass=class_create(THIS_MODULE, MOD_NAME);
	device_create(BDev.mclass, NULL, BDev.dev, NULL, MOD_NAME);
	
	cdev_init(&BDev.cdev, &Buf_fops);
	BDev.cdev.owner = THIS_MODULE;
	BDev.cdev.ops = &Buf_fops;

	cdev_add(&BDev.cdev, BDev.dev, 1);
	
	return 0;
}

static void __exit buf_exit(void){
	
	unregister_chrdev_region(BDev.dev,1);
	cdev_del(&BDev.cdev);
	device_destroy(BDev.mclass, BDev.dev);
	class_destroy(BDev.mclass);
	unregister_chrdev_region(BDev.dev, DEV_MAJOR);
	printk(KERN_ALERT "bye");
	
}

int buf_open(struct inode *inode, struct file *flip){

	struct Buf_Dev *BDev;
	BDev = container_of(inode->i_cdev, struct Buf_Dev, cdev);
	
	// vérifie si il y a un ÉCRIVAIN
	if (!BDev->numWriter){ 

		// Vérifie le MODE du USER 
		switch (flip->f_flags){
			case O_RDONLY:
				BDev->numReader ++;
				break;

			case O_WRONLY:
				BDev->numWriter ++;
				break;				

			case O_RDWR:
				//*** à définir ****
				BDev->numWriter ++;
				break;
		
			default:
				// *** à définir (code erreur) ***
				return -ENOTSUPP;				
		}
		flip->private_data=BDev;
		return 0;
	}
	else
	  	return -ENOTTY;
}

int buf_release(struct inode *inode, struct file *flip){
	
	struct Buf_Dev *BDev;

	switch (flip->f_flags){
			case O_RDONLY:
				BDev->numReader --;
				break;

			case O_WRONLY:
				BDev->numWriter --;
				break;				

			case O_RDWR:
				//*** à définir ****
				BDev->numWriter --;
				break;
	
			default:
				// *** théoriquement, impossible ***
				return -ENOTSUPP;				
	}
	return 0;
}

ssize_t buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops){
	
	return 0;
}
ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops){
	
	return 0;
}

long buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg){
	return 0;
}

//************************************** FONCTIONS  ************************************************
int BufIn(struct BufStruct *Buf, unsigned short *Data) {
    if (Buf->BufFull)
        return -1;
    Buf->BufEmpty = 0;
    Buf->Buffer[Buf->InIdx] = *Data;
    Buf->InIdx = (Buf->InIdx + 1) % Buf->BufSize;
    if (Buf->InIdx == Buf->OutIdx)
        Buf->BufFull = 1;
    return 0;
}

int BufOut (struct BufStruct *Buf, unsigned short *Data) {
    if (Buf->BufEmpty)
        return -1;
    Buf->BufFull = 0;
    *Data = Buf->Buffer[Buf->OutIdx];
    Buf->OutIdx = (Buf->OutIdx + 1) % Buf->BufSize;
    if (Buf->OutIdx == Buf->InIdx)
        Buf->BufEmpty = 1;
    return 0;
}
