#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/proc_fs.h>

#include "pciedev_ufn.h"
#include "pciedev_io.h"

int    pciedev_probe_exp(struct pci_dev *dev, const struct pci_device_id *id, 
            struct file_operations *pciedev_fops, pciedev_cdev *pciedev_cdev_p, pciedev_dev ** ppnew_device )
{
    
    int    m_brdNum   = 0;
    int     err       = 0;
    int     tmp_info = 0;
    int     i, nToAdd;
    
    u16 vendor_id;
    u16 device_id;
    u8  revision;
    u8  irq_line;
    u8  irq_pin;
    u32 res_start;
    u32 res_end;
    u32 res_flag;
    int pcie_cap;
    u32 tmp_slot_cap     = 0;
    int tmp_slot_num     = 0;
    int tmp_dev_num      = 0;
    int tmp_bus_func     = 0;
    
     int cur_mask = 0;
    u8  dev_payload;
    u32 tmp_payload_size = 0;

    u16 subvendor_id;
    u16 subdevice_id;
    u16 class_code;
    u32 tmp_devfn;
    u32 busNumber;
    u32 devNumber;
    u32 funcNumber;
    pciedev_dev   *pnew_dev;
    const char    *pdev_name = pciedev_cdev_p->pcieDevName;
    
    char f_name[64];
    char prc_entr[64];
    	
    printk(KERN_ALERT "############PCIEDEV_PROBE v1 THIS IS U_FUNCTION NAME %s\n", pdev_name );
    
    /*************************BOARD DEV INIT******************************************************/
    printk(KERN_WARNING "PCIEDEV_PROBE CALLED\n");
    
    /*setup device*/
    for(m_brdNum = 0;m_brdNum <= PCIEDEV_NR_DEVS;m_brdNum++){
        if(!pciedev_cdev_p->pciedev_dev_m[m_brdNum]->binded)
        break;
    }
    if(m_brdNum == PCIEDEV_NR_DEVS){
        printk(KERN_ALERT "PCIEDEV_PROBE  NO MORE DEVICES is %d\n", m_brdNum);
        return -1;
    }
    printk(KERN_ALERT "PCIEDEV_PROBE : BOARD NUM %i\n", m_brdNum);
    pnew_dev = pciedev_cdev_p->pciedev_dev_m[m_brdNum];
    *ppnew_device = pnew_dev;

    // flag this as taken
    pnew_dev->binded = 1;
    
    if ((err= pci_enable_device(dev)))
            return err;
    err = pci_request_regions(dev, pdev_name);
    if (err ){
        pci_disable_device(dev);
        pci_set_drvdata(dev, NULL);
        pnew_dev->binded = 0;
        return err;
    }
    pci_set_master(dev);
    
    tmp_devfn  = (u32)dev->devfn;
    busNumber  = (u32)dev->bus->number;
    devNumber  = (u32)PCI_SLOT(tmp_devfn);
    funcNumber = (u32)PCI_FUNC(tmp_devfn);
    tmp_bus_func = ((busNumber & 0xFF)<<8) + (devNumber & 0xFF);
    printk(KERN_ALERT "PCIEDEV_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X, BUS_FUNC %x\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber, tmp_bus_func);

    tmp_devfn  = (u32)dev->bus->self->devfn;
    busNumber  = (u32)dev->bus->self->bus->number;
    devNumber  = (u32)PCI_SLOT(tmp_devfn);
    funcNumber = (u32)PCI_FUNC(tmp_devfn);
    printk(KERN_ALERT "PCIEDEV_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber);
    
    pcie_cap = pci_find_capability (dev->bus->self, PCI_CAP_ID_EXP);
    printk(KERN_INFO "PCIEDEV_PROBE: PCIE SWITCH CAP address %X\n",pcie_cap);
    
    pci_read_config_dword(dev->bus->self, (pcie_cap +PCI_EXP_SLTCAP), &tmp_slot_cap);
    tmp_slot_num = (tmp_slot_cap >> 19);
    tmp_dev_num  = tmp_slot_num;
    printk(KERN_ALERT "PCIEDEV_PROBE:SLOT NUM %d DEV NUM%d SLOT_CAP %X\n",tmp_slot_num,tmp_dev_num,tmp_slot_cap);
    
    pnew_dev->swap         = 0;
    pnew_dev->slot_num  = tmp_slot_num;
    pnew_dev->bus_func  = tmp_bus_func;
    pnew_dev->pciedev_pci_dev = dev;

    dev_set_drvdata(&(dev->dev), pnew_dev );
    
    pcie_cap = pci_find_capability (dev, PCI_CAP_ID_EXP);
    printk(KERN_ALERT "DAMC_PROBE: PCIE CAP address %X\n",pcie_cap);
    pci_read_config_byte(dev, (pcie_cap +PCI_EXP_DEVCAP), &dev_payload);
    dev_payload &=0x0003;
    printk(KERN_ALERT "DAMC_PROBE: DEVICE CAP  %X\n",dev_payload);

    switch(dev_payload){
        case 0:
                   tmp_payload_size = 128;
                   break;
        case 1:
                   tmp_payload_size = 256;
                   break;
        case 2:
                   tmp_payload_size = 512;
                   break;
        case 3:
                   tmp_payload_size = 1024;	
                   break;
        case 4:
                   tmp_payload_size = 2048;	
                   break;
        case 5:
                   tmp_payload_size = 4096;	
                   break;
    }
    printk(KERN_ALERT "DAMC: DEVICE PAYLOAD  %d\n",tmp_payload_size);
    

    if (!(cur_mask = pci_set_dma_mask(dev, DMA_BIT_MASK(64))) &&
        !(cur_mask = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64)))) {
             pnew_dev->dev_dma_64mask = 1;
            printk(KERN_ALERT "CURRENT 64MASK %i\n", cur_mask);
    } else {
            if ((err = pci_set_dma_mask(dev, DMA_BIT_MASK(32))) &&
                (err = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32)))) {
                    printk(KERN_ALERT "No usable DMA configuration\n");
            }else{
             pnew_dev->dev_dma_64mask = 0;
            printk(KERN_ALERT "CURRENT 32MASK %i\n", cur_mask);
            }
    }    
    
    pci_read_config_word(dev, PCI_VENDOR_ID,   &vendor_id);
    pci_read_config_word(dev, PCI_DEVICE_ID,   &device_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID,   &subvendor_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_ID,   &subdevice_id);
    pci_read_config_word(dev, PCI_CLASS_DEVICE,   &class_code);
    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq_line);
    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq_pin);
    
    printk(KERN_INFO "PCIEDEV_PROBE: VENDOR_ID  %i\n",vendor_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_DEVICE_ID  %i\n",device_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_SUBSYSTEM_VENDOR_ID  %i\n",subvendor_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_SUBSYSTEM_ID  %i\n",subdevice_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_CLASS_DEVICE  %i\n",class_code);

     pnew_dev->vendor_id      = vendor_id;
     pnew_dev->device_id      = device_id;
     pnew_dev->subvendor_id   = subvendor_id;
     pnew_dev->subdevice_id   = subdevice_id;
     pnew_dev->class_code     = class_code;
     pnew_dev->revision       = revision;
     pnew_dev->irq_line       = irq_line;
     pnew_dev->irq_pin        = irq_pin;
     pnew_dev->scratch_bar    = 0;
     pnew_dev->scratch_offset = 0;
    
    /*******SETUP BARs******/
    pnew_dev->pciedev_all_mems = 0;
    for (i = 0, nToAdd = 1; i < NUMBER_OF_BARS; ++i, nToAdd *= 2)
        {
            res_start = pci_resource_start(dev, i);
            res_end = pci_resource_end(dev, i);
            res_flag = pci_resource_flags(dev, i);
            pnew_dev->mem_base[i] = res_start;
            pnew_dev->mem_base_end[i]   = res_end;
            pnew_dev->mem_base_flag[i]  = res_flag;
            if(res_start){
                pnew_dev->memmory_base[i] = pci_iomap(dev, i, (res_end - res_start));
                printk(KERN_INFO "PCIEDEV_PROBE: mem_region %d address %X  SIZE %X FLAG %X\n",
                i,res_start, (res_end - res_start),
                pnew_dev->mem_base_flag[i]);
                pnew_dev->rw_off[i] = (res_end - res_start);
                pnew_dev->pciedev_all_mems += nToAdd;
            }
            else{
                pnew_dev->memmory_base[i] = 0;
                pnew_dev->rw_off[i]       = 0;
                printk(KERN_INFO "PCIEDEV: NO BASE%i address\n", i);
            }
        }

    /******GET BRD INFO******/
    tmp_info = pciedev_get_brdinfo(pnew_dev);
    printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  IS STARTUP BOARD %i\n", tmp_info);
    if(tmp_info){
        tmp_info = pciedev_get_prjinfo(pnew_dev);
        printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  NUMBER OF PRJs %i\n", tmp_info);
    }
    /*******PREPARE INTERRUPTS******/
    
    // pnew_dev->irq_flag = IRQF_SHARED | IRQF_DISABLED;
     pnew_dev->irq_flag = IRQF_SHARED ;
    #ifdef CONFIG_PCI_MSI
    if (pci_enable_msi(dev) == 0) {
             pnew_dev->msi = 1;
             pnew_dev->irq_flag &= ~IRQF_SHARED;
            printk(KERN_ALERT "MSI ENABLED\n");
    } else {
             pnew_dev->msi = 0;
            printk(KERN_ALERT "MSI NOT SUPPORTED\n");
    }
    #endif
     pnew_dev->pci_dev_irq = dev->irq;
     pnew_dev->irq_mode = 0;
    
    /* Send uvents to udev, so it'll create /dev nodes */
    if( pnew_dev->dev_sts){
        pnew_dev->dev_sts   = 0;
        device_destroy(pciedev_cdev_p->pciedev_class,  MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR, 
                                                  pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum));
    }
    pnew_dev->dev_sts   = 1;
    sprintf(pnew_dev->dev_name, "%ss%d", pdev_name, tmp_slot_num);
    sprintf(f_name, "%ss%d", pdev_name, tmp_slot_num);
    sprintf(prc_entr, "%ss%d", pdev_name, tmp_slot_num);
    printk(KERN_INFO "PCIEDEV_PROBE:  CREAT DEVICE MAJOR %i MINOR %i F_NAME %s DEV_NAME %s\n",
               pciedev_cdev_p->PCIEDEV_MAJOR, (pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum), f_name, pnew_dev->dev_name);
    
    device_create(pciedev_cdev_p->pciedev_class, NULL, MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR,  pnew_dev->dev_minor),
                            & pnew_dev->pciedev_pci_dev->dev, f_name, pnew_dev->dev_name);

    
    register_upciedev_proc(tmp_slot_num, pdev_name, pnew_dev, pciedev_cdev_p);
    pnew_dev->register_size = RW_D32;
	
    pciedev_cdev_p->pciedevModuleNum ++;
    return 0;
}
EXPORT_SYMBOL(pciedev_probe_exp);