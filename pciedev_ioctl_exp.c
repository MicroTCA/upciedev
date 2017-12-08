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

/*
*	Author: Ludwig Petrosyan (Email: ludwig.petrosyan@desy.de)
*/


#include <linux/module.h>
#include <linux/fs.h>	
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "pciedev_ufn.h"
#include "pciedev_io.h"
#include "read_write_inline.h"


long     pciedev_ioctl_exp(struct file *filp, unsigned int *cmd_p, unsigned long *arg_p, pciedev_cdev * pciedev_cdev_m)
{
	unsigned int    cmd;
	unsigned long  arg;
	pid_t                cur_proc = 0;
	int                    minor    = 0;
	int                    d_num    = 0;
	int                    retval   = 0;
	int                    err      = 0;
	int			 nUserValue;
	struct pci_dev* pdev;
	u_int                tmp_offset;
	u_int                tmp_data;
	u_int                tmp_cmd;
	u_int                tmp_reserved;
	int                   io_size;

	void                *address ;    
	u8                  tmp_data_8;
	u16                tmp_data_16;
	u32                tmp_data_32;
	
	picmg_shapi_device_info      shapi_dev_info;
	picmg_shapi_module_info    shapi_mod_info;
	struct shapi_module_info  *tmp_prj_info_list; 
	struct list_head *pos;

	device_ioc_rw  aIocRW;
	device_vector_rw	aVectorRW;
	device_ioc_rw* __user	pUserRWdata;
	int i, nMaxNumRW;
	const size_t unSizeOfRW = sizeof(device_ioc_rw);
	
	device_ioctrl_data  data;
	struct pciedev_dev       *dev  = filp->private_data;

	cmd              = *cmd_p;
	arg                = *arg_p;
	io_size           = sizeof(device_ioctrl_data);
	minor           = dev->dev_minor;
	d_num         = dev->dev_num;	
	cur_proc     = current->group_leader->pid;
	pdev            = (dev->pciedev_pci_dev);

	if(!dev->dev_sts){
		printk("PCIEDEV_IOCTRL: NO DEVICE %d\n", dev->dev_num);
		retval = -EFAULT;
		return retval;
	}
	
	DEBUGNEW("\n");
        
	/*
	* the direction is a bitmask, and VERIFY_WRITE catches R/W
	* transfers. `Type' is user-oriented, while
	* access_ok is kernel-oriented, so the concept of "read" and
	* "write" is reversed
	*/
	if (_IOC_DIR(cmd) & _IOC_WRITE)
	{
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
		if (err){ ERRCT("Bad pointer!\n"); }
	}
	else if (_IOC_DIR(cmd) & _IOC_READ)
	{
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
		if (err){ ERRCT("Bad pointer!\n"); }
	}
	if (err) return -EFAULT;

	//if (mutex_lock_interruptible(&dev->dev_mut))return -ERESTARTSYS;
	//if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; } /// new  // removed to the cases
   
	switch (cmd) {
		case PCIEDEV_PHYSICAL_SLOT:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset;
			tmp_data     = data.data;
			tmp_cmd      = data.cmd;
			tmp_reserved = data.reserved;
			data.data    = dev->slot_num;
			data.cmd     = PCIEDEV_PHYSICAL_SLOT;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
			return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
		case PCIEDEV_DRIVER_VERSION:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			data.data   =  pciedev_cdev_m->PCIEDEV_DRV_VER_MAJ;
			data.offset =  pciedev_cdev_m->PCIEDEV_DRV_VER_MIN;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
		case PCIEDEV_UDRIVER_VERSION:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			data.data   =  pciedev_cdev_m->UPCIEDEV_VER_MAJ;
			data.offset =  pciedev_cdev_m->UPCIEDEV_VER_MIN;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
		case PCIEDEV_FIRMWARE_VERSION:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			data.data = dev->brd_info_list.PCIEDEV_BOARD_VERSION;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
		case PCIEDEV_GET_STATUS:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			data.data   = dev->dev_sts;
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
		case PCIEDEV_SCRATCH_REG:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			retval = 0;
			if (copy_from_user(&data, (device_ioctrl_data*)arg, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = data.offset;
			tmp_data     = data.data;
			tmp_cmd      = data.cmd;
			if(tmp_cmd == 1){
				data.data   = dev->scratch_bar;
				data.offset = dev->scratch_offset;
			}
			if(tmp_cmd == 0){
				dev->scratch_bar    = data.data;
				dev->scratch_offset = data.offset;
			}
			if(tmp_cmd == 2){
				address = pciedev_get_baraddress(dev->scratch_bar, dev);
				if (!address){
					retval = -EFAULT;
					//mutex_unlock(&dev->dev_mut);
					LeaveCritRegion(&dev->dev_mut);
					return retval;
				}

				switch(dev->register_size){
					case RW_D8:
						tmp_data_8        = ioread8(address + dev->scratch_offset);
						data.data = tmp_data_8;
						break;
					case RW_D16:
						tmp_data_16        = ioread16(address + dev->scratch_offset);
						data.data = tmp_data_16;
						break;
					case RW_D32:
						tmp_data_32        = ioread32(address + dev->scratch_offset);
						data.data = tmp_data_32;
						break;
				} 
			}
			if (copy_to_user((device_ioctrl_data*)arg, &data, (size_t)io_size)) {
				retval = -EFAULT;
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
		case PCIEDEV_SET_SWAP:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			if (copy_from_user(&nUserValue, (int*)arg, sizeof(int))) {
				//mutex_unlock(&dev->dev_mut);
				LeaveCritRegion(&dev->dev_mut); /// new
				return -EFAULT;
			}
			dev->swap = nUserValue ? 1 : 0;
			LeaveCritRegion(&dev->dev_mut);
			break;

		case PCIEDEV_GET_SWAP:
			return dev->swap;
			
		case PCIEDEV_LOCK_DEVICE:
			retval = LongLockOfCriticalRegion(&dev->dev_mut);
			break;

		case PCIEDEV_UNLOCK_DEVICE:
			retval = UnlockLongCritRegion(&dev->dev_mut);
			break;

		case PCIEDEV_SET_BITS:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			if (copy_from_user(&aIocRW, (device_ioc_rw*)arg, sizeof(device_ioc_rw))){
				LeaveCritRegion(&dev->dev_mut);
				return -EFAULT;
			}
			//aIocRW.mode_rw = W_BITS_INITIAL + (aIocRW.mode_rw&_STEP_FOR_NEXT_MODE2_);
			CORRECT_REGISTER_SIZE(aIocRW.register_size, dev->register_size);
			retval = pciedev_write_inline(dev, aIocRW.register_size, MTCA_SET_BITS, aIocRW.barx_rw, aIocRW.offset_rw,
								  (const char*)aIocRW.dataPtr, (const char*)aIocRW.maskPtr, aIocRW.count_rw);
			LeaveCritRegion(&dev->dev_mut);
			break;

		case PCIEDEV_SWAP_BITS:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			if (copy_from_user(&aIocRW, (device_ioc_rw*)arg, sizeof(device_ioc_rw))){
				LeaveCritRegion(&dev->dev_mut);
				return -EFAULT;
			}
			CORRECT_REGISTER_SIZE(aIocRW.register_size, dev->register_size);
			//ALERTCT("!!!!!!!!!!!!!!!! mode=%s\n", ModeString(aIocRW.register_size));
			retval = pciedev_write_inline(dev, aIocRW.register_size, MTCA_SWAP_BITS,aIocRW.barx_rw, aIocRW.offset_rw,
																						   NULL, (const char*)aIocRW.maskPtr, aIocRW.count_rw);
			LeaveCritRegion(&dev->dev_mut);
			break;
		
		case PCIEDEV_LOCKED_READ:
			if (copy_from_user(&aIocRW, (device_ioc_rw*)arg, unSizeOfRW)){ return -EFAULT; }
			CORRECT_REGISTER_SIZE(aIocRW.register_size, dev->register_size);
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			retval = pciedev_read_inline(dev, aIocRW.register_size, MTCA_LOCKED_READ,aIocRW.barx_rw, 
					                                   aIocRW.offset_rw, (char*)aIocRW.dataPtr, aIocRW.count_rw);
			LeaveCritRegion(&dev->dev_mut);
			break;
		
		case PCIEDEV_VECTOR_RW:
			if (copy_from_user(&aVectorRW, (device_vector_rw*)arg, sizeof(device_vector_rw))){return -EFAULT;}
			nMaxNumRW = (int)aVectorRW.number_of_rw;
			pUserRWdata = (device_ioc_rw* __user)((void*)aVectorRW.device_ioc_rw_ptr);
			retval = 0;

			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }

			for (i = 0; (i < nMaxNumRW) && (!copy_from_user(&aIocRW, pUserRWdata, unSizeOfRW)); ++i, 
					                                                                       pUserRWdata += 1)
			{
				CORRECT_REGISTER_SIZE(aIocRW.register_size, dev->register_size);
				if (aIocRW.rw_access_mode<MTCA_SIMPLE_WRITE)
				{
					
				retval += pciedev_read_inline(dev, aIocRW.register_size, aIocRW.rw_access_mode,
				      aIocRW.barx_rw,aIocRW.offset_rw, (char*)aIocRW.dataPtr, aIocRW.count_rw);
				}
				else
				{
					/*
					ALERTCT("###Vector Write Mode %i BAR %i Offset %i Count %i\n",
                                    	     aIocRW.register_size, aIocRW.barx_rw, aIocRW.offset_rw,
				 	     aIocRW.count_rw);
                                        
                                        for(i = 0; i < 10; i++){
						ALERTCT("###DATA %X\n", (int)*(((int*)aIocRW.dataPtr) + i));
					}
					*/

					retval += pciedev_write_inline(dev, aIocRW.register_size,
                                                           aIocRW.rw_access_mode, aIocRW.barx_rw, 
					            aIocRW.offset_rw,(const char*)aIocRW.dataPtr, 
                                                    (const char*)aIocRW.maskPtr, aIocRW.count_rw);
				}
			}
			LeaveCritRegion(&dev->dev_mut);
			break;	
		
		case PCIEDEV_GET_REGISTER_SIZE:
			DEBUGNEW("!!!PCIEDEV_GET_REGISTER_SIZE reg_size=%d\n", (int)dev->register_size);
			return dev->register_size;
			break;

		case PCIEDEV_SET_REGISTER_SIZE:
			if (copy_from_user(&nUserValue, (int*)arg, sizeof(int))) {
				return -EFAULT;
			}
			CORRECT_REGISTER_SIZE(nUserValue, RW_D32);
			dev->register_size = nUserValue;
			break;
		case PCIEDEV_SINGLE_IOC_ACCESS:
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			if (copy_from_user(&aIocRW, (device_ioc_rw*)arg, sizeof(device_ioc_rw)))
			{
				LeaveCritRegion(&dev->dev_mut);
				return -EFAULT;
			}
			CORRECT_REGISTER_SIZE(aIocRW.register_size, dev->register_size);
			if (aIocRW.rw_access_mode<MTCA_SIMPLE_WRITE)
			{
				if (aIocRW.rw_access_mode == MTCA_LOCKED_READ){ if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; } }
				retval += pciedev_read_inline(dev, aIocRW.register_size, aIocRW.rw_access_mode,
					aIocRW.barx_rw, aIocRW.offset_rw, (char*)aIocRW.dataPtr, aIocRW.count_rw);
				if (aIocRW.rw_access_mode == MTCA_LOCKED_READ){ LeaveCritRegion(&dev->dev_mut); }
			}
			else
			{
				if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
				retval += pciedev_write_inline(dev, aIocRW.register_size, aIocRW.rw_access_mode, aIocRW.barx_rw,
					aIocRW.offset_rw, (const char*)aIocRW.dataPtr, (const char*)aIocRW.maskPtr, aIocRW.count_rw);
				LeaveCritRegion(&dev->dev_mut);
			}
			break;
		case PCIEDEV_GET_SHAPI_DEVINFO: //picmg_shapi_device_info      shapi_dev_info;
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			retval = 0;
			
			shapi_dev_info.SHAPI_VERSION = dev->device_info_list.SHAPI_VERSION;
			shapi_dev_info.SHAPI_FIRST_MODULE_ADDRESS = dev->device_info_list.SHAPI_FIRST_MODULE_ADDRESS;
			shapi_dev_info.SHAPI_HW_IDS = dev->device_info_list.SHAPI_FW_IDS; 
			shapi_dev_info.SHAPI_FW_IDS  = dev->device_info_list.SHAPI_FW_IDS;
			shapi_dev_info.SHAPI_FW_VERSION  = dev->device_info_list.SHAPI_FW_VERSION;
			shapi_dev_info.SHAPI_FW_TIMESTAMP  = dev->device_info_list.SHAPI_FW_TIMESTAMP;
			shapi_dev_info.SHAPI_FW_NAME[0] = dev->device_info_list.SHAPI_FW_NAME[0];
			shapi_dev_info.SHAPI_FW_NAME[1] = dev->device_info_list.SHAPI_FW_NAME[1];
			shapi_dev_info.SHAPI_FW_NAME[2] == dev->device_info_list.SHAPI_FW_NAME[2];
			shapi_dev_info.SHAPI_DEVICE_CAP = dev->device_info_list.SHAPI_DEVICE_CAP;
			shapi_dev_info.SHAPI_DEVICE_STATUS = dev->device_info_list.SHAPI_DEVICE_STATUS; 
			shapi_dev_info.SHAPI_DEVICE_CONTROL  = dev->device_info_list.SHAPI_DEVICE_CONTROL;
			shapi_dev_info.SHAPI_IRQ_MASK  = dev->device_info_list.SHAPI_IRQ_MASK;
			shapi_dev_info.SHAPI_IRQ_FLAG  = dev->device_info_list.SHAPI_IRQ_FLAG;
			shapi_dev_info.SHAPI_IRQ_ACTIVE = dev->device_info_list.SHAPI_IRQ_ACTIVE;
			shapi_dev_info.SHAPI_SCRATCH_REGISTER = dev->device_info_list.SHAPI_SCRATCH_REGISTER;
			strncpy(shapi_dev_info.fw_name, dev->device_info_list.fw_name, 13);
			shapi_dev_info.number_of_modules = dev->shapi_module_num;
			
			if (copy_to_user((picmg_shapi_device_info*)arg, &shapi_dev_info, (size_t)sizeof(picmg_shapi_device_info))) {
				retval = -EFAULT;
				LeaveCritRegion(&dev->dev_mut);
			return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;

		case PCIEDEV_GET_SHAPI_MODINFO: //picmg_shapi_module_info    shapi_module_info;
			if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }
			retval = 0;
			if (copy_from_user(&shapi_mod_info, (picmg_shapi_module_info*)arg, (size_t)sizeof(picmg_shapi_module_info))) {
				retval = -EFAULT;
				LeaveCritRegion(&dev->dev_mut);
				return retval;
			}
			tmp_offset   = shapi_mod_info.module_num;
			printk(KERN_INFO "PCIEDEV_GET_SHAPI_MODINFO:: MOD_NUM %i\n",  tmp_offset);
			list_for_each(pos,  &dev->module_info_list.module_list ){
				tmp_prj_info_list = list_entry(pos, struct shapi_module_info, module_list);
				if(tmp_prj_info_list->module_num == tmp_offset){
					printk(KERN_INFO "PCIEDEV_GET_SHAPI_MODINFO:: SHAPI MOD_NUM %i\n",  tmp_prj_info_list->module_num);
					shapi_mod_info.SHAPI_VERSION = tmp_prj_info_list->SHAPI_VERSION;
					shapi_mod_info.SHAPI_NEXT_MODULE_ADDRESS = tmp_prj_info_list->SHAPI_NEXT_MODULE_ADDRESS;
					shapi_mod_info.SHAPI_MODULE_FW_IDS = tmp_prj_info_list->SHAPI_MODULE_FW_IDS; 
					shapi_mod_info.SHAPI_MODULE_VERSION  = tmp_prj_info_list->SHAPI_MODULE_VERSION;
					shapi_mod_info.SHAPI_MODULE_NAME[0] = tmp_prj_info_list->SHAPI_MODULE_NAME[0];
					shapi_mod_info.SHAPI_MODULE_NAME[1] = tmp_prj_info_list->SHAPI_MODULE_NAME[1];
					shapi_mod_info.SHAPI_MODULE_CAP = tmp_prj_info_list->SHAPI_MODULE_CAP ;
					shapi_mod_info.SHAPI_MODULE_STATUS = tmp_prj_info_list->SHAPI_MODULE_STATUS;
					shapi_mod_info.SHAPI_MODULE_CONTROL = tmp_prj_info_list->SHAPI_MODULE_CONTROL; 
					shapi_mod_info.SHAPI_IRQ_ID  = tmp_prj_info_list->SHAPI_IRQ_ID;
					shapi_mod_info.SHAPI_IRQ_FLAG_CLEAR  = tmp_prj_info_list->SHAPI_IRQ_FLAG_CLEAR;
					shapi_mod_info.SHAPI_IRQ_MASK  = tmp_prj_info_list->SHAPI_IRQ_MASK;
					shapi_mod_info.SHAPI_IRQ_FLAG = tmp_prj_info_list->SHAPI_IRQ_FLAG;
					shapi_mod_info.SHAPI_IRQ_ACTIVE = tmp_prj_info_list->SHAPI_IRQ_ACTIVE;
					strncpy(shapi_mod_info.module_name, tmp_prj_info_list->module_name, 9);
					printk(KERN_INFO "PCIEDEV_GET_SHAPI_MODINFO:: SHAPI DEV_INFO NAME %s\n",  
					                    shapi_mod_info.module_name, tmp_prj_info_list->module_name);
					shapi_mod_info.module_num = tmp_offset ;
				}
			}
			
			if (copy_to_user((picmg_shapi_module_info*)arg, &shapi_mod_info, (size_t)sizeof(picmg_shapi_module_info))) {
				retval = -EFAULT;
				LeaveCritRegion(&dev->dev_mut);
			return retval;
			}
			LeaveCritRegion(&dev->dev_mut);
			break;
			
		default:
			return -ENOTTY;
			break;
	}
	//mutex_unlock(&dev->dev_mut);
	//LeaveCritRegion(&dev->dev_mut); /// new  // removed to the cases
	return retval;
    
}
EXPORT_SYMBOL(pciedev_ioctl_exp);
