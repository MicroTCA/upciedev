#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pciedev_ufn.h"

int pciedev_remove_exp(pciedev_dev *pciedevdev)
{
    struct pci_dev *dev;
    pciedev_cdev   *pciedev_cdev_m;
    const char     *dev_name;
    int             tmp_dev_num  = 0;
    int             tmp_slot_num  = 0;
    int             m_brdNum      = 0;
    char            f_name[64];
    char            prc_entr[64];
    int             i;
    int             nLock = 1;
     
    struct list_head *pos;
    struct list_head *npos;
    struct pciedev_prj_info  *tmp_prj_info_list;
     
    upciedev_file_list *tmp_file_list;
    struct list_head *fpos, *q;

    printk(KERN_ALERT "PCIEDEV_REMOVE_EXP CALLED\n");
    
    dev            = pciedevdev->pciedev_pci_dev;
    pciedev_cdev_m = pciedevdev->parent_dev;
    dev_name       = pciedev_cdev_m->pcieDevName;

    tmp_dev_num   = pciedevdev->dev_num;
    tmp_slot_num  = pciedevdev->slot_num;
    m_brdNum      = pciedevdev->brd_num;
    sprintf(f_name, "%ss%d", dev_name, tmp_slot_num);
    sprintf(prc_entr, "%ss%d", dev_name, tmp_slot_num);
    printk(KERN_ALERT "PCIEDEV_REMOVE: SLOT %d DEV %d BOARD %i\n", tmp_slot_num, tmp_dev_num, m_brdNum);
    
    /* now let's be good and free the proj_info_list items. since we will be removing items
     * off the list using list_del() we need to use a safer version of the list_for_each() 
     * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
     * involves deletions of items (or moving items from one list to another).
     */
    list_for_each_safe(pos,  npos, &pciedevdev->prj_info_list.prj_list ){
        tmp_prj_info_list = list_entry(pos, struct pciedev_prj_info, prj_list);
        list_del(pos);
        kfree(tmp_prj_info_list);
    }
    
    printk(KERN_ALERT "REMOVING IRQ_MODE %d\n", pciedevdev->irq_mode);
    if(pciedevdev->irq_mode){
       printk(KERN_ALERT "FREE IRQ\n");
       free_irq(pciedevdev->pci_dev_irq, pciedevdev);
       printk(KERN_ALERT "REMOVING IRQ\n");
       if(pciedevdev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi(dev);
       }
    }else{
        if(pciedevdev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi(dev);
       }
    }
     
    printk(KERN_ALERT "REMOVE: UNMAPPING MEMORYs\n");
    //mutex_lock(&pciedevdev->dev_mut);
    while (nLock){ nLock = EnterCritRegion(&pciedevdev->dev_mut); }
    
    for (i = 0; i < NUMBER_OF_BARS; ++i){
        if (pciedevdev->memmory_base[i]){
            pci_iounmap(dev, pciedevdev->memmory_base[i]);
            pciedevdev->memmory_base[i] = 0;
            pciedevdev->mem_base[i] = 0;
            pciedevdev->mem_base_end[i] = 0;
            pciedevdev->mem_base_flag[i] = 0;
            pciedevdev->rw_off[i] = 0;
        }
    }
        
    pci_release_regions(dev);
    printk(KERN_INFO "PCIEDEV_REMOVE:  DESTROY DEVICE MAJOR %i MINOR %i\n",
               pciedev_cdev_m->PCIEDEV_MAJOR, (pciedev_cdev_m->PCIEDEV_MINOR + pciedevdev->brd_num));
    device_destroy(pciedev_cdev_m->pciedev_class,  MKDEV(pciedev_cdev_m->PCIEDEV_MAJOR, 
                                                  pciedev_cdev_m->PCIEDEV_MINOR + pciedevdev->brd_num));
    
    unregister_upciedev_proc(tmp_slot_num, dev_name);
    
    pciedevdev->dev_sts   = 0;
    pciedevdev->binded   = 0;
    pciedev_cdev_m->pciedevModuleNum --;
    pci_disable_device(dev);
    
    list_for_each_safe(fpos, q, &(pciedevdev->dev_file_list.node_file_list)){
             tmp_file_list = list_entry(fpos, upciedev_file_list, node_file_list);
             tmp_file_list->filp->private_data  = pciedev_cdev_m->pciedev_dev_m[PCIEDEV_NR_DEVS]; 
             list_del(fpos);
             kfree (tmp_file_list);
             pciedev_cdev_m->pciedev_dev_m[PCIEDEV_NR_DEVS]->dev_file_ref++;
    }
    
    //mutex_unlock(&pciedevdev->dev_mut);
    LeaveCritRegion(&pciedevdev->dev_mut);
    return 0;
}
EXPORT_SYMBOL(pciedev_remove_exp);
