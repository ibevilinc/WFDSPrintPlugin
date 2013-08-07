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
#ifndef __IFC_WPRINT_H__
#define __IFC_WPRINT_H__

#include "wprint_msgq.h"
#include "ifc_osimg.h"
#include "wtypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    void (*debug_start_job)(wJob_t jobHandle, const char *ext);
    void (*debug_job_data)(wJob_t jobHandle, const unsigned char *buff, unsigned long nbytes);
    void (*debug_end_job)(wJob_t jobHandle);
    void (*debug_start_page)(wJob_t jobHandle, int page_number, int width, int height);
    void (*debug_page_data)(wJob_t jobHandle, const unsigned char *buff, unsigned long nbytes);
    void (*debug_end_page)(wJob_t jobHandle);
} ifc_wprint_debug_stream_t;

typedef struct
{
    void (*debug)(int cond, char *fmt, ...);
    MSG_Q_ID (*msgQCreate)(int max_mesgs, int max_msg_length, int options);
    int (*msgQDelete)(MSG_Q_ID msgQ);
    int (*msgQSend)(MSG_Q_ID msgQ, char *buffer, unsigned long nbytes, int timeout, int priority);
    int (*msgQReceive)(MSG_Q_ID msgQ, char *buffer, unsigned long max_nbytes, int timeout);
    int (*msgQNumMsgs)(MSG_Q_ID msgQ);
    const ifc_osimg_t* (*get_osimg_ifc)(void);
    const ifc_wprint_debug_stream_t* (*get_debug_stream_ifc)(wJob_t id);
} ifc_wprint_t;

const ifc_wprint_t* wprint_get_ifc(void);

#ifdef __cplusplus
}
#endif

#endif /* __IFC_WPRINT_H__ */
