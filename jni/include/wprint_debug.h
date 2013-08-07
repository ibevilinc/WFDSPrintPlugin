/*
(c) Copyright 2013 Hewlett-Packard Development Company, L.P.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef __WPRINT_DEBUG_H__
#define __WPRINT_DEBUG_H__

#include <stdio.h>
#include <stdarg.h>

/*-------------------------------------------------------------------------*/


#define DBG_FORCE      1
#define DBG_ERROR      2
#define DBG_SESSION    4

#define DBG_IO         8
#define DBG_LOG       16
#define DBG_VERBOSE   32

#define DBG_ALL      0xff    

#ifndef WPRINT_DISABLE_LOGGING

#define DBG_DEFAULT    0x1f

#define ifprint(statements)  _ifprint statements
void  _ifprint(int cond, char *fmt, ...);

#define OS_NULL_CHK(fn)     { assert(fn); }
#define OS_ERROR_CHK(fn)    { assert(ERROR != (fn)); }

#else

#define DBG_DEFAULT    DBG_VERBOSE //TODO: 11.11.11 CHANGED - should go back to previous value, as this is not correct for a NDEBUG path

#define ifprint(ignore)   ((void) 0)

#define OS_NULL_CHK(fn)     { fn; }
#define OS_ERROR_CHK(fn)    { fn; }

#endif

void wprint_set_debug_level(int level);
void wprint_set_stdlog(FILE *stdlog);
void wprint_set_debug_callback(void (*callback)(int cond, const char *dbg_msg));

void wprint_override_plugin_path(char *debug_plugin_path);
void wprint_override_print_format(char *print_format);
void wprint_override_pcl_type(int pcl_type);
void wprint_override_ipp_detour(void);


#endif  /* __WPRINT_DEBUG_H__  */

