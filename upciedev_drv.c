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

#include "pciedev_ufn.h"
#include "pciedev_io.h"

MODULE_AUTHOR("Ludwig Petrosyan, David Kalantaryan");
MODULE_DESCRIPTION("DESY PCIE universal driver");
MODULE_VERSION("8.0.0");
MODULE_LICENSE("Dual BSD/GPL");

int g_nPrintDebugInfo = 0;
#ifndef WIN32
module_param_named(debug, g_nPrintDebugInfo, int, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL(g_nPrintDebugInfo);
#endif

static void __exit pciedev_cleanup_module(void)
{
	printk(KERN_NOTICE "UPCIEDEV_CLEANUP_MODULE CALLED\n");
}

#include "debug_functions.h"

void UociedevTestFunction(const char* a_string)
{
	ALERTRT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s\n", a_string);
}
EXPORT_SYMBOL(UociedevTestFunction);


static int __init pciedev_init_module(void)
{
	int   result  = 0;

	printk(KERN_NOTICE "UPCIEDEV_INIT_MODULE  MMAP VERSION CALLED\n");
	UociedevTestFunction("UPCIEDEV");
	printk(KERN_ALERT "UPCIEDEV_INIT:REGISTERING PCI DRIVER sizeof(loff_t)=%d, __USER_CS=0x%x\n", (int)sizeof(loff_t), (int)__USER_CS);
	return result; /* succeed */
}

module_init(pciedev_init_module);
module_exit(pciedev_cleanup_module);


