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

//déclaration des semaphores

struct semaphore    SemBuf;
struct semaphore    SemBDev;
struct semaphore	SemWriteBuf;
struct semaphore	SemReadBuf;
struct semaphore	SemSignalRead; //pour envoyer un signal données disponibles aux lecteurs
struct semaphore	SemSignalWrite;//pour envoyer un signal données lu on peut continuer à écrire

// déclaration des flags

atomic_t wait_data_write = ATOMIC_INIT(0);
atomic_t wait_data_read = ATOMIC_INIT(0);



//**************************************** PILOTE ********************************************
static int __init buf_init(void){
	
	printk("Buffer_circulaire : Begin init\n");
	
	//printk(KERN_ALERT "init");
	//printk(KERN_ALERT "Hello, world\n");
	alloc_chrdev_region(&BDev.dev,DEV_MAJOR,DEV_MINOR, MOD_NAME);
	BDev.mclass=class_create(THIS_MODULE, MOD_NAME);
	device_create(BDev.mclass, NULL, BDev.dev, NULL, MOD_NAME);
	
///initialisation des verrous
	sema_init(&SemBuf,1);
	sema_init(&SemBDev,1);
	sema_init(&SemWriteBuf,1);
	sema_init(&SemReadBuf,1);
	sema_init(&SemSignalRead,0);
	sema_init(&SemSignalWrite,0);
	
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
	Buffer.Buffer=(unsigned short *)kmalloc(DEFAULT_BUFSIZE*sizeof(char),__GFP_NORETRY);
//valider le malloc

/// initialisation des buffers lecture et ecriture
	BDev.WriteBuf=(unsigned short *) kmalloc(atomic_read(&DEFAULT_RWSIZE)*sizeof(char),__GFP_NORETRY);
	BDev.ReadBuf=(unsigned short *) kmalloc(atomic_read(&DEFAULT_RWSIZE)*sizeof(char),__GFP_NORETRY);
	//valider le malloc

	cdev_add(&BDev.cdev, BDev.dev, 1);
	
	printk("Buffer_circulaire : end init\n");
	
	return 0;
}

static void __exit buf_exit(void){
// Destruction du Buffer Circulaire
	printk("Buffer_circulaire : Begin exit\n");
	kfree(Buffer.Buffer);
	kfree(BDev.WriteBuf);
	kfree(BDev.ReadBuf);

//Désallocation du Device
	unregister_chrdev_region(BDev.dev,1);
	cdev_del(&BDev.cdev);
	device_destroy(BDev.mclass, BDev.dev);
	class_destroy(BDev.mclass);
	unregister_chrdev_region(BDev.dev, DEV_MAJOR);

	printk("Buffer_circulaire : end exit\n");
}

int buf_open(struct inode *inode, struct file *flip){
	
	struct Buf_Dev *BDev;
	
	printk("Buffer_circulaire : Begin open\n");

	BDev = container_of(inode->i_cdev, struct Buf_Dev, cdev); // Calvin help!!!

	// Vérifier protection struct BDev...
	down_interruptible(&SemBDev);
	
	
	// Vérifie le MODE du USER
	
	switch (flip->f_flags & O_ACCMODE){ // voir library asm/fcntl.h (faire un mask sur les acces)
		case O_RDONLY:
			BDev->numReader ++;
			printk("Buffer_circulaire open : as RDONLY\n");
			break;

		case O_WRONLY:
			if (!BDev->numWriter){
				BDev->numWriter ++;
				printk("Buffer_circulaire open : as WRONLY\n");
				}
			else{
				up(&SemBDev);
				return -ENOTTY;//plutot EBUSY non??
			}
			break;				

		case O_RDWR:
			if (!BDev->numWriter){
				BDev->numWriter ++;
				BDev->numReader ++;
				printk("Buffer_circulaire open : as RDWR\n");	
			}			
			else{
				up(&SemBDev);
				return -ENOTTY; //EBUSY NON?
			}
			break;
	
		default:
			up(&SemBDev);
			return -ENOTSUPP;				
	}
	flip->private_data=BDev;
	up(&SemBDev);
	
	printk("Buffer_circulaire : end open\n");
	
	return 0;
}

int buf_release(struct inode *inode, struct file *flip){
	
	printk("Buffer_circulaire : Begin release\n");
	
	switch (flip->f_flags & O_ACCMODE){
	down_interruptible(&SemBDev); // à valider avec Calvin Machine!
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
	up(&SemBDev);
	
	printk("Buffer_circulaire : end release\n");
	
	return 0;
}

ssize_t buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops){
 	
 	
 	int cpt=0;
 	
 	printk("Buffer_circulaire : Begin read\n");

 	 
	count=count/sizeof(char); //on veut un nombre de char à lire pas un poids
	
	if(count>atomic_read(&DEFAULT_RWSIZE))// vérifie l'overload
		count=atomic_read(&DEFAULT_RWSIZE);
	
 	//verifier le mode bloquant ou non (flag flip->f_flags)


	// abandon du try lock suite à la remarque du prof
	//~ if (bool_nonblock){
			//~ //vérifier Buffer circulaire est dispo (en premier)
		//~ if(down_trylock(&SemBuf)) 
			//~ return -EAGAIN;
		//~ }
	//~ else
		//~ down_interruptible(&SemBuf);
	

	down_interruptible(&SemReadBuf);
	
	
	down_interruptible(&SemBuf);
	//~ if(bool_nonblock){
			//~ //vérifer ReadBuf est dispo en AYANT le Buffer circulaire
//~ 
		//~ if(down_trylock(&SemReadBuf)) 
			//~ return -EAGAIN;
	//~ }
	//~ else
		//~ down_interruptible(&SemReadBuf);
		
	// à ce moment on possède la sema du buffer circulaire et du buffer de lecture
	
	while(cpt<count)
	{
		if(BufOut(&Buffer,&(BDev.ReadBuf[cpt])))//renvoie -1 sur buffer vide
		{
			
			if(flip->f_flags & O_NONBLOCK)
				break;
			else //on attend d'avoir des données
				{
					up(&SemBuf); //relache le buffer circulaire
					
					if(atomic_read(&wait_data_read)>0 && cpt>0){
						up(&SemSignalRead);//envoi du signal "données disponibles"
					}//l'envoi du signal ici permet d'éviter un interblocage sur le cas ou le read et write attendent le signal

					atomic_inc(&wait_data_write);//incrémentation du flag
					down_interruptible(&SemSignalWrite);//on attend le signal de la fonction buf_write
					atomic_set(&wait_data_write,0); //reset du flag d'attente de données
					down_interruptible(&SemBuf);
				}
			}
	cpt++;	
	}
	up(&SemBuf);
	//à ce niveau on possède le ReadBuf et nos données s'y trouve
	if(atomic_read(&wait_data_read)>0 && cpt>0){
		up(&SemSignalRead);//envoi du signal "données disponibles"
	}//l'envoi du signal ici permet d'éviter un interblocage sur le cas ou le read et write attendent le signal

	
	cpt-=copy_to_user(ubuf,&(BDev.ReadBuf),cpt);
	//unsigned long copy_to_user (void __user * to, const void * from, unsigned long n);
		
	up(&SemReadBuf);
	
	printk("Buffer_circulaire : end read\n");
	
	return cpt;

	// Outbuf
	// Relacher Buffer circulaire
	// copy_to_user
	//relache ReadBuf
	//retourne à l'usager nombre de byte lue
}
ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops){
	
	int cpt=0;
	

	//verifier le mode bloquant ou non (flag flip->f_flags)


	printk("Buffer_circulaire : Begin write\n");
	count=count/sizeof(char); //idem que pour buf_read on veut un nombre de byte pas un poids
	
	// vérifie l'overload à confirmer avec CALVIN!!!!!!!!!!!!!
	if(count>atomic_read(&DEFAULT_RWSIZE))
		count=atomic_read(&DEFAULT_RWSIZE);	
	
	down_interruptible(&SemWriteBuf);
	
	count-=copy_from_user(&(BDev.WriteBuf),ubuf,count);
	///unsigned long copy_from_user (void * to, const void __user * from, unsigned long n);
	
	down_interruptible(&SemBuf);
	
	while(cpt<count)
	{
		if(BufIn(&Buffer,&(BDev.WriteBuf[cpt])))//return -1 si Buffer Full
		{
			if(flip->f_flags & O_NONBLOCK)
				break;
			else
			{
				up(&SemBuf);
				
				if(atomic_read(&wait_data_write)>0 && cpt>0){
					up(&SemSignalWrite);//envoi du signal "données disponibles"
				}//l'envoi du signal ici permet d'éviter un interblocage sur le cas ou le read et write attendent le signal
				
				
				atomic_inc(&wait_data_read);//incrémentation du flag
				down_interruptible(&SemSignalRead);//on attend le signal de la fonction buf_write
				atomic_set(&wait_data_read,0); //reset du flag d'attente de données
				down_interruptible(&SemBuf);
					///Faire un signal sur le Read et ensuite suppr le break
				break;
			}
		}
		cpt++;
	}


	if(atomic_read(&wait_data_write)>0 && cpt>0){
		up(&SemSignalWrite);//envoi du signal "données disponibles"
	}
	up(&SemBuf);
	up(&SemWriteBuf);
	
	printk("Buffer_circulaire : end write\n");

	return cpt;
	
	//vérifier Buffer WriteBuf est dispo (sema)
	//copy_from_user : vérifier le succès + message a retourner au user
	//Vérifer Buffer circulaire Sema 
	// BufIn (loop)
	//relache des semaphores
	//si test sur wait_data up sur semSignal
	//retourne à l'usager nombre de byte écrit
			
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
