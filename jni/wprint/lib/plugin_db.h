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
#ifndef __PLUGIN_DB_H__
#define __PLUGIN_DB_H__

#include "lib_wprint.h"

int plugin_add(wprint_plugin_t *pluign);
wprint_plugin_t *plugin_search(char *mt, char *pf);

unsigned long long plugin_get_mime_type_bit(const char *mime_type);
void plugin_get_passthru_input_formats(unsigned long long *input_formats);
void plugin_get_convertable_input_formats(unsigned long long *input_formats);

#endif  /* __PLUGIN_DB_H__  */
