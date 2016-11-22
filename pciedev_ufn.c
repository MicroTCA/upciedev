
#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/sched.h>
#include <linux/proc_fs.h>


#include "pciedev_ufn.h"

struct vm_operations_struct upciedev_remap_vm_ops = {
	.open =  upciedev_vma_open,
	.close = upciedev_vma_close,
};

int upciedev_init_module_exp(pciedev_cdev **pciedev_cdev_pp, struct file_operations *pciedev_fops, char *dev_name)
{
	int                    i          = 0;
	int                    k         = 0;
	int                    result  = 0;
	int                    devno  = 0;
	dev_t                devt     = 0;
	char				*realEndPtr;
	char                 **endptr  = &realEndPtr;
	pciedev_cdev   *pciedev_cdev_p;

	printk(KERN_ALERT "############UPCIEDEV_INIT MODULE  NAME %s\n", dev_name);

	pciedev_cdev_p= kzalloc(sizeof(pciedev_cdev), GFP_KERNEL);
	if(!pciedev_cdev_p){
		printk(KERN_ALERT "UPCIEDEV_INIT CREATE CDEV STRUCT NO MEM\n");
		return -ENOMEM;
	}

	pciedev_fops->llseek = pciedev_llseek;

	*pciedev_cdev_pp = pciedev_cdev_p;
	pciedev_cdev_p->PCIEDEV_MAJOR             = 47;
	pciedev_cdev_p->PCIEDEV_MINOR             = 0;
	pciedev_cdev_p->pciedevModuleNum         = 0;
	pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ = 1;
	pciedev_cdev_p->PCIEDEV_DRV_VER_MIN = 1;
	pciedev_cdev_p->UPCIEDEV_VER_MAJ        = 1;
	pciedev_cdev_p->UPCIEDEV_VER_MIN        = 1;

	result = alloc_chrdev_region(&devt, pciedev_cdev_p->PCIEDEV_MINOR, (PCIEDEV_NR_DEVS + 1), dev_name);
	pciedev_cdev_p->PCIEDEV_MAJOR = MAJOR(devt);
	/* Populate sysfs entries */
	pciedev_cdev_p->pciedev_class = class_create(pciedev_fops->owner, dev_name);
	/*Get module driver version information*/
	pciedev_cdev_p->UPCIEDEV_VER_MAJ = simple_strtol(THIS_MODULE->version, endptr, 10);
	pciedev_cdev_p->UPCIEDEV_VER_MIN = simple_strtol(THIS_MODULE->version + 2, endptr, 10);
	printk(KERN_ALERT "&&&&&UPCIEDEV_INIT CALLED; UPCIEDEV MODULE VERSION %i.%i\n", 
			pciedev_cdev_p->UPCIEDEV_VER_MAJ, pciedev_cdev_p->UPCIEDEV_VER_MIN);

	pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ = simple_strtol(pciedev_fops->owner->version, endptr, 10);
	pciedev_cdev_p->PCIEDEV_DRV_VER_MIN = simple_strtol(pciedev_fops->owner->version + 2, endptr, 10);
	printk(KERN_ALERT "&&&&&UPCIEDEV_INIT CALLED; THIS MODULE VERSION %i.%i\n", 
		pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ, pciedev_cdev_p->PCIEDEV_DRV_VER_MIN);
    
    
	for(i = 0; i <= PCIEDEV_NR_DEVS;i++){
        
		pciedev_cdev_p->pciedev_dev_m[i] = kzalloc(sizeof(pciedev_dev), GFP_KERNEL);
		if(!pciedev_cdev_p->pciedev_dev_m[i]){
			printk(KERN_ALERT "AFTER_INIT CREATE DEV STRUCT NO MEM\n");
			for(k = 0; k < i; k++){
				if(pciedev_cdev_p->pciedev_dev_m[k]){
					kfree(pciedev_cdev_p->pciedev_dev_m[k]);
				}
			}
			if(pciedev_cdev_p){
				kfree(pciedev_cdev_p);
			}
			return -ENOMEM;
		}
        
	pciedev_cdev_p->pciedev_dev_m[i]->parent_dev = pciedev_cdev_p;
	devno = MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR, pciedev_cdev_p->PCIEDEV_MINOR + i);
	pciedev_cdev_p->pciedev_dev_m[i]->dev_num    = devno;
	pciedev_cdev_p->pciedev_dev_m[i]->dev_minor  = (pciedev_cdev_p->PCIEDEV_MINOR + i);
	cdev_init(&(pciedev_cdev_p->pciedev_dev_m[i]->cdev), pciedev_fops);
	pciedev_cdev_p->pciedev_dev_m[i]->cdev.owner = THIS_MODULE;
	pciedev_cdev_p->pciedev_dev_m[i]->cdev.ops = pciedev_fops;
	result = cdev_add(&(pciedev_cdev_p->pciedev_dev_m[i]->cdev), devno, 1);
	if (result){
		printk(KERN_NOTICE "Error %d adding devno%d num%d\n", result, devno, i);
		return 1;
	}
	INIT_LIST_HEAD(&(pciedev_cdev_p->pciedev_dev_m[i]->prj_info_list.prj_list));
	INIT_LIST_HEAD(&(pciedev_cdev_p->pciedev_dev_m[i]->dev_file_list.node_file_list));
	//mutex_init(&(pciedev_cdev_p->pciedev_dev_m[i]->dev_mut));
	InitCritRegionLock(&(pciedev_cdev_p->pciedev_dev_m[i]->dev_mut), _DEFAULT_TIMEOUT_);
	pciedev_cdev_p->pciedev_dev_m[i]->dev_sts                   = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->dev_file_ref            = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->irq_mode                = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->msi                         = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->dev_dma_64mask   = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->pciedev_all_mems  = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->brd_num                = i;
	pciedev_cdev_p->pciedev_dev_m[i]->binded                   = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->dev_file_list.file_cnt = 0;
	pciedev_cdev_p->pciedev_dev_m[i]->null_dev                   = 0;

	if(i == PCIEDEV_NR_DEVS){
		pciedev_cdev_p->pciedev_dev_m[i]->binded        = 1;
		pciedev_cdev_p->pciedev_dev_m[i]->null_dev      = 1;
	}
    }
     
    return result; /* succeed */
}
EXPORT_SYMBOL(upciedev_init_module_exp);

void upciedev_cleanup_module_exp(pciedev_cdev  **pciedev_cdev_p)
{  
	int                     k = 0;
	dev_t                 devno ;
	pciedev_cdev     *pciedev_cdev_m;

	printk(KERN_ALERT "UPCIEDEV_CLEANUP_MODULE CALLED\n");

	pciedev_cdev_m = *pciedev_cdev_p;

	devno = MKDEV(pciedev_cdev_m->PCIEDEV_MAJOR, pciedev_cdev_m->PCIEDEV_MINOR);
	unregister_chrdev_region(devno, (PCIEDEV_NR_DEVS + 1));
	class_destroy(pciedev_cdev_m->pciedev_class);
	for(k = 0; k <= PCIEDEV_NR_DEVS; k++){
		cdev_del(&pciedev_cdev_m->pciedev_dev_m[k]->cdev);
		if(pciedev_cdev_m->pciedev_dev_m[k]){
			kfree(pciedev_cdev_m->pciedev_dev_m[k]);
			pciedev_cdev_m->pciedev_dev_m[k] = 0;
		}
	}
	kfree(*pciedev_cdev_p);
	pciedev_cdev_m  = 0;
	*pciedev_cdev_p = 0;
}
EXPORT_SYMBOL(upciedev_cleanup_module_exp);


int    pciedev_open_exp( struct inode *inode, struct file *filp )
{
	int    minor;
	struct pciedev_dev *dev;
	upciedev_file_list *tmp_file_list;

	minor = iminor(inode);
	dev = container_of(inode->i_cdev, struct pciedev_dev, cdev);
	dev->dev_minor     = minor;

	//printk(KERN_ALERT "$$$$$$$$$$$$$OPEN BOARD 0 \n");
	if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
         //printk(KERN_ALERT "$$$$$$$$$$$$$OPEN BOARD 1\n");
	
	dev->dev_file_ref ++;
	filp->private_data  = dev; 
	filp->f_pos  = PCIED_FPOS; 

	tmp_file_list = kzalloc(sizeof(upciedev_file_list), GFP_KERNEL);
	INIT_LIST_HEAD(&tmp_file_list->node_file_list); 
	tmp_file_list->file_cnt = dev->dev_file_ref;
	tmp_file_list->filp = filp;
	list_add(&(tmp_file_list->node_file_list), &(dev->dev_file_list.node_file_list));
         
	//printk(KERN_ALERT "$$$$$$$$$$$$$OPEN BOARD 2\n");
	//mutex_unlock(&dev->dev_mut);
	LeaveCritRegion(&dev->dev_mut);
	//printk(KERN_ALERT "$$$$$$$$$$$$$OPEN BOARD 3\n");
	return 0;
}
EXPORT_SYMBOL(pciedev_open_exp);

int    pciedev_release_exp(struct inode *inode, struct file *filp)
{
    struct pciedev_dev *dev;
    u16 cur_proc     = 0;
    upciedev_file_list *tmp_file_list;
    struct list_head *pos, *q;
    
    dev = filp->private_data;
    //printk(KERN_ALERT "$$$$$$$$$$$$$RELEASE BOARD 0 \n");
    //mutex_lock(&dev->dev_mut);
    if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
    //printk(KERN_ALERT "$$$$$$$$$$$$$RELEASE BOARD 1 \n");
	
    dev->dev_file_ref --;
    cur_proc = current->group_leader->pid;
    list_for_each_safe(pos, q, &(dev->dev_file_list.node_file_list)){
             tmp_file_list = list_entry(pos, upciedev_file_list, node_file_list);
             if(tmp_file_list->filp ==filp){
                 list_del(pos);
                 kfree (tmp_file_list);
             }
    } 
    //printk(KERN_ALERT "$$$$$$$$$$$$$RELEASE BOARD 2 \n");
    //mutex_unlock(&dev->dev_mut);
    LeaveCritRegion(&dev->dev_mut);
    //printk(KERN_ALERT "$$$$$$$$$$$$$RELEASE BOARD 3 \n");
    return 0;
}
EXPORT_SYMBOL(pciedev_release_exp);

int    pciedev_set_drvdata(struct pciedev_dev *dev, void *data)
{
    if(!dev)
        return 1;
    dev->dev_str = data;
    return 0;
}
EXPORT_SYMBOL(pciedev_set_drvdata);

void *pciedev_get_drvdata(struct pciedev_dev *dev){
    if(dev && dev->dev_str)
        return dev->dev_str;
    return NULL;
}
EXPORT_SYMBOL(pciedev_get_drvdata);

int       pciedev_get_brdnum(struct pci_dev *dev)
{
    int                                 m_brdNum;
    pciedev_dev                *pciedevdev;
    pciedevdev        = dev_get_drvdata(&(dev->dev));
    m_brdNum       = pciedevdev->brd_num;
    return m_brdNum;
}
EXPORT_SYMBOL(pciedev_get_brdnum);

pciedev_dev*   pciedev_get_pciedata(struct pci_dev  *dev)
{
    pciedev_dev                *pciedevdev;
    pciedevdev    = dev_get_drvdata(&(dev->dev));
    return pciedevdev;
}
EXPORT_SYMBOL(pciedev_get_pciedata);

void*   pciedev_get_baraddress(int br_num, struct pciedev_dev  *dev)
{
    void *tmp_address;
    tmp_address = 0;
    tmp_address = dev->memmory_base[br_num];
    return tmp_address;
}
EXPORT_SYMBOL(pciedev_get_baraddress);



void upciedev_vma_open(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "UPCIEDEV VMA open, virt %lx, phys %lx\n",
			vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void upciedev_vma_close(struct vm_area_struct *vma)
{
	printk(KERN_NOTICE "UPCIEDEV VMA close.\n");
}

//static int pciedev_remap_mmap_exp(struct file *filp, struct vm_area_struct *vma)
int pciedev_remap_mmap_exp(struct file *filp, struct vm_area_struct *vma)
{
	int tmp_bar_num;
	unsigned long tmp_size = 0;
	unsigned long tmp_off;
	unsigned long tmp_physical;
	unsigned long tmp_psize = 0;
	
	struct pciedev_dev       *dev  = filp->private_data;
	
	//printk(KERN_INFO "PCIEDEV_MMAP:  start %X ; STOP %X; PGOFFSET %X \n",
         //      vma->vm_start, vma->vm_end, vma->vm_pgoff);
	
	tmp_bar_num = vma->vm_pgoff;
	tmp_size         = (vma->vm_end - vma->vm_start);
	
	if(!(dev->mem_base[tmp_bar_num])){
		//printk(KERN_INFO "PCIEDEV_MMAP:  NO MEM FOR BAR  %i \n",tmp_bar_num);
		return -ENOMEM;
	}
	tmp_psize = (dev->mem_base_end[tmp_bar_num] -  dev->mem_base[tmp_bar_num]);
	
	if(tmp_psize > PAGE_SIZE){
		if(tmp_psize < tmp_size){
	/*
			printk(KERN_INFO "PCIEDEV_MMAP:  NO ENOUGH MEM  BAR_SIZE  %i  MMAP SIZE\n",
					(dev->mem_base_end[tmp_bar_num] -  dev->mem_base[tmp_bar_num]), tmp_size);
	*/
			return -ENOMEM;
		}
	}
	
	tmp_off         = vma->vm_pgoff << PAGE_SHIFT;
	tmp_physical = dev->mem_base[tmp_bar_num] >> PAGE_SHIFT;
			
	if (remap_pfn_range(vma,  vma->vm_start , tmp_physical, tmp_size,  vma->vm_page_prot))
		return -EAGAIN;

	vma->vm_ops = &upciedev_remap_vm_ops;
	upciedev_vma_open(vma);
	return 0;
}
EXPORT_SYMBOL(pciedev_remap_mmap_exp);

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *, struct pt_regs *),struct pciedev_dev  *pdev, char  *dev_name)
#else
int pciedev_setup_interrupt(irqreturn_t (*pciedev_interrupt)(int , void *), struct pciedev_dev  *pdev, char  *dev_name)
#endif
{
    int result = 0;
    
    /*******SETUP INTERRUPTS******/
    pdev->irq_mode = 1;
    result = request_irq(pdev->pci_dev_irq, pciedev_interrupt,
                        pdev->irq_flag, dev_name, pdev);
    printk(KERN_INFO "PCIEDEV_PROBE:  assigned IRQ %i RESULT %i\n",
               pdev->pci_dev_irq, result);
    if (result) {
         printk(KERN_INFO "PCIEDEV_PROBE: can't get assigned irq %i\n", pdev->pci_dev_irq);
         pdev->irq_mode = 0;
    }
    return result;
}
EXPORT_SYMBOL(pciedev_setup_interrupt);

int      pciedev_get_brdinfo(struct pciedev_dev  *bdev)
{
    void *baddress;
    void *address;
    int    strbrd = 0;
    u32  tmp_data_32;
    
    bdev->startup_brd = 0;
    if(bdev->memmory_base[0]){ 
        baddress = bdev->memmory_base[0];
        address = baddress;
        tmp_data_32       = ioread32(address );
        if(tmp_data_32 == ASCII_BOARD_MAGIC_NUM || tmp_data_32 ==ASCII_BOARD_MAGIC_NUM_L){
            bdev->startup_brd = 1;
            address = baddress + WORD_BOARD_ID;
            tmp_data_32       = ioread32(address);
            bdev->brd_info_list.PCIEDEV_BOARD_ID = tmp_data_32;

            address = baddress + WORD_BOARD_VERSION;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_BOARD_VERSION = tmp_data_32;

            address = baddress + WORD_BOARD_DATE;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_BOARD_DATE = tmp_data_32;

            address = baddress + WORD_BOARD_HW_VERSION;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_HW_VERSION = tmp_data_32;

            bdev->brd_info_list.PCIEDEV_PROJ_NEXT = 0;
            address = baddress + WORD_BOARD_TO_PROJ;
            tmp_data_32       = ioread32(address );
            bdev->brd_info_list.PCIEDEV_PROJ_NEXT = tmp_data_32;
        }
    }
    
    strbrd = bdev->startup_brd;

    return strbrd;
}
EXPORT_SYMBOL(pciedev_get_brdinfo);

int   pciedev_fill_prj_info(struct pciedev_dev  *bdev, void  *baradress)
{
    void *address;
    int    strbrd  = 0;
    u32  tmp_data_32;
     struct pciedev_prj_info  *tmp_prj_info_list; 
    
    address           = baradress;
    tmp_data_32  = ioread32(address );
    if(tmp_data_32 == ASCII_PROJ_MAGIC_NUM ){
        bdev->startup_prj_num++;
        tmp_prj_info_list = kzalloc(sizeof(pciedev_prj_info), GFP_KERNEL);
        
        address = baradress + WORD_PROJ_ID;
        tmp_data_32       = ioread32(address);
       tmp_prj_info_list->PCIEDEV_PROJ_ID = tmp_data_32;

        address = baradress + WORD_PROJ_VERSION;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_VERSION = tmp_data_32;

        address = baradress + WORD_PROJ_DATE;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_DATE = tmp_data_32;

        address = baradress + WORD_PROJ_RESERVED;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_RESERVED = tmp_data_32;

        bdev->brd_info_list.PCIEDEV_PROJ_NEXT = 0;
        address = baradress + WORD_PROJ_NEXT;
        tmp_data_32       = ioread32(address );
       tmp_prj_info_list->PCIEDEV_PROJ_NEXT = tmp_data_32;

        list_add(&(tmp_prj_info_list->prj_list), &(bdev->prj_info_list.prj_list));
        strbrd= tmp_data_32;
    }
    
    return strbrd;
}
EXPORT_SYMBOL(pciedev_fill_prj_info);

int      pciedev_get_prjinfo(struct pciedev_dev  *bdev)
{
    void *baddress;
    void *address;
    int   strbrd             = 0;
    int  tmp_next_prj  = 0;
    int  tmp_next_prj1 = 0;
    int  i = 1;
    
    bdev->startup_prj_num = 0;
    tmp_next_prj =bdev->brd_info_list.PCIEDEV_PROJ_NEXT;
    if(tmp_next_prj){
        baddress = bdev->memmory_base[0];
        while(tmp_next_prj){
            address = baddress + tmp_next_prj;
            tmp_next_prj = pciedev_fill_prj_info(bdev, address);
        }
    }else{
        for ( i = 1 ; i < NUMBER_OF_BARS; ++i){
                if (bdev->memmory_base[i]){
                tmp_next_prj = 1;
                tmp_next_prj1 = 0;
                baddress = bdev->memmory_base[i];
                while (tmp_next_prj){
                    tmp_next_prj = tmp_next_prj1;
                    address = baddress + tmp_next_prj;
                    tmp_next_prj = pciedev_fill_prj_info(bdev, address);
                } // while (tmp_next_prj)
            } // if (bdev->memmory_base[i])
        } // for ( ; i < NUMBER_OF_BARS; ++i)
    }
    strbrd = bdev->startup_prj_num;
    return strbrd;
}
EXPORT_SYMBOL(pciedev_get_prjinfo);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void register_upciedev_proc(int num, char * dfn, struct pciedev_dev     *p_upcie_dev, struct pciedev_cdev     *p_upcie_cdev)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        p_upcie_cdev->pciedev_procdir = create_proc_entry(prc_entr, S_IFREG | S_IRUGO, 0);
        p_upcie_cdev->pciedev_procdir->read_proc = pciedev_procinfo;
        p_upcie_cdev->pciedev_procdir->data = p_upcie_dev;
    }

    void unregister_upciedev_proc(int num, char *dfn)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        remove_proc_entry(prc_entr,0);
    }

    int pciedev_procinfo(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
    {
        char *p;
        pciedev_dev     *pciedev_dev_m ;
        struct list_head *pos;
        struct pciedev_prj_info  *tmp_prj_info_list;

        printk(KERN_ALERT "UPCIEDEV_PROCINFO KERNEL < 3.10\n");
        
        pciedev_dev_m = (pciedev_dev*)data;
        p = buf;
        p += sprintf(p,"UPCIEDEV Driver Version:\t%i.%i\n", pciedev_dev_m->parent_dev->UPCIEDEV_VER_MAJ, 
                                                                                   pciedev_dev_m->parent_dev->UPCIEDEV_VER_MIN);
        p += sprintf(p,"Driver Version:\t%i.%i\n", pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MAJ, 
                                                                                   pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MIN);
        p += sprintf(p,"Board NUM:\t%i\n", pciedev_dev_m->brd_num);
        p += sprintf(p,"Slot    NUM:\t%i\n", pciedev_dev_m->slot_num);
        p += sprintf(p,"Board ID:\t%X\n", pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_ID);
        p += sprintf(p,"Board Version;\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_VERSION);
        p += sprintf(p,"Board Date:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_DATE);
        p += sprintf(p,"Board HW Ver:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_HW_VERSION);
        p += sprintf(p,"Board Next Prj:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_PROJ_NEXT);
        p += sprintf(p,"Board Reserved:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_RESERVED);
        p += sprintf(p,"Number of Proj:\t%i\n", pciedev_dev_m->startup_prj_num);

        list_for_each(pos,  &pciedev_dev_m->prj_info_list.prj_list ){
            tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
            p += sprintf(p,"Project ID:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_ID);
            p += sprintf(p,"Project Version:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_VERSION);
            p += sprintf(p,"Project Date:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_DATE);
            p += sprintf(p,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_RESERVED);
            p += sprintf(p,"Project Next:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_NEXT);
        }

        *eof = 1;
        return p - buf;
    }
#else
    void register_upciedev_proc(int num, char * dfn, struct pciedev_dev     *p_upcie_dev, struct pciedev_cdev     *p_upcie_cdev)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        p_upcie_cdev->pciedev_procdir = proc_create_data(prc_entr, S_IFREG | S_IRUGO, 0, &upciedev_proc_fops, p_upcie_dev); 
    }

    void unregister_upciedev_proc(int num, char *dfn)
    {
        char prc_entr[32];
        sprintf(prc_entr, "%ss%i", dfn, num);
        remove_proc_entry(prc_entr,0);
    }

     int pciedev_proc_open(struct inode *inode, struct file *file)
    {
         pciedev_dev     *pciedev_dev_m ;
         printk(KERN_ALERT "pciedev_proc_open CALLED 1\n");
         
         pciedev_dev_m=PDE_DATA(file_inode(file));
         return single_open(file, pciedev_proc_show, pciedev_dev_m);
    }
	
    int  pciedev_proc_show(struct seq_file *m, void *v)
   {
        pciedev_dev     *pciedev_dev_m ;
        struct list_head *pos;
        struct pciedev_prj_info  *tmp_prj_info_list;
                 
        pciedev_dev_m=(pciedev_dev *)m->private;

         seq_printf(m,
                 "UPCIEDEV Driver Version:\t%i.%i\n"
                 "Driver Version:\t%i.%i\n"
                 "Board NUM:\t%i\n"
                 "Slot    NUM:\t%i\n"
                 "Board ID:\t%X\n"
                 "Board Version;\t%X\n"
                 "Board Date:\t%X\n"
                 "Board HW Ver:\t%X\n"
                 "Board Next Prj:\t%X\n"
                 "Board Reserved:\t%X\n"
                 "Number of Proj:\t%i\n"
                 ,
                 pciedev_dev_m->parent_dev->UPCIEDEV_VER_MAJ, pciedev_dev_m->parent_dev->UPCIEDEV_VER_MIN,
                 pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MAJ, pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MIN,
                 pciedev_dev_m->brd_num,
                 pciedev_dev_m->slot_num,
                 pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_ID,
                 pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_VERSION,
                 pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_DATE,
                 pciedev_dev_m->brd_info_list.PCIEDEV_HW_VERSION,
                 pciedev_dev_m->brd_info_list.PCIEDEV_PROJ_NEXT,
                 pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_RESERVED,
                 pciedev_dev_m->startup_prj_num);
         
         list_for_each(pos,  &pciedev_dev_m->prj_info_list.prj_list ){
            tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
            seq_printf(m,"Project ID:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_ID);
            seq_printf(m,"Project ID:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_ID);
            seq_printf(m,"Project Version:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_VERSION);
            seq_printf(m,"Project Date:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_DATE);
            seq_printf(m,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_RESERVED);
            seq_printf(m,"Project Next:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_NEXT);
        }
       
/*
        list_for_each(pos,  &pciedev_dev_m->prj_info_list.prj_list ){
            tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
            p += sprintf(p,"Project ID:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_ID);
            p += sprintf(p,"Project Version:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_VERSION);
            p += sprintf(p,"Project Date:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_DATE);
            p += sprintf(p,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_RESERVED);
            p += sprintf(p,"Project Next:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_NEXT);
        }
*/
        printk(KERN_ALERT "pciedev_proc_show CALLED 3\n");
        return 0;
   }
     
   ssize_t pciedev_procinfo(struct file *filp,char *buf,size_t count,loff_t *offp)
   {
        char *p;
        //char m_p[2048];
        static char m_p[2048];
        char *m_str= "PCIEDEV_PROCINFO";
        int cnt = 0;
        pciedev_dev     *pciedev_dev_m ;
        struct list_head *pos;
        struct pciedev_prj_info  *tmp_prj_info_list;
        pciedev_dev_m=PDE_DATA(file_inode(filp));

        //p = buf;
        p = m_p;
        p += sprintf(p,"UPCIEDEV Driver Version:\t%i.%i\n", pciedev_dev_m->parent_dev->UPCIEDEV_VER_MAJ, 
                                                                                   pciedev_dev_m->parent_dev->UPCIEDEV_VER_MIN);
        p += sprintf(p,"Driver Version:\t%i.%i\n", pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MAJ, 
                                                                                   pciedev_dev_m->parent_dev->PCIEDEV_DRV_VER_MIN);
        p += sprintf(p,"Board NUM:\t%i\n", pciedev_dev_m->brd_num);
        p += sprintf(p,"Slot    NUM:\t%i\n", pciedev_dev_m->slot_num);
        p += sprintf(p,"Board ID:\t%X\n", pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_ID);
        p += sprintf(p,"Board Version;\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_VERSION);
        p += sprintf(p,"Board Date:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_DATE);
        p += sprintf(p,"Board HW Ver:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_HW_VERSION);
        p += sprintf(p,"Board Next Prj:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_PROJ_NEXT);
        p += sprintf(p,"Board Reserved:\t%X\n",pciedev_dev_m->brd_info_list.PCIEDEV_BOARD_RESERVED);
        p += sprintf(p,"Number of Proj:\t%i\n", pciedev_dev_m->startup_prj_num);

        list_for_each(pos,  &pciedev_dev_m->prj_info_list.prj_list ){
            tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
            p += sprintf(p,"Project ID:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_ID);
            p += sprintf(p,"Project Version:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_VERSION);
            p += sprintf(p,"Project Date:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_DATE);
            p += sprintf(p,"Project Reserver:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_RESERVED);
            p += sprintf(p,"Project Next:\t%X\n", tmp_prj_info_list->PCIEDEV_PROJ_NEXT);
        }
        
        //p += sprintf(p,"\0");
       *p = '\0';
        //cnt = strlen(p);
        //cnt = strlen(m_p);
        cnt = strlen(m_str);
        printk(KERN_INFO "PCIEDEV_PROC_INFO: PROC LEN%i\n", cnt);
        //copy_to_user(buf, m_p, (size_t)cnt);
        copy_to_user(buf, m_str, (size_t)cnt);
        return cnt;
}
#endif
EXPORT_SYMBOL(pciedev_procinfo);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
const struct file_operations upciedev_proc_fops = {
	.open = pciedev_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
EXPORT_SYMBOL(upciedev_proc_fops);
#endif
