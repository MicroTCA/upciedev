#ifndef _PCIEDEV_IO_H
#define _PCIEDEV_IO_H

#include <linux/types.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#ifndef NUMBER_OF_BARS
#define NUMBER_OF_BARS 6
#endif

#define UPCIEDEV_SWAPL(x) ((((x) >> 24) & 0x000000FF) | (((x) >> 8) & 0x0000FF00) | (((x) << 8) & 0x00FF0000) | (((x) << 24) & 0xFF000000))
#define UPCIEDEV_SWAPS(x) ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))
#define UPCIEDEV_SWAP_TOT(size,x) ((size)==1) ? (x) : (((size)==2) ? UPCIEDEV_SWAPS((x)) : UPCIEDEV_SWAPL((x)))

#define		PRW_REG_SIZE_MASK	(int64_t)0xf000000000000000
#define		PRW_BAR_MASK		(int64_t)0x0f00000000000000
#define		PRW_OFFSET_MASK		(int64_t)0x00ffffffffffffff
#define		PRW_REG_SIZE_SHIFT	60
#define		PRW_BAR_SHIFT		56

// Register size
#define RW_D8                0x0
#define RW_D08              RW_D8
#define RW_D16              0x1
#define RW_D32              0x2
#define RW_DMA             0x3
#define RW_INFO            0x4
#define MTCA_MAX_RW 3

// DMA size
#define DMA_DATA_OFFSET            6 
#define DMA_DATA_OFFSET_BYTE  24
#define PCIEDEV_DMA_SYZE           4096
#define PCIEDEV_DMA_MIN_SYZE   128

#define IOCTRL_R      0x00
#define IOCTRL_W     0x01
#define IOCTRL_ALL  0x02

#define BAR0 0
#define BAR1 1
#define BAR2 2
#define BAR3 3
#define BAR4 4
#define BAR5 5

// All possible device access types
#define	MTCA_SIMPLE_READ		         0
#define	MTCA_LOCKED_READ		         1
#define	MT_READ_TO_EXTRA_BUFFER	2
#define	MTCA_SIMPLE_WRITE		         5
#define	MTCA_SET_BITS			         6
#define	MTCA_SWAP_BITS			7

#define	__GetRegisterSizeInBytes__(_a_value_)	(1<<(_a_value_))
#define	CORRECT_REGISTER_SIZE(_a_rw_mode_,_a_default_rw_mode_) \
{ \
	if((_a_rw_mode_)<0 || (_a_rw_mode_)>= MTCA_MAX_RW){(_a_rw_mode_)=(_a_default_rw_mode_);}\
}

typedef u_int64_t pointer_type;

/*
 * GetRegisterSizeInBytes:  gets register size from RW mode
 *                          safe to call from interrupt contex
 *
 * Parameters:
 *   a_unMode:        register access mode (read or write and register len (8,16 or 32 bit))
 *
 * Return (int):
 *   WRONG_MODE:      error (not correct mode provided)
 *   other:           register length in Bytes
 */
static inline int	GetRegisterSizeInBytes(int16_t a_unMode, int16_t a_default_rw_mode)
{
	CORRECT_REGISTER_SIZE(a_unMode, a_default_rw_mode);
	return __GetRegisterSizeInBytes__(a_unMode);
}




/* generic register access */
struct device_rw  {
	u_int32_t            offset_rw; /* offset in address                       */
	u_int32_t            data_rw;   /* data to set or returned read data       */
	union
	{
		u_int32_t			mode_rw;	/* mode of rw (RW_D8, RW_D16, RW_D32)      */
		u_int32_t			register_size; /* (RW_D8, RW_D16, RW_D32)      */
	};
	u_int32_t            barx_rw;   /* BARx (0, 1, 2, 3, 4, 5)                 */
	union
	{
		struct
		{
			u_int32_t		size_rw;	// !!! transfer size should not be providefd by this field.
								// This field is there for backward compatibility.
								//
								// Transfer size should be provided with read/write 3-rd
								// argument (count)
								// read(int fd, void *buf, size_t count);
								// ssize_t write(int fd, const void *buf, size_t count);
			u_int32_t		rsrvd_rw;
		};
		pointer_type		dataPtr;	// In the case of more than one register access
		// this fields shows pointer to user data
	};            
};
typedef struct device_rw device_rw;

/* generic register access from ioctl system call*/
typedef struct device_ioc_rw 
{
	u_int16_t		register_size;	/* (RW_D8, RW_D16, RW_D32)      */
	u_int16_t		rw_access_mode;	/* mode of rw (MTCA_SIMLE_READ,...)      */
	u_int32_t		barx_rw;	/* BARx (0, 1, 2, 3, 4, 5)                 */
	u_int32_t		offset_rw;	/* offset in address                       */
	u_int32_t		count_rw;	/* number of register to read or write     */
	pointer_type	dataPtr;	// In the case of more than one register access
	pointer_type	maskPtr;	// mask for bitwise write operations
}device_ioc_rw;


/* generic vector register access from ioctl system call*/
typedef struct device_vector_rw
{
	u_int64_t		number_of_rw;		/* Number of read or write operations to perform     */
	pointer_type	device_ioc_rw_ptr;	// User space pointer to read or write data    
								// In kernel space this field should be casted to (device_ioc_rw* __user)
}device_vector_rw;

struct device_ioctrl_data  {
        u_int    offset;
        u_int    data;
        u_int    cmd;
        u_int    reserved;
};
typedef struct device_ioctrl_data device_ioctrl_data;

struct device_ioctrl_dma  {
        u_int    dma_offset;
        u_int    dma_size;
        u_int    dma_cmd;          // value to DMA Control register
        u_int    dma_pattern;     // DMA BAR num
        u_int    dma_reserved1; // DMA Control register offset (31:16) DMA Length register offset (15:0)
        u_int    dma_reserved2; // DMA Read/Write Source register offset (31:16) Destination register offset (15:0)
};
typedef struct device_ioctrl_dma device_ioctrl_dma;

struct device_ioctrl_time  {
        struct timeval   start_time;
        struct timeval   stop_time;
};
typedef struct device_ioctrl_time device_ioctrl_time;

struct device_i2c_rw  {
       u_int            busNum; /* I2C Bus num*/
       u_int            devAddr;   /* I2C device address on the current bus */
       u_int            regAddr;   /* I2C register address on the current device */
       u_int            size;   /* number of bytes to  read/write*/
       u_int            done;  /* read done*/
       u_int            timeout;   /* transfer timeout usec */             
       u_int            status;  /* status */
       u_int            data[256];  /* data */
};
typedef struct device_i2c_rw device_i2c_rw;

/* Use 'o' as magic number */
#define PCIEDOOCS_IOC                               '0'
#define PCIE_GEN_IOC			       PCIEDOOCS_IOC

#define PCIEDEV_PHYSICAL_SLOT            _IOWR(PCIEDOOCS_IOC, 60, int)
#define PCIEDEV_DRIVER_VERSION          _IOWR(PCIEDOOCS_IOC, 61, int)
#define PCIEDEV_FIRMWARE_VERSION    _IOWR(PCIEDOOCS_IOC, 62, int)
#define PCIEDEV_SCRATCH_REG              _IOWR(PCIEDOOCS_IOC, 63, int)
#define PCIEDEV_GET_STATUS                 _IOWR(PCIEDOOCS_IOC, 64, int)
#define PCIEDEV_SET_SWAP                     _IOWR(PCIEDOOCS_IOC, 65, int)
#define PCIEDEV_SET_BITS			  _IOWR(PCIE_GEN_IOC, 66, struct device_ioc_rw)
#define PCIEDEV_SWAP_BITS		  _IOWR(PCIE_GEN_IOC, 67, struct device_ioc_rw)
#define PCIEDEV_VECTOR_RW		  _IOWR(PCIE_GEN_IOC, 68, struct device_vector_rw) 

#define PCIEDEV_SINGLE_IOC_ACCESS	  _IOWR(PCIE_GEN_IOC, 69, struct device_ioc_rw)      //?

#define PCIEDEV_GET_DMA_TIME            _IOWR(PCIEDOOCS_IOC, 70, int)
#define PCIEDEV_WRITE_DMA                 _IOWR(PCIEDOOCS_IOC, 71, int)
#define PCIEDEV_READ_DMA                   _IOWR(PCIEDOOCS_IOC, 72, int)
#define PCIEDEV_SET_IRQ                        _IOWR(PCIEDOOCS_IOC, 73, int)
#define PCIEDEV_I2C_READ                     _IOWR(PCIEDOOCS_IOC, 74, int)
#define PCIEDEV_I2C_WRITE                   _IOWR(PCIEDOOCS_IOC, 75, int)
#define PCIEDEV_UDRIVER_VERSION      _IOWR(PCIEDOOCS_IOC, 78, int)
#define PCIEDEV_GET_SWAP                   _IOWR(PCIEDOOCS_IOC, 79, int)

#define PCIEDEV_LOCK_DEVICE		  _IO(PCIE_GEN_IOC, 80)
#define PCIEDEV_UNLOCK_DEVICE	  _IO(PCIE_GEN_IOC, 81)
#define PCIEDEV_LOCKED_READ		  _IOWR(PCIE_GEN_IOC, 82, struct device_ioc_rw) //?

#define PCIEDEV_SET_REGISTER_SIZE	  _IOWR(PCIE_GEN_IOC, 83, int)
#define PCIEDEV_GET_REGISTER_SIZE	  _IO(PCIE_GEN_IOC, 84)

#define PCIEDOOCS_IOC_MINNR  60
#define PCIEDOOCS_IOC_MAXNR 100

#define PCIEDEV_IOC_COMMON_MINNR  70
#define PCIEDEV_IOC_COMMON_MAXNR 75
#define PCIEDOOCS_IOC_DMA_MINNR  70
#define PCIEDOOCS_IOC_DMA_MAXNR 75

#endif
