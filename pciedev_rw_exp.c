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
//#include <asm/uaccess.h>

#include "pciedev_ufn.h"
#include "read_write_inline.h"
#include "debug_functions.h"

#ifdef DEBUG_TIMING
#define NSEC_FROM_TIMESPEC(_a_timespec_,_a_timespec_end_)	\
	((u_int64_t)((u_int64_t)((_a_timespec_end_).tv_sec-(_a_timespec_).tv_sec)*1000000000L+\
	(u_int64_t)((_a_timespec_end_).tv_nsec - (_a_timespec_).tv_nsec)))
#endif

ssize_t pciedev_read_exp(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t    retval         = 0;
    //struct pciedev_dev *dev = filp->private_data;
    struct file_data* file_data_p = filp->private_data;
    struct pciedev_dev *dev = file_data_p->pciedev_p;

    if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }

    if (dev->hot_plug_events_counter != file_data_p->hot_plug_number_file_openned){
        LeaveCritRegion(&dev->dev_mut);
		ERRCT("ERROR: NO DEVICE %d\n", dev->dev_num);
		return -ENODEV;
	}

	/*Check if it is pread call*/
	if (*f_pos != PCIED_FPOS)
	{
		//printk(KERN_ALERT "PCIEDEV_READ_EXP:PREAD called POS IS 7\n");
		u_int64_t   llnPos = (u_int64_t)f_pos;
		u_int32_t	reg_size = (llnPos & PRW_REG_SIZE_MASK) >> PRW_REG_SIZE_SHIFT;
		u_int32_t	barx_rw = (llnPos & PRW_BAR_MASK) >> PRW_BAR_SHIFT;
		u_int32_t	offset_rw = (llnPos & PRW_OFFSET_MASK);

		CORRECT_REGISTER_SIZE(reg_size, dev->register_size);
		retval = pciedev_read_inline(dev, reg_size, MTCA_SIMPLE_READ, barx_rw, offset_rw, buf, count);
		retval = sizeof(device_rw);
	}else{
		device_rw			reading;
		char* __user		pcUserBuffer; // Temporal buffer from user space to store value

		if (copy_from_user(&reading, buf, sizeof(device_rw))) { return -EIO; }
		pcUserBuffer = (char*)((void*)&((device_rw*)buf)->data_rw); 
		count = 1; 
		
		CORRECT_REGISTER_SIZE(reading.register_size, dev->register_size);
		retval = pciedev_read_inline(dev,reading.register_size,MTCA_SIMPLE_READ,reading.barx_rw,
				                                                                  reading.offset_rw,pcUserBuffer, count);
		retval = sizeof(device_rw);
	}
	
	LeaveCritRegion(&dev->dev_mut);

	retval = sizeof(device_rw); // read/write returns sizeof(device_rw) or error
	return retval;
}
EXPORT_SYMBOL(pciedev_read_exp);

ssize_t pciedev_write_exp(struct file *filp, const char __user *a_buf, size_t count, loff_t *a_f_pos)
{
	ssize_t				retval	= 0;
    //struct pciedev_dev*	dev		= a_filp->private_data;
    struct file_data* file_data_p = filp->private_data;
    struct pciedev_dev *dev = file_data_p->pciedev_p;

    if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }

    if (dev->hot_plug_events_counter != file_data_p->hot_plug_number_file_openned){
        LeaveCritRegion(&dev->dev_mut);
        ERRCT("ERROR: NO DEVICE %d\n", dev->dev_num);
        (void)base_upciedev_dev;
        return -ENODEV;
    }


	/*Check if it is pwrite call*/
	if (*a_f_pos != PCIED_FPOS)
	{
		u_int64_t	llnPos = (u_int64_t)a_f_pos;
		u_int32_t	reg_size   = (llnPos & PRW_REG_SIZE_MASK) >> PRW_REG_SIZE_SHIFT;
		u_int32_t	barx_rw   = (llnPos & PRW_BAR_MASK) >> PRW_BAR_SHIFT;
		u_int32_t	offset_rw = (llnPos & PRW_OFFSET_MASK);

		CORRECT_REGISTER_SIZE(reg_size, dev->register_size);
		retval = pciedev_write_inline(dev, reg_size, MTCA_SIMPLE_WRITE, barx_rw, offset_rw, a_buf, NULL, count);
		retval = sizeof(device_rw);
	}
	else
	{
		device_rw			reading;
		const char* __user	pcUserBuffer; // Temporal buffer from user space to read value

		if (copy_from_user(&reading, a_buf, sizeof(device_rw))) {return -EFAULT;}
		pcUserBuffer = (const char*)((const void*)&((device_rw*)a_buf)->data_rw); 
		count = 1;
		CORRECT_REGISTER_SIZE(reading.register_size, dev->register_size);
		retval = pciedev_write_inline(dev, reading.register_size, MTCA_SIMPLE_WRITE,reading.barx_rw, 
				                                                          reading.offset_rw, pcUserBuffer, NULL, count);
		retval = sizeof(device_rw);
	}

	LeaveCritRegion(&dev->dev_mut);

	retval = sizeof(device_rw); // read/write returns sizeof(device_rw) or error
	return retval;
}
EXPORT_SYMBOL(pciedev_write_exp);

loff_t    pciedev_llseek_exp(struct file *filp, loff_t off, int frm)
{
    struct file_data* file_data_p = filp->private_data;
    struct pciedev_dev *dev = file_data_p->pciedev_p;

    if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; }

    if (dev->hot_plug_events_counter != file_data_p->hot_plug_number_file_openned){
        LeaveCritRegion(&dev->dev_mut);
        ERRCT("ERROR: NO DEVICE %d\n", dev->dev_num);
        return -ENODEV;
    }


    filp->f_pos = (loff_t)PCIED_FPOS;
    LeaveCritRegion(&dev->dev_mut);
    return (loff_t)PCIED_FPOS;
}
EXPORT_SYMBOL(pciedev_llseek_exp);
