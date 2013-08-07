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
/**********************************************************************************************
* File: genPCLm.c
*
* Author(s): Steve Claiborne
*
* To Do:								Status
*   - Error generation
*   - Backside duplex flip/mirror capability
*   - Opportunity to scale input image to device resolution
*   - Support Adobe RGB color space
*   - Skip white strips
*
*====  COMPLETED TASKS ===========================================================
*   - Generate output file name to reflect media and input params       12/20/2010
*   - Support 300 device resolution                                     12/17/2010
*   - Support 1200 device resolution                                    12/17/2010
*   - Media size support                                                12/17/2010
*   - Other compression technologies: delta, taos                       11/17/2010
*     - zlib 								11/17/2010
*     - RLE (Hi)					                12/13/2010
*   - Margin support                                                    N/A?
*   - Strip height programmability                                      11/18/2010
*   - Multiple pages                                                    11/23/2010
*   - Source image scaling 						11/09/2010
*   - Debug option							11/09/2010
*   - Add comment job ticket                                            12/02/2010
*   - Added grayscale                                                   12/20/2010
*   - Scaled source width to full-width of media                        12/17/2010
*   - Implemented PCLmGen OO Interface					02/10/2011
*   - AdobeRGB                                                          02/01/2011
*   - Support odd-page duplex for InkJet                                02/01/2011
*   - JPEG markers to reflect resolution                                02/16/2011
*   - JPEG markers for strip height are stuck at 16                     02/16/2011
*   - KidsArray, xRefTable, KidsString, pOutBuffer are static sized     02/23/2011
*   - Rewrite the logic for handling the **bufPtr                       02/24/2011
*   - Support short-edge duplex                                         03/04/2011
*   - Need to implement error handling correctly    			03/04/2011
*   - Fixed adobeRGB multi-job issue                                    03/08/2011
*   - Color convert crash fix                                           03/08/2011
*   - Added abilty to pass debug setting to genPCLm			03/08/2011
*   - Added top-margin capabilities to shift image right and down	04/12/2011
*   - Add ability to use PNG as input                                   04/01/2011
**********************************************************************************
*   - eliminate the flate_colorspace.bin file
*   - Change compression types intra-page
*   - Assert that CS does not change       
*   - Change CS type on page boundaries
*   - Implement the media*Offset functionality
*   - Leftover lines in a strip are not supported
*   - Need to implement debug -> logfile
*
*====  Log of issues / defects  ==================================================
* 0.54: programmable strip height                                       11/23/2010
* 0.53: run-time crash of large source images                           11/17/2010
*       switched to getopt for parsing                                  11/17/2010
* 0.55: Add multi-page support						11/23/2010
* 0.56: Fixing /Length issue & removed leading comment                  11/29/2010
* 0.57: Fixing scaling and image position issues                        12/01/2010
* 0.58: Fixing white space at bottom of page                            12/01/2010
* 0.58: Fixing floating point error by switching to dev coordinates     12/02/2010
* 0.59: Added comment job-ticket                                        12/03/2010
* 0.60: Fixed xref issues that caused performance degradation           12/08/2010
*       Added support for -h 0 (generates 1 strip)                      12/08/2010
*       Added JPEG compression into JFIF header                         12/08/2010
* 0.63  Fixed media-padding issue for mediaHeight			12/20/2010
*       Fixed media-padding issue for non-600 resolutions		12/20/2010
* 0.65  Added ability to inject blank page for duplex			02/02/2011
*
* Known Issues:
*   - Can't convert large images to PDF					Fixed 11/18(0.53)
*   - 1200 dpi images are rendered to PDF at 600 dpi                    Fixed 12/17(0.61)
*
**********************************************************************************************/

/**********************************************************************************************
* JPEG image pointers:
*   - myImageBuffer: allocated source image buffer
*   - destBuffer:    current buffer pointer for put_scanline_someplace
*
**********************************************************************************************/

/**********************************************************************************************
* zlib parameters
* compress2 (dest, destLen, source, sourceLen)
*  Compresses the source buffer into the destination buffer.  sourceLen is the byte
*  length of the source buffer. Upon entry, destLen is the total size of the
*  destination buffer, which must be at least 0.1% larger than sourceLen plus
*  12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

*  compress returns Z_OK if success, Z_MEM_ERROR if there was not enough
*  memory, Z_BUF_ERROR if there was not enough room in the output buffer,
*  Z_STREAM_ERROR if the level parameter is invalid.
*    #define Z_OK            0
*    #define Z_STREAM_END    1
*    #define Z_NEED_DICT     2
*    #define Z_ERRNO        (-1)
*    #define Z_STREAM_ERROR (-2)
*    #define Z_DATA_ERROR   (-3)
*    #define Z_MEM_ERROR    (-4)
*    #define Z_BUF_ERROR    (-5)
*    #define Z_VERSION_ERROR (-6)
*
**********************************************************************************************/
#define STAND_ALLONE

#include "PCLmGenerator.h"
#include "media.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <zlib.h>
#include <unistd.h>

#include "common_defines.h"
#include "genPCLm.h"
#include "ctype.h"

#include "genPCLm_proto.h"
#include "flate_colorspace.h"
#include "myrle.h"

int RLEEncodeImage(ubyte *inFile, ubyte *outFile, int destSize);

/*
Defines
*/
#define STRIP_HEIGHT 16
#define JPEG_QUALITY 100

#define XREF_SIZE 10000
#define TEMP_BUFF_SIZE 10000000

#define DEFAULT_OUTBUFF_SIZE 64*5120*3*10
#define COMPRESSION_PAD      1024*1000

#define STANDARD_SCALE_FOR_PDF 72.0

#define KID_STRING_SIZE 1000

#define CATALOG_OBJ_NUMBER 1
#define PAGES_OBJ_NUMBER   2

/*
Local Variables
*/
static PCLmSUserSettingsType PCLmSSettings;

/*
Defines
*/
#define rgb_2_gray(r,g,b) (ubyte)(0.299*(double)r+0.587*(double)g+0.114*(double)b)

// Note: this is required for debugging
boolean writeOutputFile(int numBytes, ubyte *ptr);


/********************************************* Helper Routines **************************
Function: shiftStripByLeftMargin
Purpose:  To shift the strip image right in the strip buffer by leftMargin pixels
Assumptions: The strip buffer was allocated large enough to handle the shift; if not
then the image data on the right will get clipped.
Details:
We allocate a full strip (height and width), but then only copy numLinesThisCall from the original buffer
to the newly allocated buffer.  This pads the strips for JPEG processing.
*/

ubyte *shiftStripByLeftMargin(ubyte *ptrToStrip,sint32 currSourceWidth,sint32 currStripHeight, sint32 numLinesThisCall,
                              sint32 currMediaWidth, sint32 leftMargin, colorSpaceDisposition destColorSpace)
{
        ubyte *fromPtr=ptrToStrip, *toPtr, *newStrip;
        sint32 scanLineWidth;

        assert(currSourceWidth+(2*leftMargin)<=currMediaWidth);

       // writeOutputFile(currSourceWidth*3*numLinesThisCall, ptrToStrip);

       if(destColorSpace==grayScale)
       {
         scanLineWidth=currMediaWidth;
         // Allocate a full strip
         newStrip=(ubyte*)malloc(scanLineWidth*currStripHeight);
         memset(newStrip,0xff,scanLineWidth*currStripHeight);

         for(int i=0;i<numLinesThisCall;i++)
         {
            toPtr=newStrip+leftMargin+(i*currMediaWidth);
            fromPtr=ptrToStrip+(i*currSourceWidth);
            memcpy(toPtr,fromPtr,currSourceWidth);
         }

      }
      else
      {
        scanLineWidth=currMediaWidth*3;
        sint32 srcScanlineWidth=currSourceWidth*3;
        sint32 shiftAmount=leftMargin*3;
        newStrip=(ubyte*)malloc(scanLineWidth*currStripHeight);
        memset(newStrip,0xff,scanLineWidth*currStripHeight);
        for(int i=0;i<numLinesThisCall;i++)
        {
          toPtr=newStrip+shiftAmount+(i*scanLineWidth);
          fromPtr=ptrToStrip+(i*srcScanlineWidth);
          memcpy(toPtr,fromPtr,srcScanlineWidth);
          // memset(toPtr,0xe0,srcScanlineWidth);
        }
     }

     return(newStrip);
}

#ifdef SUPPORT_WHITE_STRIPS
bool PCLmGenerator::isWhiteStrip(void *pInBuffer, int inBufferSize)
{
  uint32 *ptr=(uint32*)pInBuffer;
  for(int i=0;i<inBufferSize/4;i++,ptr++)
  {
    if(*ptr!=0xffffffff)
      return(false);
  }
  return(true);
}
#endif

void PCLmGenerator::Cleanup(void)
{
  if(allocatedOutputBuffer)
  {
    free(allocatedOutputBuffer);
    allocatedOutputBuffer = NULL;
    currOutBuffSize = 0;
  }

  if(leftoverScanlineBuffer)
  {
    free(leftoverScanlineBuffer);
    leftoverScanlineBuffer=NULL;
  }
  if(scratchBuffer)
  {
    free(scratchBuffer);
    scratchBuffer = NULL;
  }
  if(xRefTable)
  {
    free(xRefTable);
    xRefTable=NULL;
  }
  if(KidsArray)
  {
    free(KidsArray);
    KidsArray=NULL;
  }
}

int PCLmGenerator::errorOutAndCleanUp()
{
  Cleanup();
  jobOpen=job_errored;
  return(genericFailure);
}

static sint32 startXRef=0;
static sint32 endXRef=0;

/***************************************************************
 * Warning: don't muck with this unless you understand what is going on.  This function attempst to
 * fix the xref table, based upon the strips getting inserted in reverse order (on the backside
 * page).  It does the following:
 *   1) Calculates the new object reference size (using tmpArray)
 *   2) Adds 2 to the object size to componsate for the offset
 *   3) Reorders the Image FileBody and the ImageTrasformation, as these are actually 1 PDF object
 *   4) Frees the tmp array
 **************************************************************/
void PCLmGenerator::fixXRef()
{
   if(!startXRef || !mirrorBackside)
     return;

   if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
   {
     assert(startXRef); 
     sint32 start=startXRef;
     sint32 end=endXRef-1;
     sint32 aSize=endXRef-startXRef-1;

     sint32 *tmpArray=(sint32*)malloc(aSize*20);

     sint32 xRefI=startXRef;
     for(int i=0;i<aSize+1;i++,xRefI++) 
     {
        *(tmpArray+i)=xRefTable[xRefI+1]-xRefTable[xRefI];
        //printf("size=%d: %d - %d\n",*(tmpArray+i),xRefTable[xRefI+1],xRefTable[xRefI]);
     }

     //printf("second time\n");
     // Reorder header and image sizes
     for(int i=0;i<aSize+1;i+=2,xRefI++)
     {
        sint32 t=*(tmpArray+i);
        *(tmpArray+i)=*(tmpArray+i+1);
        *(tmpArray+i+1)=t;
     }
//   for(int i=0;i<aSize+1;i++,xRefI++) 
//      printf("size=%d\n",*(tmpArray+i));

// printf("endXRef=%d\n",endXRef);
// for(int ii=startXRef;ii<endXRef+1;ii++)
//   printf("xRefTable[%d]=%d \n",ii,xRefTable[ii]);
     
     xRefI=aSize;
// printf("start=%d, end=%d\n",start+1, end);
     for(int i=start+1, j=aSize; i<end+2;i++, start++, xRefI--, j--)
     {
// printf("xRefTable[%d]=%d, *tmpArray=%d, summed=%d\n",i-1,xRefTable[i-1],*(tmpArray+j),
// xRefTable[i-1] + *tmpArray);
        xRefTable[i]=(xRefTable[i-1] + *(tmpArray+j));
     } 

// printf("start=%d, end=%d\n",startXRef+2, endXRef);
     for(int i=startXRef+2; i<endXRef;i++)
     {
// printf("i=%d\n",i);
        xRefTable[i]+=2;
     }

// printf("After\n");
// for(int ii=startXRef;ii<endXRef+1;ii++)
//   printf("xRefTable[%d]=%d\n",ii,xRefTable[ii]);


     //for(int i=startXRef; i<j/2;i++,j--)
     //
     // Now reorder, so that image0 is first in the xref table
     sint32 k=endXRef-1;
//     sint32 j=k;
     int i;
     sint32 lSize=(endXRef-startXRef)/2;
// printf("endXRef=%d, startXRef=%d, lSize=%d\n",endXRef, startXRef, lSize);
//      printf("switching order, aSize=%d, start=%d, end=%d\n",j, startXRef, endXRef-1);
// printf("startXRef+(j/2)+1=%d\n",startXRef+(j/2));
     for(i=startXRef; i<startXRef+lSize;i++,k--)
     {
// printf("SJC: i=%d, k=%d\n",i,k);
        sint32 t=xRefTable[i]; 
        xRefTable[i]=xRefTable[k];
        xRefTable[k]=t;
     }
     free(tmpArray);
   } 

//printf("Finally\n");
//for(int ii=startXRef;ii<endXRef+1;ii++)
//  printf("xRefTable[%d]=%d\n",ii,xRefTable[ii]);

   startXRef=0;
}


bool PCLmGenerator::addXRef(sint32 xRefObj)
{
   #define XREF_ARRAY_SIZE 100
   if(!xRefTable)
   {
     xRefTable=(sint32*)malloc(XREF_ARRAY_SIZE*sizeof(sint32)); 
     assert(xRefTable);
     xRefTable[0]=0;
     xRefIndex++;
   }

   xRefTable[xRefIndex]=xRefObj;
   xRefIndex++;

   if(!(xRefIndex%XREF_ARRAY_SIZE))
   {
     xRefTable=(sint32*)realloc(xRefTable,(((xRefIndex+XREF_ARRAY_SIZE)*sizeof(sint32))));
     if(DebugIt)
       printf("Realloc xRef: 0x%lx\n",(unsigned long int)xRefTable);
   }
   return(true);
}


bool PCLmGenerator::addKids(sint32 kidObj)
{
   #define KID_ARRAY_SIZE 20
   if(!KidsArray)
   {
     KidsArray=(sint32*)malloc(KID_ARRAY_SIZE*sizeof(sint32)); 
     assert(KidsArray);
   }

   KidsArray[numKids]=kidObj;
   numKids++;

   if(!(numKids%KID_ARRAY_SIZE))
   {
     KidsArray=(sint32*)realloc(KidsArray,((numKids+KID_ARRAY_SIZE)*sizeof(sint32)));
   }
   return(true);
}

boolean writeOutputFile(int numBytes, ubyte *ptr)
{
  FILE *outputFile;
  char outFileName[20];
  static int fileCntr=0;

  sprintf(outFileName,"outfile_%04d",fileCntr);
  fileCntr++;

  // Open output PDF file
  if (!(outputFile = fopen (outFileName, "w")))
  {
       fprintf (stderr, "Could not open the output file out - %s.\n", "t.pdf");
       exit (-1);
  }
  fwrite(ptr,numBytes,1,outputFile);
  fclose(outputFile); 
  return(true);
}

void PCLmGenerator::initOutBuff(char *buff, sint32 size)
{
   currBuffPtr=outBuffPtr=buff;
   outBuffSize=size;
   totalBytesWrittenToCurrBuff=0;
   memset(buff,0,size);
}


void PCLmGenerator::writeStr2OutBuff(char *str)
{
   sint32 strSize=strlen(str);
   // Make sure we have enough room for the copy
   char *maxSize=currBuffPtr+strSize;
   assert(maxSize-outBuffPtr < outBuffSize);
   memcpy(currBuffPtr,str,strSize);
   currBuffPtr+=strSize;
   totalBytesWrittenToCurrBuff+=strSize;
   totalBytesWrittenToPCLmFile+=strSize;
}

void PCLmGenerator::write2Buff(ubyte *buff, int buffSize)
{
   char *maxSize=currBuffPtr+buffSize;
   if(maxSize-outBuffPtr > outBuffSize)
   {
     printf("outBuffSize too small: %ld, %d\n",maxSize-outBuffPtr,outBuffSize);
     assert(0);
   }
   memcpy(currBuffPtr,buff,buffSize);
   currBuffPtr+=buffSize;
   totalBytesWrittenToCurrBuff+=buffSize;
   totalBytesWrittenToPCLmFile+=buffSize;
}

int PCLmGenerator::statOutputFileSize()
{
  addXRef(totalBytesWrittenToPCLmFile);
  return(1);
}

void PCLmGenerator::writePDFGrammarTrailer(int imageWidth, int imageHeight)
{
  int i;
  char KidsString[KID_STRING_SIZE];
  if(DebugIt2)
  {
    fprintf(stderr, "imageWidth=%d\n",imageWidth);
    fprintf(stderr, "imageHeight=%d\n",imageHeight);
  }

  sprintf(pOutStr,"%%============= PCLm: FileBody: Object 1 - Catalog\n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();
  sprintf(pOutStr,"%d 0 obj\n", CATALOG_OBJ_NUMBER); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Type /Catalog\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Pages %d 0 R\n",PAGES_OBJ_NUMBER); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);

  sprintf(pOutStr,"%%============= PCLm: FileBody: Object 2 - page tree \n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();
  sprintf(pOutStr,"%d 0 obj\n", PAGES_OBJ_NUMBER); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Count %d\n",numKids); writeStr2OutBuff(pOutStr);

  // Define the Kids for this document as an indirect array
  sprintf(KidsString,"/Kids [ ");writeStr2OutBuff(KidsString);
  for(i=0;i<numKids;i++)
  {
    //spot=strlen(KidsString);
    sprintf(KidsString,"%d 0 R ",KidsArray[i]);
    writeStr2OutBuff(KidsString);
  }

  sprintf(KidsString,"]\n");
  writeStr2OutBuff(KidsString);

  
  sprintf(pOutStr,"/Type /Pages\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);

  sprintf(pOutStr,"%%============= PCLm: cross-reference section: object 0, 6 entries\n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();

  // Fix up the xref table for backside duplex
  fixXRef();
  
  xRefStart=xRefIndex-1;
  infoObj=xRefIndex;

  sprintf(pOutStr,"xref\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"0 %d\n",1); writeStr2OutBuff(pOutStr);

  sprintf(pOutStr,"0000000000 65535 f\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%d %d\n",PAGES_OBJ_NUMBER+1,xRefIndex-4); writeStr2OutBuff(pOutStr);
  for(i=1;i<xRefIndex-3;i++)
  {
    sprintf(pOutStr,"%010d %05d n\n",xRefTable[i],0); writeStr2OutBuff(pOutStr);
  }

#ifdef PIECEINFO_SUPPORTED
  // HP PieceInfo Structure
  sprintf(pOutStr,"9996 0 obj\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"<</HPDefine1 1\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Private 9997 0 R>>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"9997 0 obj\n"); writeStr2OutBuff(pOutStr);
#endif
  //sprintf(pOutStr,"<</AIMetaData 32 0 R/AIPDFPrivateData1 33 0 R/AIPDFPrivateData10 34 0\n");
  
  // Now add the catalog and page object
  sprintf(pOutStr,"%d 2\n",CATALOG_OBJ_NUMBER); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%010d %05d n\n",xRefTable[xRefIndex-3],0); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%010d %05d n\n",xRefTable[xRefIndex-2],0); writeStr2OutBuff(pOutStr);

  sprintf(pOutStr,"%%============= PCLm: File Trailer\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"trailer\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  // sprintf(pOutStr,"/Info %d 0\n", infoObj); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Size %d\n", xRefIndex-1); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Root %d 0 R\n",CATALOG_OBJ_NUMBER); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"startxref\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%d\n",xRefTable[xRefStart]); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%%%EOF\n"); writeStr2OutBuff(pOutStr);
}

bool PCLmGenerator::injectAdobeRGBCS()
{
  if(adobeRGBCS_firstTime)
  {
    // We need to inject the ICC object for AdobeRGB
    sprintf(pOutStr,"%%============= PCLm: ICC Profile\n"); writeStr2OutBuff(pOutStr);
    statOutputFileSize();
    sprintf(pOutStr,"%d 0 obj\n", objCounter); objCounter++; writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"[/ICCBased %d 0 R]\n",objCounter); writeStr2OutBuff(pOutStr);

    sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);
    statOutputFileSize();
    sprintf(pOutStr,"%d 0 obj\n", objCounter); objCounter++; writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"/N 3\n"); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"/Alternate /DeviceRGB\n"); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"/Length %u\n",ADOBE_RGB_SIZE+1); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"/Filter /FlateDecode\n"); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"stream\n"); writeStr2OutBuff(pOutStr);

#ifdef STAND_ALLONE
    #define ADOBE_RGB_SIZE 284
    FILE *inFile;
    if (!(inFile = fopen ("flate_colorspace.bin", "rb")))
    {
      fprintf(stderr, "can't open %s\n", "flate_colorspace.bin");
      return 0;
    }

    ubyte *buffIn=(unsigned char *)malloc(ADOBE_RGB_SIZE); 
    assert(buffIn);

    sint32 bytesRead=fread( buffIn, 1, ADOBE_RGB_SIZE, inFile );
    assert(bytesRead==ADOBE_RGB_SIZE);
    fclose(inFile);
    write2Buff(buffIn,bytesRead);
    if(buffIn)
    {
      free(buffIn);
      buffIn=NULL;
    }
#else
    write2Buff(flateBuffer,ADOBE_RGB_SIZE);
#endif  // STEVECODE

    sprintf(pOutStr,"\nendstream\n"); writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);
  }
  
  adobeRGBCS_firstTime=false;
  return(true);
}

/****************************************************************************************
* Function: colorConvertSource
* Purpose: to convert an image from one color space to another.
* Limitations:
*   - Currently, only supports RGB->GRAY
*****************************************************************************************/
bool PCLmGenerator::colorConvertSource(colorSpaceDisposition srcCS, colorSpaceDisposition dstCS, ubyte *strip, sint32 stripWidth, sint32 stripHeight)
{
  if(srcCS==deviceRGB && dstCS==grayScale)
  {
    // Do an inplace conversion from RGB -> 8 bpp gray
    ubyte *srcPtr=strip;
    ubyte *dstPtr=strip;
    for(int h=0;h<stripHeight;h++)
    {
      for(int w=0;w<stripWidth;w++,dstPtr++,srcPtr+=3)
      {
        //*dstPtr=(ubyte)((0.299*((double)r))+(0.587*((double)g)+0.114)*((double)b));
        *dstPtr=(ubyte)rgb_2_gray(*srcPtr,*(srcPtr+1),*(srcPtr+2));
      }
    }
    dstNumComponents=1;
    // sourceColorSpace=grayScale; // We don't want to change this, as we are only changing the
    // current strip.
  }
  else
    assert(1);

  //writeOutputFile(stripWidth*stripHeight, strip);
  return(true);
}

int PCLmGenerator::injectRLEStrip(ubyte *RLEBuffer, int numBytes, int imageWidth, int imageHeight, colorSpaceDisposition destColorSpace, bool whiteStrip)
{
  //unsigned char c;
  char fileStr[25];
  static int stripCount=0;
  bool printedImageTransform=false;

  if(DebugIt2)
  {
    printf("Injecting RLE compression stream into PDF\n");
    printf("  numBytes=%d, imageWidth=%d, imageHeight=%d\n",numBytes,imageWidth,imageHeight);
    sprintf(fileStr,"rleFile.%d",stripCount);
    stripCount++;
    write2Buff(RLEBuffer,numBytes);
  }

  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    if(!startXRef)
      startXRef=xRefIndex;

    injectImageTransform();
    printedImageTransform=true;
  }


  if(destColorSpace==adobeRGB)
  { 
    injectAdobeRGBCS();
  }

  // Inject LZ compressed image into PDF file
  sprintf(pOutStr,"%%============= PCLm: FileBody: Strip Stream: RLE Image \n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();

  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter-1); objCounter++; writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter); objCounter++; writeStr2OutBuff(pOutStr);
  }
  
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Width %d\n", imageWidth); writeStr2OutBuff(pOutStr);
  if(destColorSpace==deviceRGB)
  {
    sprintf(pOutStr,"/ColorSpace /DeviceRGB\n"); writeStr2OutBuff(pOutStr);
  }
  else if(destColorSpace==adobeRGB)
  {
    sprintf(pOutStr,"/ColorSpace 5 0 R\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"/ColorSpace /DeviceGray\n"); writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"/Height %d\n", imageHeight); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Filter /RunLengthDecode\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Subtype /Image\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Length %d\n",numBytes); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Type /XObject\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/BitsPerComponent 8\n"); writeStr2OutBuff(pOutStr);
#ifdef SUPPORT_WHITE_STRIPS
  if(whiteStrip)
  {
    sprintf(pOutStr,"/Name /WhiteStrip\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"/Name /ColorStrip\n"); writeStr2OutBuff(pOutStr);
  }
#endif

  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"stream\n"); writeStr2OutBuff(pOutStr);

  // Write the zlib compressed strip to the PDF output file
  write2Buff(RLEBuffer,numBytes);
  sprintf(pOutStr,"\nendstream\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);

  if(!printedImageTransform)
    injectImageTransform();

  endXRef=xRefIndex;

  return(1);
}

int PCLmGenerator::injectLZStrip(ubyte *LZBuffer, int numBytes, int imageWidth, int imageHeight, colorSpaceDisposition destColorSpace, bool whiteStrip)
{
  //unsigned char c;
  bool printedImageTransform=false;

  if(DebugIt2)
  {
    printf("Injecting LZ compression stream into PDF\n");
    printf("  numBytes=%d, imageWidth=%d, imageHeight=%d\n",numBytes,imageWidth,imageHeight);
  }

  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    if(!startXRef)
       startXRef=xRefIndex;

    injectImageTransform();
    printedImageTransform=true;
  }

  if(destColorSpace==adobeRGB)
  { 
    injectAdobeRGBCS();
  }

  // Inject LZ compressed image into PDF file
  sprintf(pOutStr,"%%============= PCLm: FileBody: Strip Stream: zlib Image \n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();

  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter-1); objCounter++; writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter); objCounter++; writeStr2OutBuff(pOutStr);
  }

  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Width %d\n", imageWidth); writeStr2OutBuff(pOutStr);
  if(destColorSpace==deviceRGB)
  {
    sprintf(pOutStr,"/ColorSpace /DeviceRGB\n"); writeStr2OutBuff(pOutStr);
  }
  else if(destColorSpace==adobeRGB)
  {
    sprintf(pOutStr,"/ColorSpace 5 0 R\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"/ColorSpace /DeviceGray\n"); writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"/Height %d\n", imageHeight); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Filter /FlateDecode\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Subtype /Image\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Length %d\n",numBytes); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Type /XObject\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/BitsPerComponent 8\n"); writeStr2OutBuff(pOutStr);
#ifdef SUPPORT_WHITE_STRIPS
  if(whiteStrip)
  {
    sprintf(pOutStr,"/Name /WhiteStrip\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"/Name /ColorStrip\n"); writeStr2OutBuff(pOutStr);
  }
#endif

  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"stream\n"); writeStr2OutBuff(pOutStr);

  // Write the zlib compressed strip to the PDF output file
  write2Buff(LZBuffer,numBytes);
  sprintf(pOutStr,"\nendstream\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);

  if(!printedImageTransform)
    injectImageTransform();

  endXRef=xRefIndex;

  return(1);
}

void PCLmGenerator::injectImageTransform()
{
  char str[512];
  int strLength;
  sprintf(str,"q /image Do Q\n");
  strLength=strlen(str);

  // Output image transformation information 
  sprintf(pOutStr,"%%============= PCLm: Object - Image Transformation \n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();
  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter+1); objCounter++; writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter); objCounter++; writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Length %d\n",strLength); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"stream\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%s",str); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endstream\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);
}

int PCLmGenerator::injectJPEG(char *jpeg_Buff, int imageWidth, int imageHeight, int numCompBytes, colorSpaceDisposition destColorSpace, bool whiteStrip)
{
  // int fd, bytesRead;
  int strLength;
  char str[512];

  bool printedImageTransform=false;

  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    if(!startXRef)
      startXRef=xRefIndex;

    injectImageTransform();
    printedImageTransform=true;
  }

  if(DebugIt2)
  {
    printf("Injecting jpegBuff into PDF\n");
  }

  yPosition+=imageHeight;

  if(destColorSpace==adobeRGB)
  { 
    injectAdobeRGBCS();
  }

  // Inject PDF JPEG into output file
  sprintf(pOutStr,"%%============= PCLm: FileBody: Strip Stream: jpeg Image \n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();
  if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
  {
    //printf("frontside, oc=%d\n",objCounter);sprintf(pOutStr,"%d 0 obj\n", objCounter);
    //objCounter++; writeStr2OutBuff(pOutStr);
    sprintf(pOutStr,"%d 0 obj\n", objCounter-1); objCounter++; 
    writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"%d 0 obj\n", objCounter); objCounter++; 
    writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Width %d\n", imageWidth); writeStr2OutBuff(pOutStr);
  if(destColorSpace==deviceRGB)
  {
    sprintf(pOutStr,"/ColorSpace /DeviceRGB\n"); writeStr2OutBuff(pOutStr);
  }
  else if(destColorSpace==adobeRGB)
  {
    sprintf(pOutStr,"/ColorSpace 5 0 R\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"/ColorSpace /DeviceGray\n"); writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"/Height %d\n", imageHeight); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Filter /DCTDecode\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Subtype /Image\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Length %d\n",numCompBytes); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Type /XObject\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/BitsPerComponent 8\n"); writeStr2OutBuff(pOutStr);
#ifdef SUPPORT_WHITE_STRIPS
  if(whiteStrip)
  {
    sprintf(pOutStr,"/Name /WhiteStrip\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"/Name /ColorStrip\n"); writeStr2OutBuff(pOutStr);
  }
#endif
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"stream\n"); writeStr2OutBuff(pOutStr);

  // Inject JPEG image into stream
  //fd = open(jpeg_filename, O_RDWR);
  //if(fd==-1)
  //{
  //  fprintf (stderr, "Could not open the image file %s.\n", jpeg_filename);
  //  exit(-1);
  //}

  //inPtr=(unsigned char *)malloc(fileSize);
  //bytesRead=read(fd, inPtr, fileSize);
  //write(stdout, inPtr, bytesRead);
  write2Buff((ubyte*)jpeg_Buff,numCompBytes);
  sprintf(pOutStr,"\nendstream\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);

  sprintf(str,"q /image Do Q\n");
  strLength=strlen(str);

  if(!printedImageTransform)
    injectImageTransform();

  endXRef=xRefIndex;

  return(1);
}


void writeStr2Buff(char *buffer,char *str)
{
  int buffSize;
  char *buffPos;

  buffSize=strlen(buffer)+strlen(str);
  if(buffSize>TEMP_BUFF_SIZE)
    assert(0);

  buffSize=strlen(buffer);
  buffPos=buffer+buffSize;
  sprintf(buffPos,"%s",str);

  buffSize=strlen(buffer);
  if(buffSize>TEMP_BUFF_SIZE)
  {
    printf("tempBuff size exceeded: buffSize=%d\n",buffSize);
    assert(0);
  }
}


/**********************************************************************************************
* Function:       writePDFGrammarPage
* Purpose:        to generate the PDF page construct(s), which includes the image information.
* Implementation: the /Length definition is required for PDF, so we write the stream to a RAM buffer
*      first, then calculate the Buffer size, insert the PDF /Length construct, then write the
*      buffer to the PDF file.  I used the RAM buffer instead of a temporary file, as a driver
*      implementation won't be able to use a disk file.
***********************************************************************************************/

void PCLmGenerator::writePDFGrammarPage(int imageWidth, int imageHeight, int numStrips, colorSpaceDisposition destColorSpace)
{
  int i, imageRef=objCounter+2, buffSize;
  int yAnchor;
  char str[512];
  char *tempBuffer;
  int startImageIndex=0;
  int numLinesLeft = 0;

  if(destColorSpace==adobeRGB && 1 == pageCount)
  {
    imageRef+=2; // Add 2 for AdobeRGB
  }

  tempBuffer=(char *)malloc(TEMP_BUFF_SIZE);
  assert(tempBuffer);
  if(DebugIt2)
     printf("Allocated %d bytes for tempBuffer\n",TEMP_BUFF_SIZE);
  memset(tempBuffer,0x0,TEMP_BUFF_SIZE);

  sprintf(pOutStr,"%%============= PCLm: FileBody: Object 3 - page object\n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();
  sprintf(pOutStr,"%d 0 obj\n", objCounter); writeStr2OutBuff(pOutStr);
  addKids(objCounter);
  objCounter++;
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Type /Page\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Parent %d 0 R\n",PAGES_OBJ_NUMBER); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/Resources <<\n"); writeStr2OutBuff(pOutStr);
  // sprintf(pOutStr,"/ProcSet [ /PDF /ImageC ]\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"/XObject <<\n"); writeStr2OutBuff(pOutStr);

  if(topMarginInPix)
  {
     for(i=0;i<numFullInjectedStrips;i++,startImageIndex++)

     {
       sprintf(str,"/Image%d %d 0 R\n",startImageIndex,imageRef);
       sprintf(pOutStr,"%s",str); writeStr2OutBuff(pOutStr);
       imageRef+=2;
     }
     if(numPartialScanlinesToInject)
     {
       sprintf(str,"/Image%d %d 0 R\n",startImageIndex,imageRef);
       sprintf(pOutStr,"%s",str); writeStr2OutBuff(pOutStr);
       imageRef+=2;
       startImageIndex++;
     }
  }

  for(i=startImageIndex;i<numStrips+startImageIndex;i++)
  {
     sprintf(str,"/Image%d %d 0 R\n",i,imageRef);
     // sprintf(pOutStr,"/ImageA 4 0 R /ImageB 6 0 R >>\n"); writeStr2OutBuff(pOutStr);
     sprintf(pOutStr,"%s",str); writeStr2OutBuff(pOutStr);
     imageRef+=2;
  }
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  if(currMediaOrientationDisposition==landscapeOrientation)
  {
    pageOrigin=mediaWidth;
    sprintf(pOutStr,"/MediaBox [ 0 0 %d %d ]\n", mediaHeight, mediaWidth); writeStr2OutBuff(pOutStr);
  }
  else
  {
    pageOrigin=mediaHeight;
    sprintf(pOutStr,"/MediaBox [ 0 0 %d %d ]\n", mediaWidth, mediaHeight); writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"/Contents [ %d 0 R ]\n",objCounter); writeStr2OutBuff(pOutStr);
#ifdef PIECEINFO_SUPPORTED
  sprintf(pOutStr,"/PieceInfo <</HPAddition %d 0 R >> \n",9997); writeStr2OutBuff(pOutStr);
#endif
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);

  // Create the FileBody stream first, so we know the Length of the stream
  if(reverseOrder)
  {
    yAnchor=0;
  }
  else
  {
    yAnchor=(int)((pageOrigin*STANDARD_SCALE)+0.99); // Round up
  }

  // Setup the CTM so that we can send device-resolution coordinates
  sprintf(pOutStr,"%%Image Transformation Matrix: width, skewX, skewY, height, xAnchor, yAnchor\n"); writeStr2OutBuff(pOutStr);
  sprintf(str,"%f 0 0 %f 0 0 cm\n",STANDARD_SCALE_FOR_PDF/currRenderResolutionInteger,STANDARD_SCALE_FOR_PDF/currRenderResolutionInteger);
  writeStr2Buff(tempBuffer,str);

  startImageIndex=0;
  if(topMarginInPix)
  {
     for(i=0;i<numFullInjectedStrips;i++)
     {
       if(reverseOrder)
         yAnchor+=numFullScanlinesToInject;
       else
         yAnchor-=numFullScanlinesToInject;

       sprintf(str,"/P <</MCID 0>> BDC q\n");
       writeStr2Buff(tempBuffer,str);

       sprintf(str,"%%Image Transformation Matrix: width, skewX, skewY, height, xAnchor, yAnchor\n");
       writeStr2Buff(tempBuffer,str);
  
       sprintf(str,"%d 0 0 %d 0 %d cm\n",imageWidth*scaleFactor,numFullScanlinesToInject*scaleFactor,yAnchor*scaleFactor);
       writeStr2Buff(tempBuffer,str);

       sprintf(str,"/Image%d Do Q\n",startImageIndex);
       writeStr2Buff(tempBuffer,str);

       startImageIndex++; 
     }
     if(numPartialScanlinesToInject)
     {
       if(reverseOrder)
         yAnchor+=numPartialScanlinesToInject;
       else
         yAnchor-=numPartialScanlinesToInject;

       sprintf(str,"/P <</MCID 0>> BDC q\n");
       writeStr2Buff(tempBuffer,str);

       sprintf(str,"%%Image Transformation Matrix: width, skewX, skewY, height, xAnchor, yAnchor\n");
       writeStr2Buff(tempBuffer,str);
  
       sprintf(str,"%d 0 0 %d 0 %d cm\n",imageWidth*scaleFactor,numPartialScanlinesToInject*scaleFactor,yAnchor*scaleFactor);
       writeStr2Buff(tempBuffer,str);

       sprintf(str,"/Image%d Do Q\n",startImageIndex);
       writeStr2Buff(tempBuffer,str);

       startImageIndex++; 
     }
  }

  for(i=startImageIndex;i<numStrips+startImageIndex;i++)
  {
     //last strip may have less lines than currStripHeight. So update yAnchor using left over lines
     if(i == (numStrips+startImageIndex-1))
     {
       numLinesLeft = currSourceHeight - ((numStrips-1) * currStripHeight);

       if(reverseOrder)
         yAnchor+=numLinesLeft;
       else
         yAnchor-=numLinesLeft;
     }
     else
     {
       if(reverseOrder)
         yAnchor+=currStripHeight;
       else
         yAnchor-=currStripHeight;
     }

     sprintf(str,"/P <</MCID 0>> BDC q\n");
     writeStr2Buff(tempBuffer,str);

     sprintf(str,"%%Image Transformation Matrix: width, skewX, skewY, height, xAnchor, yAnchor\n");
     writeStr2Buff(tempBuffer,str);

     if(i == (numStrips+startImageIndex-1)) //last strip may have less lines than currStripHeight
     {
        sprintf(str,"%d 0 0 %d 0 %d cm\n",imageWidth*scaleFactor,numLinesLeft*scaleFactor,yAnchor*scaleFactor);
        writeStr2Buff(tempBuffer,str);
     }
     else if(yAnchor<0)
     {
       sint32 newH=currStripHeight+yAnchor;
       sprintf(str,"%d 0 0 %d 0 %d cm\n",imageWidth*scaleFactor,newH*scaleFactor,0*scaleFactor);
       writeStr2Buff(tempBuffer,str);
     }
     else
     {
       sprintf(str,"%d 0 0 %d 0 %d cm\n",imageWidth*scaleFactor,currStripHeight*scaleFactor,yAnchor*scaleFactor);
       writeStr2Buff(tempBuffer,str);
     }

     sprintf(str,"/Image%d Do Q\n",i);
     writeStr2Buff(tempBuffer,str);

  }

  // Resulting buffer size
  buffSize=strlen(tempBuffer);

  sprintf(pOutStr,"%%============= PCLm: FileBody: Page Content Stream object\n"); writeStr2OutBuff(pOutStr);
  statOutputFileSize();
  sprintf(pOutStr,"%d 0 obj\n",objCounter); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"<<\n"); writeStr2OutBuff(pOutStr);

  sprintf(pOutStr,"/Length %d\n",buffSize); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,">>\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"stream\n"); writeStr2OutBuff(pOutStr);

  // Now write the FileBody stream
  write2Buff((ubyte*)tempBuffer,buffSize);

  sprintf(pOutStr,"endstream\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"endobj\n"); writeStr2OutBuff(pOutStr);
  objCounter++;
  if(tempBuffer)
  {
    free(tempBuffer);
    tempBuffer=NULL;
  }
}


/****************************************************************************************
* Function: prepImageForBacksideDuplex
* Purpose:  To mirror the source image in preperation for backside duplex support
* Limitations:
*   - 
*****************************************************************************************/
boolean prepImageForBacksideDuplex(ubyte *imagePtr, sint32 imageHeight, sint32 imageWidth, sint32 numComponents)
{
  sint32 numBytes=imageHeight*imageWidth*numComponents;
  ubyte *head, *tail, t0, t1, t2; 

  if(numComponents==3)
  {
    for(head=imagePtr,tail=imagePtr+numBytes-1;tail>head;)
    {
        t0=*head;
        t1=*(head+1);
        t2=*(head+2);

        *head=    *(tail-2);
        *(head+1)=*(tail-1);
        *(head+2)=*(tail-0);
        *tail=    t2;
        *(tail-1)=t1;
        *(tail-2)=t0;

        head+=3;
        tail-=3;
    }
  }
  else
  {
    for(head=imagePtr,tail=imagePtr+numBytes;tail>head;)
    {
      t0=*head;
      *head=*tail;
      *tail=t0;
      head++;
      tail--;
    }
  }
//origTail++;
  return(true);
}

bool PCLmGenerator::getInputBinString(jobInputBin bin, char *returnStr)
{
  memset(returnStr,0,sizeof(returnStr));
  switch (bin)
  {
    case alternate:      strcpy(returnStr,"alternate"); break;
    case alternate_roll: strcpy(returnStr,"alternate_roll"); break;
    case auto_select:    strcpy(returnStr,"auto_select"); break;
    case bottom:         strcpy(returnStr,"bottom"); break;
    case center:         strcpy(returnStr,"center"); break;
    case disc:           strcpy(returnStr,"disc"); break;
    case envelope:       strcpy(returnStr,"envelope"); break;
    case hagaki:         strcpy(returnStr,"hagaki"); break;
    case large_capacity: strcpy(returnStr,"large_capacity"); break;
    case left:           strcpy(returnStr,"left"); break;
    case main_tray:      strcpy(returnStr,"main_tray"); break;
    case main_roll:      strcpy(returnStr,"main_roll"); break;
    case manual:         strcpy(returnStr,"manual"); break;
    case middle:         strcpy(returnStr,"middle"); break;
    case photo:          strcpy(returnStr,"photo"); break;
    case rear:           strcpy(returnStr,"rear"); break;
    case right:          strcpy(returnStr,"right"); break;
    case side:           strcpy(returnStr,"side"); break;
    case top:            strcpy(returnStr,"top"); break;
    case tray_1:         strcpy(returnStr,"tray_1"); break;
    case tray_2:         strcpy(returnStr,"tray_2"); break;
    case tray_3:         strcpy(returnStr,"tray_3"); break;
    case tray_4:         strcpy(returnStr,"tray_4"); break;
    case tray_5:         strcpy(returnStr,"tray_5"); break;
    case tray_N:         strcpy(returnStr,"tray_N"); break;
    default: assert(0); break;
  }
  return(true);
}

bool PCLmGenerator::getOutputBin(jobOutputBin bin, char *returnStr)
{
  memset(returnStr,0,sizeof(returnStr));
  switch(bin)
  {
    case top_output:               strcpy(returnStr,"top_output"); break;
    case middle_output:            strcpy(returnStr,"middle_output"); break;
    case bottom_output:            strcpy(returnStr,"bottom_output"); break;
    case side_output:              strcpy(returnStr,"side_output"); break;
    case center_output:            strcpy(returnStr,"center_output"); break;
    case rear_output:              strcpy(returnStr,"rear_output"); break;
    case face_up:                  strcpy(returnStr,"face_up"); break;
    case face_down:                strcpy(returnStr,"face_down"); break;
    case large_capacity_output:    strcpy(returnStr,"large_capacity_output"); break;
    case stacker_N:                strcpy(returnStr,"stacker_N"); break;
    case mailbox_N:                strcpy(returnStr,"mailbox_N"); break;
    case tray_1_output:            strcpy(returnStr,"tray_1_output"); break;
    case tray_2_output:            strcpy(returnStr,"tray_2_output"); break;
    case tray_3_output:            strcpy(returnStr,"tray_3_output"); break;
    case tray_4_output:            strcpy(returnStr,"tray_4_output"); break;
    default: assert(0); break;
  }
  return(true);
}

void PCLmGenerator::writeJobTicket()
{
  // Write JobTicket
  char inputBin[256];
  char outputBin[256];

  if(!m_pPCLmSSettings)
    return;

  getInputBinString(m_pPCLmSSettings->userInputBin,&inputBin[0]);
  getOutputBin(m_pPCLmSSettings->userOutputBin, &outputBin[0]);
  strcpy(inputBin, inputBin);
  strcpy(outputBin, outputBin);

  sprintf(pOutStr,"%%  genPCLm (Ver: %f)\n",PCLM_Ver); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%============= Job Ticket =============\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%  PCLmS-Job-Ticket\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      job-ticket-version: 0.1\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      epcl-version: 1.01\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%    JobSection\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      job-id: xxx\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%    MediaHandlingSection\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      media-size-name: %s\n",currMediaName); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      media-type: %s\n",m_pPCLmSSettings->userMediaType); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      media-source: %s\n",inputBin); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      sides: xxx\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      finishings: xxx\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      output-bin: %s\n",outputBin); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%    RenderingSection\n"); writeStr2OutBuff(pOutStr);
  if(currCompressionDisposition==compressDCT)
  {
    sprintf(pOutStr,"%%      pclm-compression-method: JPEG\n"); writeStr2OutBuff(pOutStr);
  }
  else if(currCompressionDisposition==compressFlate)
  {
    sprintf(pOutStr,"%%      pclm-compression-method: FLATE\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"%%      pclm-compression-method: RLE\n"); writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"%%      strip-height: %d\n",currStripHeight); writeStr2OutBuff(pOutStr);

  if(destColorSpace==deviceRGB)
  {
    sprintf(pOutStr,"%%      print-color-mode: deviceRGB\n"); writeStr2OutBuff(pOutStr);
  }
  else if(destColorSpace==adobeRGB)
  {
    sprintf(pOutStr,"%%      print-color-mode: adobeRGB\n"); writeStr2OutBuff(pOutStr);
  }
  else if(destColorSpace==grayScale)
  {
    sprintf(pOutStr,"%%      print-color-mode: gray\n"); writeStr2OutBuff(pOutStr);
  }

  sprintf(pOutStr,"%%      print-quality: %d\n",m_pPCLmSSettings->userPageQuality); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      printer-resolution: %d\n",currRenderResolutionInteger); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      print-content-optimized: xxx\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      orientation-requested: %d\n",m_pPCLmSSettings->userOrientation); writeStr2OutBuff(pOutStr);

  if(PCLmSSettings.userCopies==0)
    PCLmSSettings.userCopies=1;

  sprintf(pOutStr,"%%      copies: %d\n",m_pPCLmSSettings->userCopies); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%      pclm-raster-back-side: xxx\n"); writeStr2OutBuff(pOutStr);
  if(currRenderResolutionInteger)
  {
    sprintf(pOutStr,"%%      margins-pre-applied: TRUE\n"); writeStr2OutBuff(pOutStr);
  }
  else
  {
    sprintf(pOutStr,"%%      margins-pre-applied: FALSE\n"); writeStr2OutBuff(pOutStr);
  }
  sprintf(pOutStr,"%%  PCLmS-Job-Ticket-End\n"); writeStr2OutBuff(pOutStr);
}

void PCLmGenerator::writePDFGrammarHeader()
{
  // sprintf(pOutStr,"%%============= PCLm: File Header \n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%PDF-1.7\n"); writeStr2OutBuff(pOutStr);
  sprintf(pOutStr,"%%PCLm 1.0\n"); writeStr2OutBuff(pOutStr);
}

int PCLmGenerator::RLEEncodeImage(ubyte *in, ubyte *out, int inLength)
{
  // Overview
  //    compress input by identifying repeating bytes (not sequences)
  //    Compression ratio good for grayscale images, not great on RGB
  //    Output:
  //      1-127:   literal run
  //      128:     end of compression block
  //      129-256: repeating byte sequence
  //
  ubyte *imgPtr=in;
  ubyte *endPtr=in+inLength;
  ubyte *origOut=out;
  ubyte c;
  sint32 cnt=0;

  while(imgPtr<endPtr)
  {
    c=*imgPtr++;
    cnt=1;

    // Figure out how many repeating bytes are in the image
    while(*imgPtr==c && cnt<inLength )
    {
      if(imgPtr>endPtr)
        break;
      cnt++;
      imgPtr++;
    }

    // If cnt > 1, then output repeating byte specification
    //    The syntax is "byte-count repeateByte", where byte-count is
    //    257-byte-count.
    //    Since the cnt value is a byte, if the repeateCnt is > 128
    //    then we need to put out multiple repeat-blocks
    //    Referred to as method 1, range is 128-256
    if(cnt>1)
    {
      // Handle the repeat blocks of greater than 128 bytes
      while(cnt>128)
      {              // block of 128 repeating bytes
         *out++=129; // i.e. 257-129==128
         *out++=c;
         cnt-=128;
      }
      // Now handle the repeats that are < 128
      if(cnt)
      {
        *out++=(257-cnt); // i.e. cnt==2: 257-255=2
        *out++=c;
      }
    }
    // If cnt==1, then this is a literal run - no repeating bytes found.
    //    The syntax is "byte-count literal-run", where byte-count is < 128 and
    //    literal-run is the non-repeating bytes of the input stream.
    //    Referred to as method 2, range is 0-127
    else
    {
      ubyte *start, *p;
      sint32 i;
      start=(imgPtr-1);  // The first byte of the literal run

      // Now find the end of the literal run
      for(cnt=1,p=start;*p!=*imgPtr;p++,imgPtr++,cnt++)
        if(imgPtr>=endPtr) break;
      if(!(imgPtr==endPtr))
        imgPtr--;    // imgPtr incremented 1 too many
      cnt--;
      // Blocks of literal bytes can't exceed 128 bytes, so output multiple
      //    literal-run blocks if > 128
      while(cnt>128)
      {
        *out++=127;
        for(i=0;i<128;i++)
          *out++=*start++;
        cnt-=128;
      }
      // Now output the leftover literal run
      *out++=cnt-1;
      for(i=0;i<cnt;i++)
        *out++=*start++;
    }
  }
  // Now, write the end-of-compression marker (byte 128) into the output stream
  *out++=128;
  // Return the compressed size
  return((int)(out-origOut));
}

/* PCLmGenerator Constructor
 * 
 * /desc
 * /param  
 * /return 
 */
PCLmGenerator::PCLmGenerator()
{
  strcpy(currMediaName,"LETTER");
  currDuplexDisposition=simplex;
  currColorSpaceDisposition=deviceRGB;
  currDebugDisposition=debugOn;
  currCompressionDisposition=compressDCT;
  currMediaOrientationDisposition=portraitOrientation;
  currRenderResolution=res600;
  currStripHeight=STRIP_HEIGHT;

  // Default media h/w to letter specification
  mediaWidthInPixels=0;
  mediaHeightInPixels=0;
  mediaWidth=612;
  mediaHeight=792;
  destColorSpace=deviceRGB;
  sourceColorSpace=deviceRGB;
  scaleFactor=1;
  genExtraPage=false;
  jobOpen=job_closed;
  scratchBuffer=NULL;
  pageCount=0;

  currRenderResolutionInteger=600;
  STANDARD_SCALE=(float)currRenderResolutionInteger/(float)STANDARD_SCALE_FOR_PDF;
  yPosition=0;
  infoObj=0;
  numKids=0;
// XRefTable storage 
  xRefIndex=0;

  DebugIt=0;
  DebugIt2=0;

  objCounter=PAGES_OBJ_NUMBER+1;
  totalBytesWrittenToPCLmFile=0;

  // Initialize first index in xRefTable
  xRefTable=NULL;
  KidsArray=NULL;

  // Initialize the output Buffer
  allocatedOutputBuffer=NULL;

  // Initialize the leftover scanline logic
  leftoverScanlineBuffer=0;

  adobeRGBCS_firstTime=true;
  mirrorBackside=true;
 
  topMarginInPix=0;
  leftMarginInPix=0;
  m_pPCLmSSettings = NULL;
}

/* PCLmGenerator Destructor
 * 
 * /desc
 * /param  
 * /return 
 */
PCLmGenerator::~PCLmGenerator()
{
  Cleanup();
}

/* StartJob
 * 
 * /desc
 * /param  
 * /return 
 */
#ifdef STANDALONE
int  PCLmGenerator::StartJob(void **pOutBuffer, int *iOutBufferSize, bool debug)
#else
int  PCLmGenerator::StartJob(void **pOutBuffer, int *iOutBufferSize)
#endif
{
    DebugIt=debug;
    DebugIt2=debug;

    if(DebugIt)
      printf("genPCLm::StartJob\n");

    // Allocate the output buffer; we don't know much at this point, so make the output buffer size
    // the worst case dimensions; when we get a startPage, we will resize it appropriately
    outBuffSize=DEFAULT_OUTBUFF_SIZE;
    *iOutBufferSize=outBuffSize; 
    *pOutBuffer=(ubyte*)malloc(outBuffSize); // This multipliy by 10 needs to be removed...

    if(NULL == *pOutBuffer)
    {
      return(errorOutAndCleanUp());
    }

    currOutBuffSize=outBuffSize;
    if(DebugIt2)
      printf("Allocated %d for myOutBufferSize\n",outBuffSize);

    if(NULL==*pOutBuffer)
    {
      return(errorOutAndCleanUp());
    }

    allocatedOutputBuffer=*pOutBuffer;
    initOutBuff((char*)*pOutBuffer,outBuffSize);
    writePDFGrammarHeader();
    *iOutBufferSize=totalBytesWrittenToCurrBuff;
    jobOpen=job_open;

    return(success);  
}

/* EndJob
 * 
 * /desc
 * /param  
 * /return 
 */
int  PCLmGenerator::EndJob(void **pOutBuffer, int *iOutBufferSize)
{
    int result; 

    if(DebugIt)
      printf("genPCLm::EndJob\n");


    if(NULL==allocatedOutputBuffer)
    {
      return(errorOutAndCleanUp());
    }

    *pOutBuffer = allocatedOutputBuffer;

    initOutBuff((char*)*pOutBuffer,outBuffSize);

    // Write PDF trailer
    writePDFGrammarTrailer(currSourceWidth, currSourceHeight);

    if(!DebugIt && (currCompressionDisposition==compressDCT))
    {
      result=remove("jpeg_chunk.jpg");
    }

    *iOutBufferSize=totalBytesWrittenToCurrBuff;

    jobOpen=job_closed;

    if(xRefTable)
    {
      free(xRefTable);
      xRefTable=NULL;
    }
    if(KidsArray)
    {
      free(KidsArray);
      KidsArray=NULL;
    }

    return(success);  
}

/* PCLmSStartPage
 * * 
 * * /desc
 * * /param  
 * * /return 
 * */
int  PCLmGenerator::StartPage(PCLmSSetup *PCLmSPageContent, bool generatePCLmS, void **pOutBuffer, int *iOutBufferSize)
{
  m_pPCLmSSettings=PCLmSPageContent->PCLmSUserSettings;
  return(StartPage(PCLmSPageContent->PCLmPageContent,pOutBuffer,iOutBufferSize));
}

/* StartPage
 * 
 * /desc
 * /param  
 * /return 
 */
int  PCLmGenerator::StartPage(PCLmPageSetup *PCLmPageContent, void **pOutBuffer, int *iOutBufferSize)
{
	int numImageStrips;
    // Save the resolution information
    currRenderResolution=PCLmPageContent->destinationResolution;

    *pOutBuffer = allocatedOutputBuffer;

    if(currRenderResolution==res300)
      currRenderResolutionInteger=300;
    else if(currRenderResolution==res600)
      currRenderResolutionInteger=600;
    else if(currRenderResolution==res1200)
      currRenderResolutionInteger=1200;
    else
      assert(0);

    // Recalculate STANDARD_SCALE to reflect the job resolution
    STANDARD_SCALE=(float)currRenderResolutionInteger/(float)STANDARD_SCALE_FOR_PDF;

    // Media and source sizes are in 72 DPI; convert media information to native resolutions: 
    //   Add 0.5 to force rounding
    // Use the values set by the caller
    currSourceWidth = PCLmPageContent->SourceWidthPixels;
    currSourceHeight = PCLmPageContent->SourceHeightPixels;

    // Save off the media information
    mediaWidth=(int)(PCLmPageContent->mediaWidth);
    mediaHeight=(int)(PCLmPageContent->mediaHeight);

    // Use the values set by the caller
    mediaWidthInPixels = PCLmPageContent->mediaWidthInPixels;
    mediaHeightInPixels= PCLmPageContent->mediaHeightInPixels;


    topMarginInPix=(int)(((PCLmPageContent->mediaHeightOffset/STANDARD_SCALE_FOR_PDF)*currRenderResolutionInteger)+0.50);
    leftMarginInPix=(int)(((PCLmPageContent->mediaWidthOffset/STANDARD_SCALE_FOR_PDF)*currRenderResolutionInteger)+0.50);

    currCompressionDisposition=PCLmPageContent->compTypeRequested;

	if(DebugIt)
    {
      printf("genPCLm::StartPage\n");
      printf("  mediaName=%s\n",   PCLmPageContent->mediaSizeName);
      printf("  clientLocale=%s\n",PCLmPageContent->mediaSizeName);
      printf("  mediaHeight=%f\n", PCLmPageContent->mediaHeight);
      printf("  mediaWidth=%f\n",  PCLmPageContent->mediaWidth);
      printf("  topMargin=%d\n",   topMarginInPix);
      printf("  leftMargin=%d\n",  leftMarginInPix);
 
      printf("  topLeftMargin=%f,%f\n",PCLmPageContent->mediaWidthOffset,PCLmPageContent->mediaHeightOffset);
      printf("  sourceHeight=%f\n",PCLmPageContent->sourceHeight);
      printf("  sourceWidth=%f\n", PCLmPageContent->sourceWidth);
      printf("  stripHeight=%d\n", PCLmPageContent->stripHeight);
      printf("  scaleFactor=%d\n", PCLmPageContent->scaleFactor);
      printf("  genExtraPage=%d\n",PCLmPageContent->genExtraPage);
      if(PCLmPageContent->colorContent==color_content)
        printf("  colorContent=color_content\n");
      else if(PCLmPageContent->colorContent==gray_content)
        printf("  colorContent=gray_content\n");
      else
        printf("  colorContent=unknown_content\n");

      if(PCLmPageContent->pageOrigin==top_left)
        printf("  pageOrigin=top_left\n");
      else
        printf("  pageOrigin=bottom_right\n");
      
      if(PCLmPageContent->compTypeRequested==compressRLE)
        printf("compTypeRequested=RLE\n");
      else if(PCLmPageContent->compTypeRequested==compressDCT)
        printf("compTypeRequested=DCT\n");
      else if(PCLmPageContent->compTypeRequested==compressFlate)
        printf("compTypeRequested=Flate\n");
      else if(PCLmPageContent->compTypeRequested==compressDefault)
        printf("compTypeRequested=Flate\n");
      else if(PCLmPageContent->compTypeRequested==compressNone)
        printf("compTypeRequested=None\n");

      if(PCLmPageContent->dstColorSpaceSpefication==deviceRGB)
        printf("colorSpaceSpefication=deviceRGB\n");
      else if(PCLmPageContent->dstColorSpaceSpefication==adobeRGB)
        printf("colorSpaceSpefication=adobeRGB\n");
      else if(PCLmPageContent->dstColorSpaceSpefication==grayScale)
        printf("colorSpaceSpefication=grayScale\n");

      if(PCLmPageContent->destinationResolution==res300)
        printf("destinationResolution Requested=300 DPI\n");
      else if(PCLmPageContent->destinationResolution==res600)
        printf("destinationResolution Requested=600 DPI\n");
      else if(PCLmPageContent->destinationResolution==res1200)
        printf("destinationResolution Requested=1200 DPI\n");

      if(PCLmPageContent->duplexDisposition==simplex)
        printf("duplex disposition=Simplex\n");
      else if(PCLmPageContent->duplexDisposition==duplex_longEdge)
        printf("duplex disposition=Duplex_longEdge\n");
      else if(PCLmPageContent->duplexDisposition==duplex_shortEdge)
        printf("duplex disposition=Duplex_shortEdge\n");
       

    }

    if(strlen(PCLmPageContent->mediaSizeName))
      strcpy(currMediaName,PCLmPageContent->mediaSizeName);

    currStripHeight=PCLmPageContent->stripHeight;
    if(!currStripHeight)
    {
      numImageStrips=1;
      currStripHeight=currSourceHeight;
    }
    else
    {
      float numImageStripsReal=ceil((float)currSourceHeight/(float)currStripHeight); // Need to know how many strips will be inserted into PDF file
      numImageStrips=(int)numImageStripsReal;
      //if(topMarginInPix) // We will inject an extra image for the topMargin offset
      //  numImageStrips++;
    }

    if(PCLmPageContent->srcColorSpaceSpefication==grayScale)
      srcNumComponents=1;
    else
      srcNumComponents=3;

    if(PCLmPageContent->dstColorSpaceSpefication==grayScale)
      dstNumComponents=1;
    else
      dstNumComponents=3;

    currDuplexDisposition=PCLmPageContent->duplexDisposition;

    destColorSpace=PCLmPageContent->dstColorSpaceSpefication;

    // Calculate how large the output buffer needs to be based upon the page specifications
    int tmp_outBuffSize=mediaWidthInPixels*currStripHeight*dstNumComponents;

    if(tmp_outBuffSize>currOutBuffSize)
    {
      *pOutBuffer=realloc(*pOutBuffer,tmp_outBuffSize);  // Realloc the pOutBuffer to the correct size

      if(*pOutBuffer==NULL)                              //realloc failed and prev buffer not freed
      {
        return errorOutAndCleanUp();
      }

      outBuffSize=currOutBuffSize=tmp_outBuffSize;
      allocatedOutputBuffer=*pOutBuffer;
      if(NULL==allocatedOutputBuffer)
      {
        return(errorOutAndCleanUp());
      }
      if(DebugIt2)
        printf("pOutBuffer: allocated %d bytes at 0x%lx\n",tmp_outBuffSize, (long int)*pOutBuffer);
    }

    initOutBuff((char*)*pOutBuffer,outBuffSize);

    if(DebugIt2)
      printf("Allocated %d for myOutBufferSize\n",outBuffSize);
 

    if(DebugIt)
      printf("numImageStrips=%d\n",numImageStrips);

    // Keep track of the page count
    pageCount++;

    // If we are on a backside and doing duplex, prep for reverse strip order
    if(currDuplexDisposition==duplex_longEdge && !(pageCount%2) && mirrorBackside)
    {
      if(DebugIt2)
        printf("genPCLm.cpp: setting reverseOrder\n");
      reverseOrder=true;
    }
    else
      reverseOrder=false;

    // Calculate the number of injected strips, if any
    if(topMarginInPix)
    {
      if(topMarginInPix<=currStripHeight)
      {
         numFullInjectedStrips=1;
         numFullScanlinesToInject=topMarginInPix;
         numPartialScanlinesToInject=0;
      }
      else
      {
         numFullInjectedStrips=topMarginInPix/currStripHeight;
         numFullScanlinesToInject=currStripHeight;
         numPartialScanlinesToInject=topMarginInPix - (numFullInjectedStrips*currStripHeight);
      }
    }

    writeJobTicket();
    writePDFGrammarPage(mediaWidthInPixels, mediaHeightInPixels, numImageStrips, destColorSpace);
    *iOutBufferSize=totalBytesWrittenToCurrBuff;

    if(!scratchBuffer)
    {
      // We need to pad the scratchBuffer size to allow for compression expansion (RLE can create
      // compressed segments that are slightly larger than the source.
      scratchBuffer=(ubyte*)malloc(currStripHeight*mediaWidthInPixels*srcNumComponents*2);
      if(!scratchBuffer)
        return(errorOutAndCleanUp());
      if(DebugIt2)
         printf("scrachBuffer: Allocated %d bytes at 0x%lx\n",currStripHeight*currSourceWidth*srcNumComponents, (long int) scratchBuffer);
    }

    mirrorBackside=PCLmPageContent->mirrorBackside;
    firstStrip=true;

    return(success);  
}

/* EndPage
 * 
 * /desc
 * /param  
 * /return 
 */
int  PCLmGenerator::EndPage(void **pOutBuffer, int *iOutBufferSize)
{
    *pOutBuffer = allocatedOutputBuffer;
    initOutBuff((char*)*pOutBuffer,outBuffSize);
    *iOutBufferSize=totalBytesWrittenToCurrBuff;

    // Free up the scratchbuffer at endpage, to allow the next page to be declared with a different
    // size.
    if(scratchBuffer)
    {
      free(scratchBuffer);
      scratchBuffer=NULL;
    }

    return(success);  
}

/* SkipLines
 * 
 * /desc
 * /param  
 * /return 
 */
int  PCLmGenerator::SkipLines(int iSkipLines)
{
    return(success);  
}

/* Encapsulate
 * 
 * /desc
 * /param  
 * /return 
 */
int  PCLmGenerator::Encapsulate(void *pInBuffer, int inBufferSize, int thisHeight, void **pOutBuffer, int *iOutBufferSize)
{
    int result=0, numCompBytes;
    int scanlineWidth=mediaWidthInPixels*srcNumComponents;
    // int numLinesThisCall=inBufferSize/(currSourceWidth*srcNumComponents);
    int numLinesThisCall=thisHeight;
    void *savedInBufferPtr=NULL;
    void *tmpBuffer=NULL;
    void *localInBuffer;
    ubyte *newStripPtr=NULL;

    // writeOutputFile(currSourceWidth*3*currStripHeight, (ubyte*)pInBuffer);

    if(leftoverScanlineBuffer)
    {
      ubyte *whereAreWe;
      sint32 scanlinesThisTime;
      // The leftover scanlines have already been processed (color-converted and flipped), so justscanlineWidth
      // put them into the output buffer.
      // Allocate a temporary buffer to copy leftover and new data into
      tmpBuffer=malloc(scanlineWidth*currStripHeight);
      if(!tmpBuffer)
        return(errorOutAndCleanUp());
 
      // Copy leftover scanlines into tmpBuffer
      memcpy(tmpBuffer,leftoverScanlineBuffer,scanlineWidth*numLeftoverScanlines);

      whereAreWe=(ubyte*)tmpBuffer+(scanlineWidth*numLeftoverScanlines);

      scanlinesThisTime=currStripHeight-numLeftoverScanlines;
   
      // Copy enough scanlines from the real inBuffer to fill out the tmpBuffer
      memcpy(whereAreWe,pInBuffer,scanlinesThisTime*scanlineWidth);

      // Now copy the remaining scanlines from pInBuffer to the leftoverBuffer
      if(DebugIt)
        printf("Leftover scanlines: numLinesThisCall=%d, currStripHeight=%d\n",numLinesThisCall,currStripHeight);
      numLeftoverScanlines=thisHeight-scanlinesThisTime;
      assert(leftoverScanlineBuffer);
      whereAreWe=(ubyte*)pInBuffer+(scanlineWidth*numLeftoverScanlines);
      memcpy(leftoverScanlineBuffer,whereAreWe,scanlineWidth*numLeftoverScanlines);
      numLinesThisCall=thisHeight=currStripHeight;

      savedInBufferPtr=pInBuffer;
      localInBuffer=tmpBuffer;
    }
    else
      localInBuffer=pInBuffer;


    if(thisHeight > currStripHeight)
    {
      // Copy raw raster into leftoverScanlineBuffer
      ubyte *ptr;
      if(DebugIt)
        printf("Leftover scanlines: numLinesThisCall=%d, currStripHeight=%d\n",numLinesThisCall,currStripHeight);
      numLeftoverScanlines=thisHeight-currStripHeight;
      leftoverScanlineBuffer=malloc(scanlineWidth*numLeftoverScanlines);
      if(!leftoverScanlineBuffer)
        return(errorOutAndCleanUp());
      ptr=(ubyte *)localInBuffer+scanlineWidth*numLeftoverScanlines; 
      memcpy(leftoverScanlineBuffer,ptr,scanlineWidth*numLeftoverScanlines);
      thisHeight=currStripHeight;
    }

    if(NULL==allocatedOutputBuffer)
    {
      return(errorOutAndCleanUp());
    }
    *pOutBuffer = allocatedOutputBuffer;
    initOutBuff((char*)*pOutBuffer,outBuffSize);

    if(currDuplexDisposition==duplex_longEdge && !(pageCount%2))
    {
      if(mirrorBackside)
        prepImageForBacksideDuplex((ubyte*)localInBuffer, numLinesThisCall, currSourceWidth, srcNumComponents);
    }

    if(destColorSpace==grayScale && (sourceColorSpace==deviceRGB || sourceColorSpace==adobeRGB))
    {
      colorConvertSource(sourceColorSpace, grayScale, (ubyte*)localInBuffer, currSourceWidth, numLinesThisCall);
      // Adjust the scanline width accordingly
      scanlineWidth = mediaWidthInPixels * dstNumComponents;
    }

    if(leftMarginInPix)
    {
      newStripPtr=shiftStripByLeftMargin((ubyte*)localInBuffer, currSourceWidth, currStripHeight, numLinesThisCall, mediaWidthInPixels, leftMarginInPix, destColorSpace);

      // newStripPtr=shiftStripByLeftMargin((ubyte*)localInBuffer, currSourceWidth, currStripHeight, mediaWidthInPixels, leftMarginInPix, destColorSpace);
#if 0
      if(sourceColorSpace==grayScale)
        writeOutputFile(currStripHeight*mediaWidthInPixels,(ubyte*)newStripPtr);
      else
        writeOutputFile(3*currStripHeight*mediaWidthInPixels,(ubyte*)newStripPtr);
#endif
    }

#ifdef SUPPORT_WHITE_STRIPS
    bool whiteStrip=isWhiteStrip(pInBuffer, thisHeight*currSourceWidth*srcNumComponents);
    if(DebugIt2)
    {
      if(whiteStrip)
        printf("Found white strip\n");
      else
        printf("Found non-white strip\n");
    }
#else
    bool whiteStrip=false;
#endif

    if(currCompressionDisposition==compressDCT)
    {
      if(firstStrip && topMarginInPix)
      {
         ubyte whitePt=0xff;

         ubyte *tmpStrip=(ubyte*)malloc(scanlineWidth*topMarginInPix);
         memset(tmpStrip,whitePt,scanlineWidth*topMarginInPix);

         for(sint32 stripCntr=0; stripCntr<numFullInjectedStrips;stripCntr++)
         {
           write_JPEG_Buff (scratchBuffer, JPEG_QUALITY, mediaWidthInPixels, (sint32)numFullScanlinesToInject, (JSAMPLE*)tmpStrip, currRenderResolutionInteger, destColorSpace, &numCompBytes);
           injectJPEG((char*)scratchBuffer, mediaWidthInPixels, (sint32)numFullScanlinesToInject, numCompBytes, destColorSpace, true /*white*/ );

         }

         if(numPartialScanlinesToInject)
         {
           // Handle the leftover strip
           write_JPEG_Buff (scratchBuffer, JPEG_QUALITY, mediaWidthInPixels, numPartialScanlinesToInject, (JSAMPLE*)tmpStrip, currRenderResolutionInteger, destColorSpace, &numCompBytes);
           injectJPEG((char*)scratchBuffer, mediaWidthInPixels, numPartialScanlinesToInject, numCompBytes, destColorSpace, true /*white*/ );

         }

         free(tmpStrip);
         firstStrip=false;
      }

      /*The "if 1" block below pads the incoming buffer from numLinesThisCall to strip_height assuming that it has 
      * that much extra space. Also "else" block after this "if 1" block was used before; when for
      * JPEG also, there was no padding to strip_height. Retaining it just in case, in future, the padding
      * has to be removed*/

#if 1
      // We are always going to compress the full strip height, even though the image may be less;
      // this allows the compressed images to be symetric
      if(numLinesThisCall<currStripHeight)
      {
        sint32 numLeftoverBytes=(currStripHeight-numLinesThisCall)*currSourceWidth*3;
        sint32 numImagedBytes  =numLinesThisCall*currSourceWidth*3;

        // End-of-page: we have to white-out the unused section of the source image
        memset((ubyte*)localInBuffer+numImagedBytes, 0xff, numLeftoverBytes);
      }

      if(newStripPtr)
      {
        write_JPEG_Buff (scratchBuffer, JPEG_QUALITY, mediaWidthInPixels, currStripHeight, (JSAMPLE*)newStripPtr, currRenderResolutionInteger, destColorSpace, &numCompBytes);

        free(newStripPtr);
      }
      else
      {
        write_JPEG_Buff (scratchBuffer, JPEG_QUALITY, mediaWidthInPixels, currStripHeight, (JSAMPLE*)localInBuffer, currRenderResolutionInteger, destColorSpace, &numCompBytes);
      }

      if(DebugIt2)
        writeOutputFile(numCompBytes, scratchBuffer);
      injectJPEG((char*)scratchBuffer, mediaWidthInPixels, currStripHeight, numCompBytes, destColorSpace, whiteStrip );


#else
      if(newStripPtr)
      {
        write_JPEG_Buff (scratchBuffer, JPEG_QUALITY, mediaWidthInPixels, numLinesThisCall, (JSAMPLE*)newStripPtr, currRenderResolutionInteger, destColorSpace, &numCompBytes);
        free(newStripPtr);
      }
      else
        write_JPEG_Buff (scratchBuffer, JPEG_QUALITY, mediaWidthInPixels, numLinesThisCall, (JSAMPLE*)localInBuffer, currRenderResolutionInteger, destColorSpace, &numCompBytes);
      if(DebugIt2)
        writeOutputFile(numCompBytes, scratchBuffer);
      injectJPEG((char*)scratchBuffer, mediaWidthInPixels, numLinesThisCall, numCompBytes, destColorSpace, whiteStrip );

#endif
    }
    else if(currCompressionDisposition==compressFlate)
    {
      uint32 len=numLinesThisCall*scanlineWidth;
      uLongf destSize=len;

      if(firstStrip && topMarginInPix)
      {
         ubyte whitePt=0xff;
 
         // We need to inject a blank image-strip with a height==topMarginInPix
         ubyte *tmpStrip=(ubyte*)malloc(scanlineWidth*topMarginInPix);
         uLongf tmpDestSize=destSize;
         memset(tmpStrip,whitePt,scanlineWidth*topMarginInPix);

         for(sint32 stripCntr=0; stripCntr<numFullInjectedStrips;stripCntr++)
         {
           result=compress((Bytef*)scratchBuffer,&tmpDestSize, (const Bytef*)tmpStrip, scanlineWidth*numFullScanlinesToInject);
           injectLZStrip((ubyte*)scratchBuffer, tmpDestSize, mediaWidthInPixels, numFullScanlinesToInject, destColorSpace, true /*white*/ );
         }
         if(numPartialScanlinesToInject)
         {
           result=compress((Bytef*)scratchBuffer,&tmpDestSize, (const Bytef*)tmpStrip, scanlineWidth*numPartialScanlinesToInject);
           injectLZStrip((ubyte*)scratchBuffer, tmpDestSize, mediaWidthInPixels, numPartialScanlinesToInject, destColorSpace, true /*white*/ );

         }
         free(tmpStrip);
         firstStrip=false;
      }
 
      if(newStripPtr)
      {
        result=compress((Bytef*)scratchBuffer,&destSize,(const Bytef*)newStripPtr,scanlineWidth*numLinesThisCall);
        if(DebugIt2)
          writeOutputFile(destSize, scratchBuffer);
        if(DebugIt2)
        {
          printf("Allocated zlib dest buffer of size %d\n",numLinesThisCall*scanlineWidth);
          printf("zlib compression return result=%d, compSize=%d\n",result,(int)destSize);
        }
        free(newStripPtr);
      }
      else
      {
        // Dump the source data
        // writeOutputFile(scanlineWidth*numLinesThisCall, (ubyte *)localInBuffer);
        result=compress((Bytef*)scratchBuffer, &destSize, (const Bytef*)localInBuffer, scanlineWidth*numLinesThisCall);
        if(DebugIt2)
          writeOutputFile(destSize, scratchBuffer);
        if(DebugIt2)
        {
          printf("Allocated zlib dest buffer of size %d\n",numLinesThisCall*scanlineWidth);
          printf("zlib compression return result=%d, compSize=%d\n",result,(int)destSize);
        }
      }
      injectLZStrip(scratchBuffer,destSize, mediaWidthInPixels, numLinesThisCall, destColorSpace, whiteStrip);
    }

    else if(currCompressionDisposition==compressRLE)
    {
#ifdef RLE_SUPPORTED
      int compSize;
      if(firstStrip && topMarginInPix)
      {
         ubyte whitePt=0xff;
 
         // We need to inject a blank image-strip with a height==topMarginInPix

         ubyte *tmpStrip=(ubyte*)malloc(scanlineWidth*topMarginInPix);
         memset(tmpStrip,whitePt,scanlineWidth*topMarginInPix);

         for(sint32 stripCntr=0; stripCntr<numFullInjectedStrips;stripCntr++)
         {
           compSize=RLEEncodeImage((ubyte*)tmpStrip, scratchBuffer, scanlineWidth*numFullScanlinesToInject);
          injectRLEStrip((ubyte*)scratchBuffer, compSize, mediaWidthInPixels, numFullScanlinesToInject, destColorSpace, true /*white*/);


         }

         if(numPartialScanlinesToInject)
         {
           compSize=RLEEncodeImage((ubyte*)tmpStrip, scratchBuffer, scanlineWidth*numPartialScanlinesToInject);
           injectRLEStrip((ubyte*)scratchBuffer, compSize, mediaWidthInPixels, numPartialScanlinesToInject, destColorSpace, true /*white*/);

         }

         free(tmpStrip);
         firstStrip=false;
      }
 
      if(newStripPtr)
      {
        compSize=RLEEncodeImage((ubyte*)newStripPtr,   scratchBuffer, scanlineWidth*numLinesThisCall);
        free(newStripPtr);
      }
      else
        compSize=RLEEncodeImage((ubyte*)localInBuffer, scratchBuffer, scanlineWidth*numLinesThisCall);

      if(DebugIt2)
      {
        printf("Allocated rle dest buffer of size %d\n",numLinesThisCall*scanlineWidth);
        printf("rle compression return size=%d=%d\n",result,(int)compSize);
      }
      injectRLEStrip(scratchBuffer, compSize, mediaWidthInPixels, numLinesThisCall, destColorSpace, whiteStrip);
#else
      assert(0);
#endif
    }
    else
      assert(0);

    *iOutBufferSize=totalBytesWrittenToCurrBuff;

    if(savedInBufferPtr)
      pInBuffer=savedInBufferPtr;

    if(tmpBuffer)
      free(tmpBuffer);

    return(success);  
}

int PCLmGenerator::get_pclm_media_dimensions(const char *mediaRequested, PCLmPageSetup *myPageInfo)
{


	  int mediaNumEntries=sizeof(MasterMediaSizeTable)/sizeof(struct MediaSizeTableElement);
	  int            i=0;
	  int result = 99;


	  int iRenderResolutionInteger = 0;
	  if(myPageInfo->destinationResolution==res300)
        iRenderResolutionInteger=300;
      else if(myPageInfo->destinationResolution==res600)
        iRenderResolutionInteger=600;
      else if(myPageInfo->destinationResolution==res1200)
        iRenderResolutionInteger=1200;
      else
        assert(0);

	  do
	  {
		  if(myPageInfo == NULL)
		  {
			  continue;
		  }

		  for(i=0; i<mediaNumEntries; i++)
		  {
		    if(strcasecmp(mediaRequested,MasterMediaSizeTable[i].PCL6Name)==0)
			{
				myPageInfo->mediaWidth      =  floorf(_MI_TO_POINTS(MasterMediaSizeTable[i].WidthInInches));
				myPageInfo->mediaHeight     = floorf(_MI_TO_POINTS(MasterMediaSizeTable[i].HeightInInches));
				myPageInfo->mediaWidthInPixels   = floorf(_MI_TO_PIXELS(MasterMediaSizeTable[i].WidthInInches, iRenderResolutionInteger));
				myPageInfo->mediaHeightInPixels   = floorf(_MI_TO_PIXELS(MasterMediaSizeTable[i].HeightInInches, iRenderResolutionInteger));

				result = i;

				if(DebugIt){
				     printf("PCLmGenerator get_pclm_media_size(): match found: %s, %s\n", mediaRequested, MasterMediaSizeTable[i].PCL6Name);
				     printf("PCLmGenerator Calculated mediaWidth=%f\n",myPageInfo->mediaWidth);
				     printf("PCLmGenerator Calculated mediaHeight=%f\n",myPageInfo->mediaHeight);


				}


				break;  // we found a match, so break out of loop
			}
		  }
	  }
	  while(0);

	  if (i == mediaNumEntries)
	  {
		  // media size not found, defaulting to letter
		  printf("PCLmGenerator get_pclm_media_size(): media size, %s, NOT FOUND, setting to letter", mediaRequested);
		  result = get_pclm_media_dimensions("LETTER", myPageInfo);
	  }

	  return(result);
}


/* FreeBuffer
 * 
 * /desc
 * /param  
 * /return 
 */
void PCLmGenerator::FreeBuffer(void *pBuffer)
{
  if(jobOpen==job_closed && pBuffer)
  {
    if(pBuffer==allocatedOutputBuffer)
    {
      allocatedOutputBuffer=NULL;
    }
    free(pBuffer);
  }
  pBuffer=NULL;
}


