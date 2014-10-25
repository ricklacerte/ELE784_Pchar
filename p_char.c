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
struct semaphore	SemBuf;
struct semaphore	SemBDev;
struct semaphore	SemWriteBuf;
struct semaphore	SemReadBuf;

// déclaration des variables atomiques

atomic_t BUFFER_SIZE =ATOMIC_INIT(16);
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
	BDev.WriteBuf=(char *) kmalloc(DEFAULT_RWSIZE*sizeof(BUF_DATA_TYPE),__GFP_NORETRY);
	BDev.ReadBuf=(char *) kmalloc(DEFAULT_RWSIZE*sizeof(BUF_DATA_TYPE),__GFP_NORETRY);
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
	int nb_chaine=0;
	int no_chaine=0;
	int nb_char=0;
	int temp=0;
	int err=0;
	int char_total=0;
	int while_flag=1;

 	
 	printk(KERN_WARNING "Buffer_circulaire READ: Begin");
	printk(KERN_WARNING "Buffer_circulaire READ: CHAR to read=%d \n",count);

	
//CAPTURE
	down_interruptible(&SemBuf);
	down_interruptible(&SemReadBuf);
	
	nb_chaine=count/DEFAULT_RWSIZE;	//nb chaines totales
	printk(KERN_WARNING "Buffer_circulaire READ: nb_chaine=%d \n", nb_chaine);
		
while(no_chaine<=nb_chaine && while_flag){

	//Nombres de CHARs à transférer pour ce no_chaine.
	if(no_chaine<nb_chaine){
		nb_char=DEFAULT_RWSIZE;
	}
	else{
		nb_char=count%DEFAULT_RWSIZE; //derniere chaine
	}
	printk(KERN_WARNING "Buffer_circulaire READ: no_chaine=%d nb_char=%d \n", no_chaine,nb_char);


	char_read=0;

	//Remplir READBuf
	while(char_read<nb_char && while_flag)
	{
		res=BufOut(&Buffer,&BDev.ReadBuf[char_read]); // OK=0, Buffer vide =-1 
		//printk(KERN_WARNING "Buffer_circulaire READ: char_read= %d, char=%c \n",char_read,BDev.ReadBuf[char_read]);
		//printk(KERN_WARNING "Buffer_circulaire READ: char %d %c in circle Buffer",char_read,BDev.ReadBuf[char_read]);
		//printk(KERN_WARNING "Buffer_circulaire READ: res=%d, char_read=%d, count=%d \n",res,char_read,count);

		if (res!=0){ 
			//Buffer maintenant vide
			printk(KERN_WARNING "Buffer_circulaire READ: Buffer vide!\n");
			
			//MODE : NON BLOQUANT
			if(flip->f_flags & O_NONBLOCK){
				printk(KERN_WARNING "Buffer_circulaire READ: Non Bloquant\n");	
				while_flag=0;
				break;			
			}

			//MODE : BLOQUANT
			else{
				printk(KERN_WARNING "Buffer_circulaire READ: Bloquant\n");
				up(&SemBuf); 
				up(&SemReadBuf);

				printk(KERN_WARNING "Buffer_circulaire READ: Task(PROP=%s PID= %d)SLEEP",current->comm,current->pid);
				wait_event_interruptible(READ_Queue,Buffer.BufEmpty!=1); 

				down_interruptible(&SemBuf);
				down_interruptible(&SemReadBuf);
				printk(KERN_WARNING "Buffer_circulaire READ: Task(PROP=%s PID= %d)AWAKEN",current->comm,current->pid);			
			}
		}
		char_read++;
	}

	if(res==0){
	//ENVOYER READBuf au USER
	temp=no_chaine*Buffer.BufSize;
	err+=copy_to_user(&ubuf[temp],BDev.ReadBuf,nb_char);
	printk(KERN_WARNING "Buffer_circulaire READ: ubuf start at=%d, nb_char=%d, err=%d \n",temp,nb_char,err);
	}

	no_chaine ++;
}

//FIN de la lecture
	printk(KERN_WARNING "Buffer_circulaire READ: Wake Write_Queue \n");		
	wake_up(&WRITE_Queue);

	//nb total CHAR tranférés
	char_total=(no_chaine-1)*DEFAULT_RWSIZE+char_read-err;
	printk(KERN_WARNING "Buffer_circulaire READ: end, OutIdx =%d \n", Buffer.OutIdx);

	up(&SemBuf);
	up(&SemReadBuf);

	printk(KERN_WARNING "Buffer_circulaire READ: end, Char lue =%d \n", char_total);

//RETURN
	if(char_total)
		return char_total;	
	else
		return -EAGAIN;

}







ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops){
	
	unsigned int char_write=0;
	unsigned int char_miss=0;

	int res;
	int temp=0;
	
	int no_chaine=0;
	int nb_chaine=0;
	int nb_char=0;
	int char_total=0;
	
	unsigned int while_flag=1;
	
	printk(KERN_WARNING "Buffer_circulaire WRITE: count= %d \n", count);

//CAPTURE
	down_interruptible(&SemWriteBuf); 	//test
	down_interruptible(&SemBuf); 		//test

	nb_chaine=count/DEFAULT_RWSIZE;	//nb chaine totales
	printk(KERN_WARNING "Buffer_circulaire WRITE: nb_chaine=%d \n", nb_chaine);
		
while(no_chaine<=nb_chaine && while_flag){
	/*	Relache le buffer entre chaques chaines?? nécessaire?? a voir! 
		(p-e intéressant pour éviter l'interblocage si READER = BLOCK et WRITER=BLOCK*/
	//down_interruptible(&SemWriteBuf); 	
	//down_interruptible(&SemBuf);

//Nombres de CHARs à transférer pour ce no_chaine.
	if(no_chaine<nb_chaine){
		nb_char=DEFAULT_RWSIZE;
	}
	else{
		nb_char=count%DEFAULT_RWSIZE; //derniere chaine
	} 
	//printk(KERN_WARNING "Buffer_circulaire WRITE: no_chaine=%d nb_char=%d \n", no_chaine,nb_char);

//écriture dans WriteBuf
	temp=no_chaine*DEFAULT_RWSIZE; 
	printk(KERN_WARNING "Buffer_circulaire WRITE: ubuf start at=%d \n",temp);
	char_miss+=copy_from_user(BDev.WriteBuf,&ubuf[temp],nb_char);
	printk(KERN_WARNING "Buffer_circulaire WRITE: bytes missed in WriteBuf %d \n", char_miss);

//écriture dans le Buffer circulaire
		char_write=0;
		while(char_write<nb_char && while_flag)
		{
			//printk(KERN_WARNING "Buffer_circulaire WRITE: IN char_write= %d, char=%c \n",char_write,BDev.WriteBuf[char_write]);
			//printk(KERN_WARNING "Buffer_circulaire WRITE: ecrit char %c dans WriteBuf \n",BDev.WriteBuf[char_write]);
			res=BufIn(&Buffer,&(BDev.WriteBuf[char_write]));
			if(res!=0)//0=OK, -1 = Full
			{
			//BUFFER = FULL
			printk(KERN_WARNING "Buffer_circulaire WRITE: Buffer FULL! \n");
				//NON-BLOQUANT
				if(flip->f_flags & O_NONBLOCK){
					printk(KERN_WARNING "Buffer_circulaire WRITE: Non Bloquant \n");
					while_flag=0;
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
		no_chaine++;
		if(while_flag<1) break;
}

//fin d'une écriture OU Buffer full-> Wake READ!
	printk(KERN_WARNING "Buffer_circulaire WRITE: Wake READ_Queue \n");		
	wake_up(&READ_Queue);

	char_total=(no_chaine-1)*DEFAULT_RWSIZE+char_write-char_miss;
	printk(KERN_WARNING "Buffer_circulaire WRITE: end InIdx=%d\n",Buffer.InIdx);

//RELACHE
	up(&SemBuf);
	up(&SemWriteBuf);

	printk(KERN_WARNING "Buffer_circulaire WRITE: end char_total=%d\n",char_total);	
	if (char_total)
		return char_total;
	else
		return -EAGAIN;		
}




long buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg){
/*Commande (cmd) :	1-> GetNumData
					2-> GetNumReader
					3->	GetBufSize
					4-> SetBufSize
*/
//Variables locales
int nb_data;
int res;

printk(KERN_WARNING "Buffer_circulaire IOCTL: Begin cmd=%d\n",cmd);
//Commandes
switch (cmd){

//GetNumData
	case GET_NUM_DATA:{
	printk(KERN_WARNING "Buffer_circulaire IOCTL: GetNumData\n");
	
	down_interruptible(&SemBuf);
	nb_data=Buffer.InIdx-Buffer.OutIdx;
	up(&SemBuf);
	res=put_user(nb_data,(int*)arg);
	//res=copy_to_user(&arg,&nb_data,4);
	printk(KERN_WARNING "Buffer_circulaire IOCTL: data=%d, res=%d\n",nb_data,res);
	return 1;
	}

//GetNumReader
	case GET_NUM_READER:{
	printk(KERN_WARNING "Buffer_circulaire IOCTL: GetNumReader\n");
	
 	down_interruptible(&SemBDev);
	put_user(BDev.numReader,(int*)arg);
	up(&SemBDev);

	return 1;
	}

//GetBufSize
	case GET_BUF_SIZE:{
	printk(KERN_WARNING "Buffer_circulaire IOCTL: GetBufSize\n");

	/*Amélioration :peut-etre mettre un RWsem pour le BufSize -> beaucoup de lecture, peu d'écriture*/
	down_interruptible(&SemBuf);
	put_user(Buffer.BufSize,(int*)arg);
	up(&SemBuf);

	return 1;
	}

//SetBufSize
	case SET_BUF_SIZE:{
	printk(KERN_WARNING "Buffer_circulaire IOCTL: GetBufSize, PERM=\n");
	//Vérifier la permission
	if(!capable(CAP_SYS_ADMIN))
		return -EPERM;
	
	return 1;
	}
	
	default:{
	return -ENOTTY;	
	}
}


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
