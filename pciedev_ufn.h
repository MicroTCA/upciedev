/** @file pciedev_ufn.H
 *
 *  @brief PCI Express universal driver
 *
 *  @author Ludwig Petrosyan
**/

/** 
 *  @mainpage
 *  
 *  @section intro Introduction
 *  This PCI Express universal Device Driver build by DESY-Zeuthen.\n
 *  This Kernel Module contains all PCI Express as well MTCA Standard specific functionality.\n
 *  The Module maps all enabled BARs of the PCIe endpoint and provides read/write as well some common IOCTRLs.\n 
 *  The top level Device Drivers use functionality provided by this Module and, if needed, add Device specific functionality.\n
 *  \n
 *  @date	24.04.2013
 *  @author	Ludwig Petrosyan
 *  @author	David Kalantaryan
**/

#ifndef   PCIEDEV_UFN_H
#define  PCIEDEV_UFN_H

#define	_DEPRICATED2_
#define	_LOCK_TIMEOUT_MS_	2000

#include <linux/version.h>
#include <linux/pci.h>
#include <linux/types.h>	/* size_t */
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>
#include "criticalregionlock.h"
#include "pciedev_io.h"

#ifndef NUMBER_OF_BARS
#define NUMBER_OF_BARS 6
#endif

#ifndef PCIEDEV_NR_DEVS
#define PCIEDEV_NR_DEVS 15    /* pciedev0 through pciedev15 */
#endif

#define ASCII_BOARD_MAGIC_NUM         0x424F5244 /*BORD*/
#define ASCII_PROJ_MAGIC_NUM             0x50524F4A /*PROJ*/
#define ASCII_BOARD_MAGIC_NUM_L      0x626F7264 /*bord*/

/*FPGA Standard Register Set*/
#define WORD_BOARD_MAGIC_NUM      0x00
#define WORD_BOARD_ID                       0x04
#define WORD_BOARD_VERSION            0x08
#define WORD_BOARD_DATE                  0x0C
#define WORD_BOARD_HW_VERSION     0x10
#define WORD_BOARD_RESET                0x14
#define WORD_BOARD_TO_PROJ            0x18

#define WORD_PROJ_MAGIC_NUM         0x00
#define WORD_PROJ_ID                          0x04
#define WORD_PROJ_VERSION               0x08
#define WORD_PROJ_DATE                    0x0C
#define WORD_PROJ_RESERVED            0x10
#define WORD_PROJ_RESET                   0x14
#define WORD_PROJ_NEXT                    0x18

#define WORD_PROJ_IRQ_ENABLE         0x00
#define WORD_PROJ_IRQ_MASK             0x04
#define WORD_PROJ_IRQ_FLAGS            0x08
#define WORD_PROJ_IRQ_CLR_FLAGSE  0x0C

#define PCIED_FPOS  7

#define	_PROC_ENTRY_CREATED		0
#define	_DEVC_ENTRY_CREATED		1
#define	_MUTEX_CREATED			2
//alloc_chrdev_region
#define _CHAR_REG_ALLOCATED			3
#define _CDEV_ADDED				4
#define _PCI_DRV_REG				5
#define	_IRQ_REQUEST_DONE			6
#define	_PCI_DEVICE_ENABLED		7
#define	_PCI_REGIONS_REQUESTED		8
#define	_CREATED_ADDED_BY_USER		9

struct upciedev_file_list {
    struct list_head node_file_list;
    struct file *filp;
    int      file_cnt;
};
typedef struct upciedev_file_list upciedev_file_list;

#if defined(WIN32) & !defined(u32)
#define u32 unsigned
#endif

struct pciedev_brd_info {
    u32 PCIEDEV_BOARD_ID;
    u32 PCIEDEV_BOARD_VERSION;
    u32 PCIEDEV_BOARD_DATE ; 
    u32 PCIEDEV_HW_VERSION  ;
    u32 PCIEDEV_BOARD_RESERVED  ;
    u32 PCIEDEV_PROJ_NEXT  ;
};
typedef struct pciedev_brd_info pciedev_brd_info;

struct pciedev_prj_info {
    struct list_head prj_list;
    u32 PCIEDEV_PROJ_ID;
    u32 PCIEDEV_PROJ_VERSION;
    u32 PCIEDEV_PROJ_DATE ; 
    u32 PCIEDEV_PROJ_RESERVED  ;
    u32 PCIEDEV_PROJ_NEXT  ;
};
typedef struct pciedev_prj_info pciedev_prj_info;

struct pciedev_cdev ;
struct pciedev_dev {
   
	struct cdev           cdev;	           /* Char device structure      */
	//struct mutex        dev_mut;            /* mutual exclusion semaphore */
	struct SCriticalRegionLock	dev_mut;            /* mutual exclusion semaphore */
	struct pci_dev      *pciedev_pci_dev;
	char                     dev_name[64];
	int                        binded;          /* is binded to device*/
	int                        brd_num;       /* board num dev index*/
	int                        dev_minor;     /* file minor num = PCIEDEV_MINOR + brd_num */
	int                        dev_num;       /*  file dev_t = MKDEV(PCIEDEV_MAJOR, (dev_minor))*/
	int                        dev_sts;
	int                        dev_file_ref;
	int                        null_dev;
	int                        swap;
	
	int					irq_type : 3;
	void*					irqData;
	const struct pci_device_id*	m_id;
	unsigned long int		m_ulnFlag;
	void*					remove;
	void*					deviceData;
	struct device*			deviceFl;
	void*					parent;

	int							register_size : 7;
    
	struct upciedev_file_list dev_file_list; 
    
	int                        slot_num;
	u16                      vendor_id;
	u16                      device_id;
	u16                      subvendor_id;
	u16                      subdevice_id;
	u16                      class_code;
	u8                        revision;
	u32                      devfn;
	u32                      busNumber;
	u32                      devNumber;
	u32                      funcNumber;
	int                        bus_func;

	u32                  mem_base[NUMBER_OF_BARS];
	u32                  mem_base_end[NUMBER_OF_BARS];
	u32                  mem_base_flag[NUMBER_OF_BARS];
	loff_t                rw_off[NUMBER_OF_BARS];
	void __iomem*  memmory_base[NUMBER_OF_BARS];
	/******************************************************/
    
	int      dev_dma_64mask;
	int      pciedev_all_mems ;
	int      dev_payload_size;

	u32    scratch_bar;
	u32    scratch_offset;

	u8                         msi;
	int                         irq_flag;
	u16                       irq_mode;
	u8                         irq_line;
	u8                         irq_pin;
	u32                       pci_dev_irq;

	struct pciedev_cdev *parent_dev;
	void                          *dev_str;
    
	struct pciedev_brd_info brd_info_list;
	struct pciedev_prj_info  prj_info_list;
	int                                 startup_brd;
	int                                 startup_prj_num;
};
typedef struct pciedev_dev pciedev_dev;

struct pciedev_cdev {
	u16 UPCIEDEV_VER_MAJ;
	u16 UPCIEDEV_VER_MIN;
	u16 PCIEDEV_DRV_VER_MAJ;
	u16 PCIEDEV_DRV_VER_MIN;
	int   PCIEDEV_MAJOR ;     /* major by default */
	int   PCIEDEV_MINOR  ;    /* minor by default */

	pciedev_dev                   *pciedev_dev_m[PCIEDEV_NR_DEVS + 1];
	struct class                    *pciedev_class;
	struct proc_dir_entry     *pciedev_procdir;
	int                                   pciedevModuleNum;
};
typedef struct pciedev_cdev pciedev_cdev;


void     upciedev_cleanup_module_exp(pciedev_cdev **);
int       upciedev_init_module_exp(pciedev_cdev **, struct file_operations *, char *);

int       pciedev_probe_exp(struct pci_dev *, const struct pci_device_id *,  struct file_operations *, pciedev_cdev *, char *, int * );
int       pciedev_remove_exp(struct pci_dev *dev, pciedev_cdev *, char *, int *);

int        pciedev_open_exp( struct inode *, struct file * );
int        pciedev_release_exp(struct inode *, struct file *);
ssize_t  pciedev_read_exp(struct file *, char __user *, size_t , loff_t *);
ssize_t  pciedev_write_exp(struct file *, const char __user *, size_t , loff_t *);
loff_t    pciedev_llseek(struct file *, loff_t, int);
long     pciedev_ioctl_exp(struct file *, unsigned int* , unsigned long* , pciedev_cdev *);
int        pciedev_set_drvdata(struct pciedev_dev *, void *);
void*    pciedev_get_drvdata(struct pciedev_dev *);
int        pciedev_get_brdnum(struct pci_dev *);
pciedev_dev*   pciedev_get_pciedata(struct pci_dev *);
void*    pciedev_get_baraddress(int, struct pciedev_dev *);

int       pciedev_get_prjinfo(struct pciedev_dev *);
int       pciedev_fill_prj_info(struct pciedev_dev *, void *);
int       pciedev_get_brdinfo(struct pciedev_dev *);

//adding mmap staff

//int       pciedev_check_scratch(struct pciedev_dev *, int );

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *, struct pt_regs *), struct pciedev_dev *, char *);
#else
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *), struct pciedev_dev *, char *);
#endif

void register_upciedev_proc(int num, char * dfn, struct pciedev_dev     *p_upcie_dev, struct pciedev_cdev     *p_upcie_cdev);
void unregister_upciedev_proc(int num, char *dfn);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	int        pciedev_procinfo(char *, char **, off_t, int, int *,void *);
#else
	//static int pciedev_proc_open(struct inode *inode, struct file *file);
	//static int pciedev_proc_show(struct seq_file *m, void *v);
	int pciedev_proc_open(struct inode *inode, struct file *file);
	int pciedev_proc_show(struct seq_file *m, void *v);
	ssize_t pciedev_procinfo(struct file *filp,char *buf,size_t count,loff_t *offp );
	extern const struct file_operations upciedev_proc_fops;
	//    static const struct file_operations upciedev_proc_fops = { 
	//        .open           = pciedev_proc_open,
	//        .read           = seq_read,
	//        .llseek         = seq_lseek,
	//        .release       = single_release,
	//    }; 
#endif
	
extern void UociedevTestFunction(const char* report);

#endif	/* PCIEDEV_UFN_H */

