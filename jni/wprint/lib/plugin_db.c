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
#include <stdio.h>
#include <string.h>
#include "plugin_db.h"
#include "wprint_debug.h"
#include "mime_types.h"

#define _MAX_MIME_TYPES      32
#define _MAX_PRINT_FORMATS    8

#define _MAX_LIB_LENGTH      128
#define _MAX_PLUGINS         16


typedef struct
{
   wprint_plugin_t *plugin;
   const char  *name;
   char  mime_types[_MAX_MIME_TYPES][MAX_MIME_LENGTH + 1];
   char  print_formats[_MAX_PRINT_FORMATS][MAX_MIME_LENGTH + 1];
   int version;

} _plugin_db_t;


static _plugin_db_t  _plugin[_MAX_PLUGINS];
static int _index = 0;

int plugin_add(wprint_plugin_t *plugin) {

char const **mt, **pf;
int i, j, index;
const char *name;

    if (plugin == NULL) {
        return ERROR;
    }

    index = _index;
    mt= plugin->get_mime_types();
    pf = plugin->get_print_formats();
    int version = plugin->version;

    if ((mt == NULL) || (pf == NULL)) {
        return ERROR;
    }

    memset(&_plugin[index], 0, sizeof(_plugin_db_t));
    _plugin[index].version  = version;
    _plugin[index].plugin = plugin;

   ifprint((DBG_LOG, "   MIME types:"));
   // save a pointer to the name for comparison
   
   i = j = 0;  
   while(mt[i])
   {
      if (strlen(mt[i]) < MAX_MIME_LENGTH)
      {
		  ifprint((DBG_LOG, "\t\t   %s", mt[i])); 
          strncpy(_plugin[index].mime_types[j++], mt[i], MAX_MIME_LENGTH);
      }
      i++;
   }
   if (j < _MAX_MIME_TYPES)
       _plugin[index].mime_types[j][0] = 0;
       
   ifprint((DBG_LOG, "   print formats:"));

   i = j = 0;  
   while(pf[i])
   {
      if (strlen(pf[i]) < MAX_MIME_LENGTH)
      {
          ifprint((DBG_LOG, "\t\t   %s", pf[i]));
          strncpy(_plugin[index].print_formats[j++], pf[i], MAX_MIME_LENGTH);
      }
      i++;
   }
   if (j < _MAX_PRINT_FORMATS)
       _plugin[index].print_formats[j][0] = 0;

   ifprint((DBG_LOG, "\r"));

   _index++;

   return(OK);

}  /*  plugin_add  */


wprint_plugin_t *plugin_search(char *mt,  char *pf)
{
int i, j, k;

   for (i = 0; i < _index; i++)
   {
       j = 0;
       while (strlen(_plugin[i].print_formats[j]))
       {
          if (strcmp(_plugin[i].print_formats[j], pf) == 0)
          {
              k = 0;
              while(strlen(_plugin[i].mime_types[k]))
              {
                  if (strcmp(_plugin[i].mime_types[k], mt) == 0)
                  {
                      return(_plugin[i].plugin);
                  }
                  k++;
              }
          }
          j++;
       }
   }

   return(NULL);

}  /*  plugin_search */

unsigned long long plugin_get_mime_type_bit(const char *mime_type)
{
    unsigned long long bit = 0;
    if (strcmp(MIME_TYPE_PPM, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_PPM);
    else if (strcmp(MIME_TYPE_JPEG, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_JPEG);
    else if (strcmp(MIME_TYPE_PNG, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_PNG);
    else if (strcmp(MIME_TYPE_PDF, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_PDF);
    else if (strcmp(MIME_TYPE_WORD, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_WORD);
    else if (strcmp(MIME_TYPE_PPT, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_PPT);
    else if (strcmp(MIME_TYPE_EXCEL, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_EXCEL);
    else if (strcmp(MIME_TYPE_PCLM, mime_type) == 0)
        bit = (unsigned long long)(1 << INPUT_MIME_TYPE_PCLM);
    return(bit);
}

void plugin_get_passthru_input_formats(unsigned long long *input_formats)
{
    int i;
    *input_formats = 0;
    for(i = 0; i < _index; i++)
    {
        /* is this a passthrough plugin */
        if ((strcmp(_plugin[i].mime_types[0], _plugin[i].print_formats[0]) == 0) &&
            (strlen(_plugin[i].print_formats[1]) == 0) &&
            (strlen(_plugin[i].mime_types[1]) == 0))
        {
            *input_formats |= plugin_get_mime_type_bit(_plugin[i].mime_types[0]);
        }
    }
}

void plugin_get_convertable_input_formats(unsigned long long *input_formats)
{
    int i, j;
    *input_formats = 0;
    for(i = 0; i < _index; i++)
    {
        /* is this a passthrough plugin */
        if (!((strcmp(_plugin[i].mime_types[0], _plugin[i].print_formats[0]) == 0) &&
              (strlen(_plugin[i].mime_types[1]) == 0) &&
              (strlen(_plugin[i].print_formats[1]) == 0)))
        {
            for(j = 0; strlen(_plugin[i].mime_types[j]) != 0; j++)
            {
                *input_formats |= plugin_get_mime_type_bit(_plugin[i].mime_types[j]);
            }
        }
    }
}

