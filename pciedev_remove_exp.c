/**
*Copyright 2016-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
*
*This file is part of UPCIEDEV driver.
*
*UPCIEDEV is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*UPCIEDEV is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with UPCIEDEV.  If not, see <http://www.gnu.org/licenses/>.
**/

/*
*	Author: Ludwig Petrosyan (Email: ludwig.petrosyan@desy.de)
*/


#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pciedev_ufn.h"

extern struct upciedev_base_dev base_upciedev_dev;

int pciedev_remove_exp(struct pci_dev *dev, pciedev_cdev  *pciedev_cdev_m, char *dev_name, int * brd_num)
{
     pciedev_dev                *pciedevdev = 0;
     int                    tmp_dev_num  = 0;
     int                    tmp_slot_num  = 0;
     int                    m_brdNum      = 0;
     char                f_name[64];
     char                prc_entr[64];
     int                   i;
     int                   nLock = 1;
     int                   d = 0;
     
     struct list_head *pos;
     struct list_head *npos;
     struct pciedev_prj_info  *tmp_prj_info_list;
     
    upciedev_file_list *tmp_file_list;
    struct list_head *fpos, *q;
     
     printk(KERN_ALERT "PCIEDEV_REMOVE_EXP CALLED\n");
    
    pciedevdev = dev_get_drvdata(&(dev->dev));
    if(!pciedevdev) return 0;
	
    tmp_dev_num  = pciedevdev->dev_num;
    tmp_slot_num  = pciedevdev->slot_num;
    m_brdNum      = pciedevdev->brd_num;
    *brd_num        = tmp_slot_num;
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
       for(i = 0; i < 32; ++i){
		    if((pciedevdev->enbl_irq_num >> i)&0x1){
				free_irq(pciedevdev->pci_dev_irq + i, pciedevdev);
				printk(KERN_ALERT "REMOVING IRQ NUM %i:%i\n", i, pciedevdev->pci_dev_irq + i);
		   }
       }
       if(pciedevdev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi((pciedevdev->pciedev_pci_dev));
       }
    }else{
        if(pciedevdev->msi){
           printk(KERN_ALERT "DISABLE MSI\n");
           pci_disable_msi((pciedevdev->pciedev_pci_dev));
       }
    }
	pciedevdev->parent_base_dev->dev_phys_addresses[tmp_slot_num].slot_num = 0;
	for(d = 0; d < NUMBER_OF_BARS; ++d){
		pciedevdev->parent_base_dev->dev_phys_addresses[tmp_slot_num].bars[d].res_start = 0;
		pciedevdev->parent_base_dev->dev_phys_addresses[tmp_slot_num].bars[d].res_end = 0;
		pciedevdev->parent_base_dev->dev_phys_addresses[tmp_slot_num].bars[d].res_flag = 0;
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
        
    pci_release_regions((pciedevdev->pciedev_pci_dev));
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
