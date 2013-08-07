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

#include <jpeglib.h>

int read_JPEG_file (char * filename);

// #define OLDWAY
#ifdef OLDWAY
typedef enum
{
  deviceRGB,
  adobeRGB,
  grayScale
} colorSpaceDisposition;
#endif


extern int write_JPEG_Buff (ubyte * outBuff, int quality, int image_width, int image_height, JSAMPLE *imageBuffer, int resolution, colorSpaceDisposition, int *numCompBytes);
extern int write_JPEG_file (char * filename, int quality, int image_width, int image_height, JSAMPLE *imageBuffer, int resolution, colorSpaceEnum destCS);


extern int image_width;
extern int image_height;
extern int image_numComponents;
extern JSAMPLE * image_buffer; /* Points to large array of R,G,B-order data */
extern unsigned char *myImageBuffer;
extern int LZWEncodeFile(char *inBuff, int inBuffSize, char *outFile) ;

