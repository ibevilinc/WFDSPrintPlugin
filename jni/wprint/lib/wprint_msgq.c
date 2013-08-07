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
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>

#include "wprint_msgq.h"
#include "wprint_debug.h"

#define _SEM_NAME_LENGTH    16

typedef struct
{
   MSG_Q_ID               msgq_id;
   char                   name[_SEM_NAME_LENGTH];
   int                    max_msgs;
   int                    max_msg_length;
   int                    num_msgs;
   sem_t                  sem_count;
   sem_t                  *sem_ptr;
   pthread_mutex_t        mutex;
   pthread_mutexattr_t    mutexattr;
   unsigned long          read_offset;
   unsigned long          write_offset;

}  _msgq_hdr_t;


MSG_Q_ID msgQCreate(int   max_msgs,
                    int   max_msg_length,
                    int   options)
{
_msgq_hdr_t  *msgq;
int          msgq_size;

   msgq_size = sizeof(_msgq_hdr_t) + max_msgs * max_msg_length;
   msgq = (_msgq_hdr_t *) malloc(msgq_size);

   if (msgq)
   {
       memset((char *) msgq, 0, msgq_size);        /* for safety */

       msgq->msgq_id         =  (MSG_Q_ID) msgq;
       msgq->max_msgs        =  max_msgs;
       msgq->max_msg_length  =  max_msg_length;
       msgq->num_msgs        =  0;

       /* create a mutex to protect access to this structure */
       pthread_mutexattr_init(&(msgq->mutexattr));
       pthread_mutexattr_settype(&(msgq->mutexattr),
#if __APPLE__
PTHREAD_MUTEX_RECURSIVE
#else /* assume linux for now */
PTHREAD_MUTEX_RECURSIVE_NP
#endif
);
       pthread_mutex_init(&msgq->mutex, &msgq->mutexattr);

#if __APPLE__
{
static unsigned long _next_sem = 0;
       sprintf(msgq->name, "mqsem_%08x",(unsigned int)_next_sem++);
       msgq->sem_ptr = sem_open(msgq->name, O_CREAT, 0660, 0);
}
#else
       /* create a counting semaphore */
       msgq->sem_ptr = &msgq->sem_count;
       sem_init(msgq->sem_ptr, 0, 0); /* PRIVATE, EMTPY */
#endif

       msgq->read_offset     =  0;
       msgq->write_offset    =  0;
   }

   return((MSG_Q_ID) msgq);

}  /*  msgQCreate  */


int msgQDelete(MSG_Q_ID    msgQ)
{
_msgq_hdr_t  *msgq = (MSG_Q_ID) msgQ;

   if (msgq)
   {
       pthread_mutex_lock(&(msgq->mutex));
       if (msgq->num_msgs)
       {
          ifprint((DBG_ERROR, 
                   "Warning msgQDelete() called on queue with %d messages",
                   msgq->num_msgs));
       }

#if __APPLE__
       sem_post(msgq->sem_ptr);
       sem_close(msgq->sem_ptr);
       sem_unlink(msgq->name);
#else
       sem_destroy(&(msgq->sem_count));
#endif
       pthread_mutex_unlock(&(msgq->mutex));
       pthread_mutex_destroy(&(msgq->mutex));
       free((void *) msgq);
   }

   return(msgq ? 0 : -1);

}  /*  msgQDelete  */


int msgQSend( MSG_Q_ID           msgQ,
              char               *buffer,
              unsigned long      nbytes,
              int                timeout,
              int                priority)
{
_msgq_hdr_t  *msgq = (MSG_Q_ID) msgQ;
char  *msg_loc;
int   result = -1;

   /*  validate function arguments  */
   if ( msgq && (timeout == NO_WAIT) && (priority == MSG_Q_FIFO) )
   {
       pthread_mutex_lock(&(msgq->mutex));

       /*  ensure the message conforms to size limits and there is room
        *  in the msgQ 
        */

       if ( (nbytes <= msgq->max_msg_length) &&
            (msgq->num_msgs < msgq->max_msgs) )
       {
           msg_loc = (char *) msgq + sizeof(_msgq_hdr_t) + 
                     (msgq->write_offset * msgq->max_msg_length);
           memcpy(msg_loc, buffer, nbytes);
           msgq->write_offset = (msgq->write_offset + 1) % msgq->max_msgs;
           msgq->num_msgs++;
           sem_post(msgq->sem_ptr);
           result = 0;
       }

       pthread_mutex_unlock(&(msgq->mutex));
   }

   return(result);

}  /*  msgQSend  */


int msgQReceive( MSG_Q_ID        msgQ,
                 char            *buffer,
                 unsigned long   max_nbytes,
                 int             timeout )
{
_msgq_hdr_t  *msgq = (MSG_Q_ID) msgQ;
char  *msg_loc;
int   result = -1;

   if ( msgq && buffer && 
        ((timeout == WAIT_FOREVER) || (timeout == NO_WAIT)) )
   {
       if (timeout == WAIT_FOREVER)
           /* coverity[lock] */
           result = sem_wait(msgq->sem_ptr);
       else
           if (timeout == NO_WAIT)
              /* coverity[lock] */
              result = sem_trywait(msgq->sem_ptr);

       if (result == 0)
       {
          pthread_mutex_lock(&(msgq->mutex));

          msg_loc = (char *) msgq + sizeof(_msgq_hdr_t) + 
                    (msgq->read_offset * msgq->max_msg_length);
          memcpy(buffer, msg_loc, max_nbytes);
          msgq->read_offset = (msgq->read_offset + 1) % msgq->max_msgs;
          msgq->num_msgs--;          

          pthread_mutex_unlock(&(msgq->mutex));
       }

       /* coverity[missing_unlock] : semaphore is counting not a mutex */
   }
   return(result);

}  /*  msgQReceive  */


int msgQNumMsgs( MSG_Q_ID        msgQ )
{
_msgq_hdr_t  *msgq = (MSG_Q_ID) msgQ;
int num_msgs = -1;

   if (msgq)
   {
       pthread_mutex_lock(&(msgq->mutex));
       num_msgs = msgq->num_msgs;
       pthread_mutex_unlock(&(msgq->mutex));
   }

   return(num_msgs);

}  /*  msgQNumMsgs  */

