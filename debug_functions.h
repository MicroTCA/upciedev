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
 *
 *  debug_functions.h
 *
 *  Created on: Jul 06, 2015
 *	Author: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  Functions for verbose loging implemented here
 *
 */
#ifndef __debug_functions_h__
#define __debug_functions_h__

#include <linux/printk.h>
#include <linux/string.h>

extern int g_nPrintDebugInfo;
#define DEBUGNEW(...)	if(g_nPrintDebugInfo){ALERTCT(__VA_ARGS__);}


#ifdef WIN32
#define _LAST_CHAR_		'\\'
#else
#define _LAST_CHAR_		'/'
#endif  /*  #ifdef WIN32 */

#define	__SOURCE_PATH__	(strrchr(__FILE__, _LAST_CHAR_) ? (strrchr(__FILE__, _LAST_CHAR_)+1) : __FILE__)
#define	__COMMON_FORMAT__ "fl:\"%s\";ln:%d;fnc:%s:  "
#define	__COMMON_ARGS__	__SOURCE_PATH__,__LINE__,__FUNCTION__

#define KERN_LOGCT(__a_loglevel__,...) {printk(__a_loglevel__ __COMMON_FORMAT__,__COMMON_ARGS__); \
										printk(KERN_CONT __VA_ARGS__); }

#define KERN_LOGRT(__a_loglevel__,...) {printk(__a_loglevel__ "mod:%s;"__COMMON_FORMAT__,THIS_MODULE->name,__COMMON_ARGS__); \
										printk(KERN_CONT __VA_ARGS__); }

#define DEBUGCT(...) KERN_LOGCT(KERN_DEBUG,__VA_ARGS__)
#define INFOCT(...) KERN_LOGCT(KERN_INFO,__VA_ARGS__)
#define ALERTCT(...) KERN_LOGCT(KERN_ALERT,__VA_ARGS__)
#define WARNCT(...)  KERN_LOGCT(KERN_WARNING,__VA_ARGS__)
#define ERRCT(...)  KERN_LOGCT(KERN_ERR,__VA_ARGS__)
#define DEBUGRT(...) KERN_LOGRT(KERN_DEBUG,__VA_ARGS__)
#define INFORT(...) KERN_LOGRT(KERN_INFO,__VA_ARGS__)
#define ALERTRT(...) KERN_LOGRT(KERN_ALERT,__VA_ARGS__)
#define WARNRT(...)  KERN_LOGRT(KERN_WARNING,__VA_ARGS__)
#define ERRRT(...)  KERN_LOGRT(KERN_ERR,__VA_ARGS__)


#endif  /* #ifndef __debug_functions_h__ */
