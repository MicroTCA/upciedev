#include <linux/module.h>
#include <linux/fs.h>	
#include <asm/uaccess.h>

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
    struct pciedev_dev *dev = filp->private_data;
	
    #ifdef DEBUG_TIMING
	static int snIterations = 0;
	static u_int64_t	sullnNanoseconds = 0;
	u_int64_t	ullnCurrent;
	struct timespec	aTvEnd, aTvBeg;
	getnstimeofday(&aTvBeg);
   #endif

	if (!dev->dev_sts)
	{
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
	}
	else
	{
		device_rw			reading;
		char* __user		pcUserBuffer; // Temporal buffer from user space to store value

		if (copy_from_user(&reading, buf, sizeof(device_rw))) { return -EIO; }
		
		// For compatibility in the case of count=1 data read from register
		// will be stored in the field 'data_rw'
			//if (count < 2){ pcUserBuffer = (char*)((void*)&((device_rw*)buf)->data_rw); count = 1; }
			//else { pcUserBuffer = (char*)reading.dataPtr; }
		//read used only for single read, count has to be 1
		pcUserBuffer = (char*)((void*)&((device_rw*)buf)->data_rw); 
		count = 1; 
		
		CORRECT_REGISTER_SIZE(reading.register_size, dev->register_size);
		retval = pciedev_read_inline(dev,reading.register_size,MTCA_SIMPLE_READ,reading.barx_rw,
				                                                                  reading.offset_rw,pcUserBuffer, count);
	}

#ifdef DEBUG_TIMING
#ifdef _SMB_RMB_
	_SMB_RMB_();
#endif
	getnstimeofday(&aTvEnd);
	snIterations++;
	ullnCurrent = NSEC_FROM_TIMESPEC(aTvBeg, aTvEnd);
	sullnNanoseconds += ullnCurrent;
	if (g_nPrintDebugInfo){
		printk(KERN_ALERT "READ(%d):   iter=%d, current=%llu, totanNanosec=%llu\n",
			(int)count, snIterations, ullnCurrent, sullnNanoseconds);
	}
#endif

    retval = sizeof(device_rw); // read/write returns sizeof(device_rw) or error
    return retval;
}
EXPORT_SYMBOL(pciedev_read_exp);

ssize_t pciedev_write_exp(struct file *a_filp, const char __user *a_buf, size_t count, loff_t *a_f_pos)
{
    ssize_t				retval	= 0;
	struct pciedev_dev*	dev		= a_filp->private_data;

#ifdef DEBUG_TIMING
	static int snIterations = 0;
	static u_int64_t	sullnNanoseconds = 0;
	u_int64_t	ullnCurrent;
	struct timespec	aTvEnd, aTvBeg;
	getnstimeofday(&aTvBeg);
#endif

	if (!dev->dev_sts)
	{
		ERRCT("ERROR: NO DEVICE %d\n", dev->dev_num);
		return -EFAULT;
	}

	if (EnterCritRegion(&dev->dev_mut)){ return -ERESTARTSYS; } /// new

	/*Check if it is pwrite call*/
	if (*a_f_pos != PCIED_FPOS)
	{
		u_int64_t	llnPos = (u_int64_t)a_f_pos;
		u_int32_t	reg_size = (llnPos & PRW_REG_SIZE_MASK) >> PRW_REG_SIZE_SHIFT;
		u_int32_t	barx_rw = (llnPos & PRW_BAR_MASK) >> PRW_BAR_SHIFT;
		u_int32_t	offset_rw = (llnPos & PRW_OFFSET_MASK);

		CORRECT_REGISTER_SIZE(reg_size, dev->register_size);
		retval = pciedev_write_inline(dev, reg_size, MTCA_SIMPLE_WRITE, barx_rw, offset_rw, a_buf, NULL, count);
	}
	else
	{
		device_rw			reading;
		const char* __user	pcUserBuffer; // Temporal buffer from user space to read value

		if (copy_from_user(&reading, a_buf, sizeof(device_rw))) {return -EFAULT;}
		// For compatibility in the case of count=1 data read from register
		// will be stored in the field 'data_rw'
			//if (count < 2){pcUserBuffer = (const char*)((const void*)&((device_rw*)a_buf)->data_rw); count = 1;}
			//else {pcUserBuffer = (const char*)reading.dataPtr;}
		//read used only for single read, count has to be 1
		pcUserBuffer = (const char*)((const void*)&((device_rw*)a_buf)->data_rw); 
		count = 1;

		//reading.mode_rw = W_INITIAL + (reading.mode_rw&_STEP_FOR_NEXT_MODE2_);
		CORRECT_REGISTER_SIZE(reading.register_size, dev->register_size);
		retval = pciedev_write_inline(dev, reading.register_size, MTCA_SIMPLE_WRITE,reading.barx_rw, reading.offset_rw, pcUserBuffer, NULL, count);
	}

	LeaveCritRegion(&dev->dev_mut);
#ifdef DEBUG_TIMING
#ifdef _SMB_WMB_
	_SMB_WMB_();
#endif
	getnstimeofday(&aTvEnd);
	snIterations++;
	ullnCurrent = NSEC_FROM_TIMESPEC(aTvBeg, aTvEnd);
	sullnNanoseconds += ullnCurrent;
	if (g_nPrintDebugInfo){ printk(KERN_ALERT "WRITE(%d):   iter=%d, current=%llu, totanNanosec=%llu\n", 
		(int)count, snIterations, ullnCurrent,sullnNanoseconds);
	}
#endif
	
    retval = sizeof(device_rw); // read/write returns sizeof(device_rw) or error
    return retval;
}
EXPORT_SYMBOL(pciedev_write_exp);

loff_t    pciedev_llseek(struct file *filp, loff_t off, int frm)
{
    ssize_t       retval         = 0;
    int             minor          = 0;
    int             d_num          = 0;
    
    struct pciedev_dev       *dev = filp->private_data;
    minor = dev->dev_minor;
    d_num = dev->dev_num;
        
    filp->f_pos = (loff_t)PCIED_FPOS;
    if(!dev->dev_sts){
        retval = -EFAULT;
        return retval;
    }
    return (loff_t)PCIED_FPOS;
}
