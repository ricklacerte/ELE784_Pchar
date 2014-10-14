// ************************ Définitions ********************************
#define DEV_MAJOR 	0
#define DEV_MINOR	1
#define MOD_NAME	"pchar"

#define READWRITE_BUFSIZE 16
#define DEFAULT_BUFSIZE 256


// ************************ Variables ********************************
struct BufStruct {
    unsigned int    InIdx;
    unsigned int    OutIdx;
    unsigned short  BufFull;
    unsigned short  BufEmpty;
    unsigned int    BufSize;
    unsigned short  *Buffer;
};

struct Buf_Dev {
    unsigned short      *ReadBuf;
    unsigned short      *WriteBuf;
    struct semaphore    SemBuf;
    unsigned short      numWriter;
    unsigned short      numReader;
    struct class		*mclass;
    dev_t               dev;
    struct cdev         cdev;
};

// ************************ Prototypes ********************************
int BufIn(struct BufStruct *Buf, unsigned short *Data);
int BufOut (struct BufStruct *Buf, unsigned short *Data);

static int buf_init(void);
static void buf_exit(void);
int buf_open(struct inode *inode, struct file *flip);
int buf_release(struct inode *inode, struct file *flip);
ssize_t buf_read(struct file *flip, char __user *ubuf, size_t count, loff_t *f_ops);
ssize_t buf_write(struct file *flip, const char __user *ubuf, size_t count, loff_t *f_ops);
long buf_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

