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
#ifndef __PRINT_MEDIA_TRAY_TYPES_H__
#define __PRINT_MEDIA_TRAY_TYPES_H__

typedef enum
{
    PT_PRELOAD = -2,
    PT_CONTINUOUS_FEED = -1,
    UNSPECIFIED = 0,
    PT_SOURCE_TRAY_1 = 1,
    PT_MANUAL_FEED = 2,
    PT_MANUAL_ENV_FEED = 3,
    PT_SOURCE_TRAY2 = 4,
    PT_OPTIONAL_SOURCE = 5,
    PT_OPTIONAL_ENV_FEED = 6,
    PT_SOURCE_PHOTO_TRAY = 6,
    PT_SRC_AUTO_SELECT = 7,
    PT_MANUAL_CD_FEED = 14,
} PT_media_src_t;

#endif /* __PRINT_MEDIA_TRAY_TYPES_H__ */
