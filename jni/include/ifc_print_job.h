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
#ifndef __IFC_PRINT_JOB_H__
#define __IFC_PRINT_JOB_H__

#include "lib_wprint.h"
#include "ifc_wprint.h"

typedef struct ifc_print_job_st
{
    int (*init)(const struct ifc_print_job_st *this_p,
                 const char *printer_address);

    int (*validate_job)(const struct ifc_print_job_st *this_p,
                        wprint_job_params_t *job_params);

    int (*start_job)(const struct ifc_print_job_st *this_p,
                     const wprint_job_params_t *job_params);

    int (*send_data)(const struct ifc_print_job_st *this_p,
                     const char *buffer,
                     size_t length);

    int (*check_status)(const struct ifc_print_job_st *this_p);

    int (*end_job)(const struct ifc_print_job_st *this_p);

    void (*destroy)(const struct ifc_print_job_st *this_p);

    void (*enable_timeout)(const struct ifc_print_job_st *this_p,
                          int enable);
} ifc_print_job_t;

void printer_get_timeout(long int *timeout_msec);
void printer_set_timeout(long int timeout_msec);

const ifc_print_job_t *printer_connect(int port_num, const ifc_wprint_t *wprint_ifc);
int wConnect(const char *printer_addr,  int port_num,  long int timeout_msec);

#endif /* __IFC_PRINT_JOB_H__ */
