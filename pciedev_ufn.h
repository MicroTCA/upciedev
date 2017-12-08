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

/**
*Copyright 2016-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
*
*This file is part of UPCIEDEV driver.
*
*Foobar is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*Foobar is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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
#include <linux/mm.h>
#include <linux/uaccess.h>
#include "criticalregionlock.h"
#include "pciedev_io.h"

#ifndef NUMBER_OF_BARS
#define NUMBER_OF_BARS 6
#endif

#define NUMBER_OF_SLOTS 12  

#ifndef PCIEDEV_NR_DEVS
#define PCIEDEV_NR_DEVS 15    /* pciedev0 through pciedev15 */
#endif

#define ASCII_BOARD_MAGIC_NUM         0x424F5244 /*BORD*/
#define ASCII_PROJ_MAGIC_NUM             0x50524F4A /*PROJ*/
#define ASCII_BOARD_MAGIC_NUM_L      0x626F7264 /*bord*/

#define SHAPI_MAGIC_DEVICE_NUM               0x00005348 /*SH*/
#define SHAPI_MAGIC_MODULE_NUM             0x0000534D /*SMJ*/

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

//SHAPI DEVICE Registers
#define WORD_DEVICE_VERSION			0x00
#define WORD_FIRST_MODULE_ADDRESS            0x04
#define WORD_HW_IDS                                               0x08
#define WORD_FW_IDS                                               0x0C
#define WORD_FW_VERSION                                    0x10
#define WORD_FW_TIMESTAMP                               0x14
#define WORD_FW_NAME                                          0x18
#define WORD_DEVICE_CAP                                      0x24
#define WORD_DEVICE_STATUS                               0x28
#define WORD_DEVICE_CONTROL                           0x2C
#define WORD_IRQ_MASK                                          0x30
#define WORD_IRQ_FLAG                                           0x34
#define WORD_IRQ_ACTIVE                                       0x38
#define WORD_SCRATCH_REGISTER                      0x3C

//SHAPI MODULE Registers
#define WORD_MODULE_SHAPI_VERSION		0x00
#define WORD_NEXT_MODULE_ADDRESS            0x04
#define WORD_MODULE_IDS                                    0x08
#define WORD_MODULE_VERSION                         0x0C
#define WORD_MODULE_NAME                               0x10
#define WORD_MODULE_CAP                                   0x18
#define WORD_MODULE_STATUS                            0x1C
#define WORD_MODULE_CONTROL                        0x20
#define WORD_MODULE_IRQ_ID                              0x24
#define WORD_MODULE_IRQ_CLEAR                     0x28
#define WORD_MODULE_IRQ_MASK                       0x2C
#define WORD_MODULE_IRQ_FLAG                        0x30
#define WORD_MODULE_IRQ_ACTIVE                    0x34

#define PCIED_FPOS  7

#define	_PROC_ENTRY_CREATED		0
#define	_DEVC_ENTRY_CREATED		1
#define	_MUTEX_CREATED			2
#define      _CHAR_REG_ALLOCATED			3
#define      _CDEV_ADDED				4
#define      _PCI_DRV_REG				5
#define	_IRQ_REQUEST_DONE			6
#define	_PCI_DEVICE_ENABLED		7
#define	_PCI_REGIONS_REQUESTED		8
#define	_CREATED_ADDED_BY_USER		9

struct upciedev_bar_address {
    u32 res_start;
    u32 res_end;
    u32 res_flag;
};
typedef struct upciedev_bar_address  upciedev_bar_address ;

struct upciedev_phys_address {
    int      slot_num;
    upciedev_bar_address  bars[NUMBER_OF_BARS];
};
typedef struct upciedev_phys_address upciedev_phys_address;

struct upciedev_file_list {
    struct list_head node_file_list;
    struct file *filp;
    int      file_cnt;
};
typedef struct upciedev_file_list upciedev_file_list;

#if defined(WIN32) & !defined(u32)
#define u32 unsigned
#endif

//DESY Register Set
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

//SHAPI PICMG Register Set
struct shapi_device_info {
    u32 SHAPI_VERSION;
    u32 SHAPI_FIRST_MODULE_ADDRESS;
    u32 SHAPI_HW_IDS ; 
    u32 SHAPI_FW_IDS  ;
    u32 SHAPI_FW_VERSION  ;
    u32 SHAPI_FW_TIMESTAMP  ;
    u32 SHAPI_FW_NAME[3];
    u32 SHAPI_DEVICE_CAP;
    u32 SHAPI_DEVICE_STATUS ; 
    u32 SHAPI_DEVICE_CONTROL  ;
    u32 SHAPI_IRQ_MASK  ;
    u32 SHAPI_IRQ_FLAG  ;
    u32 SHAPI_IRQ_ACTIVE;
    u32 SHAPI_SCRATCH_REGISTER;
    char fw_name[13];
};
typedef struct shapi_device_info shapi_device_info;

struct shapi_module_info {
    struct list_head module_list;
    u32 SHAPI_VERSION;
    u32 SHAPI_NEXT_MODULE_ADDRESS;
    u32 SHAPI_MODULE_FW_IDS ; 
    u32 SHAPI_MODULE_VERSION  ;
    u32 SHAPI_MODULE_NAME[2] ;
    u32 SHAPI_MODULE_CAP;
    u32 SHAPI_MODULE_STATUS;
    u32 SHAPI_MODULE_CONTROL ; 
    u32 SHAPI_IRQ_ID  ;
    u32 SHAPI_IRQ_FLAG_CLEAR  ;
    u32 SHAPI_IRQ_MASK  ;
    u32 SHAPI_IRQ_FLAG;
    u32 SHAPI_IRQ_ACTIVE;
    char module_name[9];
    int module_num;
};
typedef struct shapi_module_info shapi_module_info;

struct upciedev_base_dev {
	u16 UPCIEDEV_VER_MAJ;
	u16 UPCIEDEV_VER_MIN;
	upciedev_phys_address dev_phys_addresses[NUMBER_OF_SLOTS];
};
typedef struct upciedev_base_dev upciedev_base_dev;

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
	int                         msi_num;
	int                         irq_flag;
	u16                       irq_mode;
	u8                         irq_line;
	u8                         irq_pin;
	u32                       pci_dev_irq;
	u32                       enbl_irq_num;
	u32                       device_irq_num;

	struct pciedev_cdev            *parent_dev;
	struct upciedev_base_dev  *parent_base_dev;
	void                          *dev_str;
    
	struct pciedev_brd_info brd_info_list;
	struct pciedev_prj_info  prj_info_list;
	int                                 startup_brd;
	int                                 startup_prj_num;
	
	struct shapi_device_info device_info_list;
	struct shapi_module_info module_info_list;
	int                                 shapi_brd;
	int                                 shapi_module_num;
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
//int       upciedev_init_module_exp(pciedev_cdev **, struct file_operations *, char *, upciedev_base_dev *);
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

int       pciedev_get_shapi_module_info(struct pciedev_dev *);
int       pciedev_fill_shapi_module__info(struct pciedev_dev *, void *);

u_int  pciedev_get_physical_address(struct pciedev_dev *, device_phys_address *);

//adding mmap staff
/* The mmap system call declared as:
 *  mmap (caddr_t addr, size_t len, int prot, int flags, int fd, off_t offset)
 * 
 * off_t ofset is a long int, in current implementation of the mmap this the BAR number, this means
 * the BAR will mapped from start (0x0)
 * could be in future changed to remap BAR from some offset, in this case
 * the most 4bits of the offset is a BAR num and the rest 60buts is a offset inside BAR 
 */

void upciedev_vma_open(struct vm_area_struct *vma);
void upciedev_vma_close(struct vm_area_struct *vma);
//static int pciedev_remap_mmap_exp(struct file *filp, struct vm_area_struct *vma);
int pciedev_remap_mmap_exp(struct file *filp, struct vm_area_struct *vma);



//struct vm_operations_struct upciedev_remap_vm_ops = {
//	.open =  upciedev_vma_open,
//	.close = upciedev_vma_close,
//};

// end mmap staff


#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *, struct pt_regs *), struct pciedev_dev *, char *, int);
#else
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *), struct pciedev_dev *, char *, int);
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

