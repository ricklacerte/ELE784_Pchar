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
#include <linux/semaphore.h>

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

char* WriteBuf;
char* ReadBuf;

struct semaphore    SemBDev;
struct semaphore	SemWriteBuf;
struct semaphore	SemReadBuf;


//**************************************** PILOTE ********************************************
static int __init buf_init(void){
	
	//printk(KERN_ALERT "init");
	//printk(KERN_ALERT "Hello, world\n");
	alloc_chrdev_region(&BDev.dev,DEV_MAJOR,DEV_MINOR, MOD_NAME);
	BDev.mclass=class_create(THIS_MODULE, MOD_NAME);
	device_create(BDev.mclass, NULL, BDev.dev, NULL, MOD_NAME);
	
//initialisation des verrous
	sema_init(&SemBDev,1);
	sema_init(&SemWriteBuf,1);
	sema_init(&SemReadBuf,1);

///initialisation du BDev
	cdev_init(&BDev.cdev, &Buf_fops);
	BDev.cdev.owner = THIS_MODULE;
	BDev.cdev.ops = &Buf_fops;
	sema_init(&BDev.SemBuf,1);
	BDev.numWriter=0;
	BDev.numReader=0;

///initialisation du BufStruct
	Buffer.InIdx=0;//write
    Buffer.OutIdx=0;//read
    Buffer.BufFull=0;
    Buffer.BufEmpty=1;
    Buffer.BufSize=DEFAULT_BUFSIZE;
	Buffer.Buffer=(unsigned short *)kmalloc(DEFAULT_BUFSIZE*sizeof(char),__GFP_NORETRY);
//valider le malloc

/// initialisation des buffers lecture et ecriture

	WriteBuf=(char*) kmalloc(DEFAULT_RWSIZE*sizeof(char),__GFP_NORETRY);
	ReadBuf=(char*) kmalloc(DEFAULT_RWSIZE*sizeof(char),__GFP_NORETRY);
//valider le malloc
	


	cdev_add(&BDev.cdev, BDev.dev, 1);
	
	return 0;
}

static void __exit buf_exit(void){
// Destruction du Buffer Circulaire
	kfree(Buffer.Buffer);
	kfree(WriteBuf);
	kfree(ReadBuf);

//Désallocation du Device
	unregister_chrdev_region(BDev.dev,1);
	cdev_del(&BDev.cdev);
	device_destroy(BDev.mclass, BDev.dev);
	class_destroy(BDev.mclass);
	unregister_chrdev_region(BDev.dev, DEV_MAJOR);

	
}

int buf_open(struct inode *inode, struct file *flip){

	struct Buf_Dev *BDev;
	BDev = container_of(inode->i_cdev, struct Buf_Dev, cdev);


	// Vérifie le MODE du USER
	switch (flip->f_flags){
		case O_RDONLY:
			BDev->numReader ++;
			break;

		case O_WRONLY:
			if (!BDev->numWriter)
				BDev->numWriter ++;
			else
				return -ENOTTY;
			break;				

		case O_RDWR:
			//*** à définir ****
			if (!BDev->numWriter){
				BDev->numWriter ++;
				BDev->numReader ++;	
			}			
			else
				return -ENOTTY;
			break;
	
		default:
			// *** à définir (code erreur) ***
			return -ENOTSUPP;				
	}
	flip->private_data=BDev;
	return 0;
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
 	//verifier le mode bloquant ou non (flag flip->f_flags)
	//vérifier Buffer circulaire est dispo (en premier)
	//vérifer ReadBuf est dispo en AYANT le Buffer circulaire
	// Outbuf
	// Relacher Buffer circulaire
	// copy_to_user
	//relache ReadBuf
	//retourne à l'usager nombre de byte lue

	return 0;
}
ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops){
//verifier le mode bloquant ou non (flag flip->f_flags)

	//vérifier Buffer WriteBuf est dispo (sema)
	//copy_from_user : vérifier le succès + message a retourner au user
	//Vérifer Buffer circulaire Sema 
	// BufIn (loop)
	//retourne à l'usager nombre de byte écrit
			
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
