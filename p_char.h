// ************************ Définitions ********************************
#define DEV_MINOR_STRT 0
#define DEV_CNT 1
#define MOD_NAME "etsele_cdev"
#define DEFAULT_RWSIZE 16
#define DEFAULT_BUFSIZE 32
#define BUF_DATA_TYPE char
#define MAX_USER 10

#include "ioctl_cmd.h"

// ************************ Structures ********************************
struct BufStruct {
unsigned int InIdx;
unsigned int OutIdx;
unsigned short BufFull;
unsigned short BufEmpty;
unsigned int BufSize;
char *Buffer;
};
struct Buf_Dev {
char	*ReadBuf;
char *WriteBuf;
unsigned short numWriter;
unsigned short numReader;
unsigned short numUser;
unsigned short maxUser;
struct class	*mclass;
dev_t dev;
struct cdev cdev;
};
// ************************ Prototypes ********************************
int BufIn(struct BufStruct *Buf, char *Data);
int BufOut (struct BufStruct *Buf, char *Data);
static int buf_init(void);
static void buf_exit(void);
int buf_open(struct inode *inode, struct file *flip);
int buf_release(struct inode *inode, struct file *flip);
ssize_t buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops);
ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops);
long buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);
