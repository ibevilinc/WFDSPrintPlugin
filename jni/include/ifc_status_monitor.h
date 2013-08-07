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
#ifndef __IFC_STATUS_MONITOR_H__
#define __IFC_STATUS_MONITOR_H__

#include "wprint_status_types.h"

typedef struct ifc_status_monitor_st
{
    void (*init)(const struct ifc_status_monitor_st *this_p,
                 const char *network_address);

    void (*get_status)(const struct ifc_status_monitor_st *this_p,
                       printer_state_dyn_t *printer_state_dyn);
    int  (*cancel)(const struct ifc_status_monitor_st *this_p);
    void (*resume)(const struct ifc_status_monitor_st *this_p);

    void (*start)(const struct ifc_status_monitor_st *this_p,
                  void (*status_callback)(const printer_state_dyn_t *new_status,
                                          const printer_state_dyn_t *old_status,
                                          void *param),
                  void *param);
    void (*stop)(const struct ifc_status_monitor_st *this_p);

    /* closes up any internal structures and destroys interface */
    void (*destroy)(const struct ifc_status_monitor_st *this_p);
} ifc_status_monitor_t;

#endif /* __IFC_STATUS_MONITOR_H__ */
