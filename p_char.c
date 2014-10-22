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
#include <linux/sched.h>

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

//déclaration des semaphores

struct semaphore    SemBuf;
struct semaphore    SemBDev;
struct semaphore	SemWriteBuf;
struct semaphore	SemReadBuf;

// déclaration des variables atomiques

atomic_t DEFAULT_RWSIZE =ATOMIC_INIT(16); //Paramètre en INT car modifiable par IOCTL
atomic_t wait_data_write = ATOMIC_INIT(0);
atomic_t wait_data_read = ATOMIC_INIT(0);

// file d'attente (BLOQUANT)
//struct wait_queue_head_t READ_Queue;
DECLARE_WAIT_QUEUE_HEAD(READ_Queue);
DECLARE_WAIT_QUEUE_HEAD(WRITE_Queue);



//**************************************** PILOTE ********************************************
static int __init buf_init(void){
	
	printk(KERN_WARNING "Buffer_circulaire INIT: begin\n");
	
	alloc_chrdev_region(&BDev.dev,DEV_MINOR_STRT,DEV_CNT, MOD_NAME);
	BDev.mclass=class_create(THIS_MODULE, MOD_NAME);
	device_create(BDev.mclass, NULL, BDev.dev, NULL, MOD_NAME);
	
///initialisation des verrous
	sema_init(&SemBuf,1);
	sema_init(&SemBDev,1);
	sema_init(&SemWriteBuf,1);
	sema_init(&SemReadBuf,1);

//initialisation file d'attente
	//DECLARE_WAIT_QUEUE_HEAD(READ_Queue);
	init_waitqueue_head(&READ_Queue);
	init_waitqueue_head(&WRITE_Queue);

	
///initialisation du BDev
	cdev_init(&BDev.cdev, &Buf_fops);
	BDev.cdev.owner = THIS_MODULE;
	BDev.cdev.ops = &Buf_fops;
	//sema_init(&BDev.SemBuf,1);
	BDev.numWriter=0;
	BDev.numReader=0;

///initialisation du BufStruct
	Buffer.InIdx=0;//write
	Buffer.OutIdx=0;//read
	Buffer.BufFull=0;
	Buffer.BufEmpty=1;
Buffer.BufSize=DEFAULT_BUFSIZE;
	Buffer.Buffer=(char *)kmalloc(DEFAULT_BUFSIZE*sizeof(BUF_DATA_TYPE),__GFP_NORETRY);
//valider le malloc

/// initialisation des buffers lecture et ecriture
	BDev.WriteBuf=(char *) kmalloc(atomic_read(&DEFAULT_RWSIZE)*sizeof(BUF_DATA_TYPE),__GFP_NORETRY);
	BDev.ReadBuf=(char *) kmalloc(atomic_read(&DEFAULT_RWSIZE)*sizeof(BUF_DATA_TYPE),__GFP_NORETRY);
	//valider le malloc

	cdev_add(&BDev.cdev, BDev.dev, 1);
	
	printk(KERN_WARNING "Buffer_circulaire INIT: end\n");
	
	return 0;
}

static void __exit buf_exit(void){
// Destruction du Buffer Circulaire
	printk(KERN_WARNING "Buffer_circulaire EXIT: begin\n");
	kfree(Buffer.Buffer);
	kfree(BDev.WriteBuf);
	kfree(BDev.ReadBuf);

//Désallocation du Device
	//unregister_chrdev_region(BDev.dev,1);
	cdev_del(&BDev.cdev);
	device_destroy(BDev.mclass, BDev.dev);
	class_destroy(BDev.mclass);
	unregister_chrdev_region(BDev.dev, DEV_MINOR_STRT);

	printk(KERN_WARNING "Buffer_circulaire EXIT: end\n");
}







int buf_open(struct inode *inode, struct file *flip){
	
	//struct Buf_Dev *BDev;
	
	printk(KERN_WARNING "Buffer_circulaire OPEN : begin\n");
	
	down_interruptible(&SemBDev);
	printk(KERN_WARNING "Buffer_circulaire OPEN : capturer sem BDEV\n");
	

	//BDev = container_of(inode->i_cdev, struct Buf_Dev*, cdev);	
	flip->private_data=&BDev;

// Vérifie le MODE du USER
	
	switch (flip->f_flags & O_ACCMODE){ // mask sur les access
		case O_RDONLY:
			BDev.numReader ++;
			printk(KERN_WARNING "Buffer_circulaire OPEN : as RDONLY\n");
			break;

		case O_WRONLY:
			if (!BDev.numWriter){
				BDev.numWriter ++;
				printk(KERN_WARNING "Buffer_circulaire OPEN : as WRONLY\n");
				}
			else{
				up(&SemBDev);
				return -EBUSY;
			}
			break;				

		case O_RDWR:
			if (!BDev.numWriter){
				BDev.numWriter ++;
				BDev.numReader ++;
				printk(KERN_WARNING "Buffer_circulaire OPEN :as RDWR\n");	
			}			
			else{
				up(&SemBDev);
				return -EBUSY; 
			}
			break;
	
		default:
			up(&SemBDev);
			return -ENOTSUPP;				
	}
	up(&SemBDev);
	
	printk(KERN_WARNING "Buffer_circulaire OPEN: END numWriter=%d numReader=%d\n",BDev.numWriter,BDev.numReader);
	
	return 0;
}










int buf_release(struct inode *inode, struct file *flip){
	
	printk(KERN_WARNING "Buffer_circulaire RELEASE: Begin\n");
	
	switch (flip->f_flags & O_ACCMODE){
	down_interruptible(&SemBDev); 
	printk(KERN_WARNING "Buffer_circulaire RELEASE: Begin numWriter=%d numReader=%d\n",BDev.numWriter,BDev.numReader);
			case O_RDONLY:
				BDev.numReader --;
				break;

			case O_WRONLY:
				BDev.numWriter --;
				break;				

			case O_RDWR:
				BDev.numWriter --;
				BDev.numReader --;
				break;
	
			default:
				up(&SemBDev);
				return -ENOTSUPP;				
	}

	printk(KERN_WARNING "Buffer_circulaire RELEASE: end numWriter=%d numReader=%d\n",BDev.numWriter,BDev.numReader);			
	up(&SemBDev);
	
	printk(KERN_WARNING "Buffer_circulaire RELEASE: end\n");	
	return 0;
}









ssize_t buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops){ 
 	
	// Variables LOCALES
	int res=0;
 	int char_read=0;
	int n=0;
 	
 	printk(KERN_WARNING "Buffer_circulaire READ: Begin");

	//nombre de CHAR à lire 
	count=count/sizeof(BUF_DATA_TYPE);
	
	// Vérifie l'overload du ReadBuf
	if(count>atomic_read(&DEFAULT_RWSIZE))
		count=atomic_read(&DEFAULT_RWSIZE); 

	printk(KERN_WARNING "Buffer_circulaire READ: CHAR to read=%d \n",count);
	

	down_interruptible(&SemBuf);
	printk(KERN_WARNING "Buffer_circulaire READ: got SemBuf");
	down_interruptible(&SemReadBuf);
	printk(KERN_WARNING "Buffer_circulaire READ: got ReadBuf");	
	
	while(char_read<count)
	{
		res=BufOut(&Buffer,&BDev.ReadBuf[char_read]); // OK=0, Buffer vide =-1 
		//printk(KERN_WARNING "Buffer_circulaire READ: char %d %c in circle Buffer",char_read,BDev.ReadBuf[char_read]);
		//printk(KERN_WARNING "Buffer_circulaire READ: res=%d, char_read=%d, count=%d \n",res,char_read,count);

		if (res!=0){ 
			//Buffer maintenant vide
			printk(KERN_WARNING "Buffer_circulaire READ: Buffer vide!\n");
			
			//MODE : NON BLOQUANT
			if(flip->f_flags & O_NONBLOCK){
				printk(KERN_WARNING "Buffer_circulaire READ: Non Bloquant\n");	
				break;			
			}

			//MODE : BLOQUANT
			else{
				printk(KERN_WARNING "Buffer_circulaire READ: Bloquant\n");
				up(&SemBuf); 
				up(&SemReadBuf);

				printk(KERN_WARNING "Buffer_circulaire READ: Task(PROP=%s PID= %d)SLEEP",current->comm,current->pid);
				wait_event_interruptible(READ_Queue,Buffer.BufEmpty!=1); 

				printk(KERN_WARNING "Buffer_circulaire READ: Tasks AWAKEN");
				down_interruptible(&SemBuf);
				down_interruptible(&SemReadBuf);			
			}
		}
		
		char_read++;
	}

	printk(KERN_WARNING "Buffer_circulaire READ: Wake Write_Queue \n");		
	wake_up(&WRITE_Queue);

	printk(KERN_WARNING "Buffer_circulaire READ:end, OutIdx =%d \n", Buffer.OutIdx);
	up(&SemBuf);

	//nb CHARs renvoyé au USER
	n=copy_to_user(ubuf,BDev.ReadBuf,char_read);
	char_read-=n;
	up(&SemReadBuf);

	printk(KERN_WARNING "Buffer_circulaire READ: end, Char lue =%d \n", char_read);
	if (char_read)	
		return char_read;
	else
		return -EAGAIN;

}







ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops){
	
	unsigned int char_write=0;
	unsigned long char_miss;
	unsigned long nb_bytes=0;
	//int i;
	int res;

	nb_bytes=count/sizeof(char); //idem que pour buf_read on veut un nombre de byte pas un poids
	printk(KERN_WARNING "Buffer_circulaire WRITE: bytes to Write Buf %lu \n", nb_bytes);

	// vérifie l'overload à confirmer avec CALVIN!!!!!!!!!!!!!
	if(count>atomic_read(&DEFAULT_RWSIZE))
		count=atomic_read(&DEFAULT_RWSIZE);	
	
	//écriture dans WriteBuf
	down_interruptible(&SemWriteBuf);
	printk(KERN_WARNING "Buffer_circulaire WRITE: WriteBuf capture \n");
	char_miss=copy_from_user(BDev.WriteBuf,ubuf,nb_bytes);
	printk(KERN_WARNING "Buffer_circulaire WRITE: bytes missed in WriteBuf %lu \n", char_miss);

	//écriture dans le Buffer circulaire
	down_interruptible(&SemBuf);
	printk(KERN_WARNING "Buffer_circulaire WRITE: Buffer circulaire capture \n");


		while(char_write<count)
		{
			printk(KERN_WARNING "Buffer_circulaire WRITE: ecrit char %c dans WriteBuf \n",BDev.WriteBuf[char_write]);
		
			res=BufIn(&Buffer,&(BDev.WriteBuf[char_write]));
			if(res!=0)//0=OK, -1 = Full
			{
			//BUFFER = FULL

				//NON-BLOQUANT
				if(flip->f_flags & O_NONBLOCK){
					printk(KERN_WARNING "Buffer_circulaire WRITE: Non Bloquant \n");
					break;
				}

				//BLOQUANT
				else
				{
					printk(KERN_WARNING "Buffer_circulaire WRITE: Bloquant \n");
					up(&SemBuf);
					up(&SemWriteBuf);
				
					printk(KERN_WARNING "Buffer_circulaire WRITE: Task(PROP=%s PID= %d)SLEEP",current->comm,current->pid);
					wait_event_interruptible(WRITE_Queue,Buffer.BufFull!=1); 
				
					down_interruptible(&SemWriteBuf);
					down_interruptible(&SemBuf);
				}
			}
			char_write++;
		}
		//fin d'une écriture OU Buffer circulaire full-> Wake READ!
		printk(KERN_WARNING "Buffer_circulaire WRITE: Wake READ_Queue \n");		
		wake_up(&READ_Queue);

	printk(KERN_WARNING "Buffer_circulaire WRITE: end InIdx=%d\n",Buffer.InIdx);
	up(&SemBuf);
	up(&SemWriteBuf);
	
	printk(KERN_WARNING "Buffer_circulaire WRITE: end char_write=%d\n",char_write);	

	if (char_write)	
		return char_write;
	else
		return -EAGAIN;		
}




long buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg){
	return 0;
}

//************************************** FONCTIONS  ************************************************
int BufIn(struct BufStruct *Buf, char *Data) {
    if (Buf->BufFull)
        return -1;
    Buf->BufEmpty = 0;
    Buf->Buffer[Buf->InIdx] = *Data;
    Buf->InIdx = (Buf->InIdx + 1) % Buf->BufSize;
    if (Buf->InIdx == Buf->OutIdx)
        Buf->BufFull = 1;
    return 0;
}

int BufOut (struct BufStruct *Buf, char *Data) {
    if (Buf->BufEmpty)
        return -1;
    Buf->BufFull = 0;
    *Data = Buf->Buffer[Buf->OutIdx];
    Buf->OutIdx = (Buf->OutIdx + 1) % Buf->BufSize;
    if (Buf->OutIdx == Buf->InIdx)
        Buf->BufEmpty = 1;
    return 0;
}
