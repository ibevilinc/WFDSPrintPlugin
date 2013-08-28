/*
  Copyright (c) 2010,2011, Hewlett-Packard Co.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of Hewlett-Packard nor the names of its
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
  NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
  TO, PATENT INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "ifc_print_job.h"
#include "lib_wprint.h"
#include "wprint_debug.h"

#define BUFF_SIZE 8192

typedef struct
{
    const ifc_wprint_t    *wprint_ifc;
    const ifc_print_job_t *print_ifc;
} plugin_data_t;

static const char *_mime_types[] = {
      MIME_TYPE_PDF,
      NULL };
  
static const char *_print_formats[] = {
      PRINT_FORMAT_PDF,
      NULL };


static const char **_get_mime_types(void)
{
   return(_mime_types);
}

static const char **_get_print_formats(void)
{
   return(_print_formats);
}

static int  _start_job ( wJob_t                job_handle,
                         void                  *wprint_ifc_p,
                         void                  *print_ifc_p,
                         wprint_job_params_t   *job_params )
{
    plugin_data_t *priv;
    if (job_params == NULL)
        return(ERROR);

    job_params->plugin_data = NULL;
    if ((wprint_ifc_p == NULL) || (print_ifc_p == NULL))
        return(ERROR);

    priv = (plugin_data_t*)malloc(sizeof(plugin_data_t));

    priv->wprint_ifc = (ifc_wprint_t*)wprint_ifc_p;
    priv->print_ifc  = (ifc_print_job_t*)print_ifc_p;

    job_params->plugin_data = (void*)priv;

    return(OK);
} /* _start_page */


static int  _print_page ( wJob_t                job_handle,
                          wprint_job_params_t   *job_params,
                          char                  *mime_type,
                          char                  *pathname )
{
plugin_data_t *priv;
int fd;
int result = OK;
int rbytes, wbytes, nbytes = 0;
char *buff;

    if (job_params == NULL)
        return(ERROR);

    priv = (plugin_data_t*)job_params->plugin_data;

    if (priv == NULL)
        return(ERROR);


    //  open the PDF file and dump it to the socket

    if (pathname && strlen(pathname))
    {
        buff = malloc(BUFF_SIZE);
        if (buff == NULL)
            return(ERROR);

        fd = open(pathname, O_RDONLY);
        if (fd != ERROR)
        {
           rbytes = read(fd, buff, BUFF_SIZE);

           while ( (rbytes > 0) && !job_params->cancelled )
           {
               wbytes = priv->print_ifc->send_data(priv->print_ifc, buff, rbytes);
               if (wbytes == rbytes)
               {
                   nbytes += wbytes;
                   rbytes = read(fd, buff, BUFF_SIZE);
               }
               else
               {
                   priv->wprint_ifc->debug(DBG_ERROR, "ERROR: write() failed, %s", strerror(errno));
                   result = ERROR;
                   break;
               }
           }
           priv->wprint_ifc->debug(DBG_LOG, "dumped %d bytes of %s to printer\n", nbytes, pathname);
           close(fd);
        }

        free(buff);
    }

    return result;

}  /*  _print_page  */


static int _end_job  ( wJob_t              job_handle,
                       wprint_job_params_t *job_params )
{
    if (job_params != NULL)
    {
        if (job_params->plugin_data != NULL)
            free(job_params->plugin_data);
    }
    return OK;

}  /*   _end_job  */


wprint_plugin_t *libwprintplugin_pdf_reg(void)
{
static const wprint_plugin_t _pdf_plugin = 
  {
     .version           = WPRINT_PLUGIN_VERSION(0),
     .get_mime_types    = _get_mime_types,
     .get_print_formats = _get_print_formats,

     .start_job         = _start_job,
     .print_page        = _print_page,
     .print_blank_page  = NULL,
     .end_job           = _end_job,
   };

   return((wprint_plugin_t *) &_pdf_plugin);

}  /*  libwpdf_reg  */


