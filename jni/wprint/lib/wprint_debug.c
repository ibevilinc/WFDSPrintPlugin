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
#include <stdlib.h>
#include "wprint_debug.h"
#include <pthread.h>

static FILE *stdlog = NULL;
static void (*_debug_callback)(int cond, const char *dbg_msg) = NULL;

static int _debug_level = DBG_DEFAULT;

void wprint_set_debug_level(int level)
{
   _debug_level = level;
}

void wprint_set_stdlog(FILE *logfile)
{
   stdlog = logfile;
}

void wprint_set_debug_callback(void (*callback)(int cond, const char *dbg_msg))
{
    _debug_callback = callback;
}


#ifndef WPRINT_DISABLE_LOGGING

static char *_taskname(pthread_t  tid)
{
static char name[16];

   snprintf(name, sizeof(name), "0x%08x", (unsigned int) tid);
   return(name);
}

#define DEBUG_BUF_SIZE (2048)
#define DEBUG_MSG_SIZE (DEBUG_BUF_SIZE + 64)

void _ifprint(int cond, char *fmt, ...)
{
  va_list args;
  char *buff = malloc(DEBUG_BUF_SIZE);
  char *dbg_msg = malloc(DEBUG_MSG_SIZE);

  pthread_t  tid;
  void (*callback)(int cond, const char *dbg_msg) = _debug_callback;

  if (((buff != NULL) && (dbg_msg != NULL)) && (_debug_level & cond) )
  {
     va_start(args, fmt);
     vsnprintf(buff, DEBUG_BUF_SIZE, fmt, args);
     va_end(args);

     tid = pthread_self();
  
     snprintf(dbg_msg, DEBUG_MSG_SIZE, "<%-8s> %s%c", _taskname(tid), buff, (callback == NULL) ? '\n' : '\0');
     if (callback != NULL)
       (*callback)(cond, dbg_msg);
     else if (stdlog != NULL)
       fprintf(stdlog, "%s", dbg_msg);
  } 

  if (buff != NULL)    free(buff);
  if (dbg_msg != NULL) free(dbg_msg);

}  /*  _ifprint  */

#endif


