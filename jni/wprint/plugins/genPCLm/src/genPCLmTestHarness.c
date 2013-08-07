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
#include "PCLmGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#include <getopt.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common_defines.h"
#include "genPCLm.h"
#include "ctype.h"
#include "media.h"
#include "genPCLm_proto.h"
#include <png.h>

#define STRIP_HEIGHT 64
#define LANDSCAPE 0
#define PORTRAIT  1

#define rgb_2_gray(r,g,b) (ubyte)(0.299*(double)r+0.587*(double)g+0.114*(double)b)

duplexDispositionEnum currDuplex=simplex;
colorSpaceDisposition destColorSpace;
static sint32         currResolution=600;
static float          STANDARD_SCALE=(float)currResolution/(float)72;
float                 mediaWidthInPixels=8.5*currResolution;
float                 mediaHeightInPixels=11.0*currResolution;
colorSpaceDisposition sourceColorSpace=deviceRGB;
static float          mediaWidth=mediaWidthInPixels/8.33, mediaHeight=mediaHeightInPixels/8.33; // Default width and height to Letter size
sint32                DebugIt=0, DebugIt2=0;
static sint32         scaleFactor=1;
static boolean        genExtraPage=false;
static compTypeEnum   cType=jpeg;
static char           compType[20]={"jpeg"};
static sint32         setStripHeight=STRIP_HEIGHT,currStripHeight=64;
static sint32         orientation=0;
static boolean        renameOutFile=false;
char                  Pages[200][200];
sint32                currPage=0;
static int            numImageStrips;
static float          partialStrip=0.0;
// static char        overrideType[2]={'x'};
static int            scanlineWidth;
static char           tmpStr[100];
static sint32         currStripCnt=0;
static sint32         pageCount=0;
static FILE           *outputFile;
static ubyte          *myOutBuffer;
static boolean        mirrorBackside=false;
static char           additionalStr[40]="";
static float          leftMargin=0, topMargin=0;
static char           compSequence[200];
static boolean        compSequenceFlag=false;
static sint32         compSequenceStrSize=0;
static bool           usePCLmS=false;


static char mediaRequested[256]="letter";
static char outFileName[255];

typedef struct
{
  ubyte *data;
  sint32 cnt;
} stripContainerType;

#define STRIP_CONTAINER_SIZE 1000

stripContainerType stripContainer[STRIP_CONTAINER_SIZE];

PCLmGenerator  *myGenerator=NULL;

extern boolean prepImageForDuplex(ubyte *imagePtr, sint32 imageHeight, sint32 imageWidth, sint32 numComponents);

void printAllMediaSupported()
{
  int mediaNumEntries=sizeof(MasterMediaSizeTable)/sizeof(MediaSizeTableElement);
  int i;

  printf("Available media sizes supported by genPCLm are:\n");

  for(i=0;i<mediaNumEntries;i++)
    printf("%s:         w=%5.2f, h=%5.2f\n",MasterMediaSizeTable[i].PCL6Name,MasterMediaSizeTable[i].WidthInInches/1000.0, MasterMediaSizeTable[i].HeightInInches/1000.0);
}


void usage_blurb()
{
  printf("genPCLm version %f\n\n",PCLM_Ver);
  printf("Usage: genPCLm SourceImageFileName [-s imageScaleFactor] [-c compressionType] [-d] [-h StripHeight]\n");

  printf("  Where:\n");
  printf("    -f fname:           Specifies the file(s) to use as input (jpeg); specify multiple -f options for multiple pages\n");
  printf("    -c compressionType  Output compression format: [jpeg|rle|zlib] - default=jpeg\n");
  printf("    -C str              Cause genPCLm to generate each page with a different compression method.  Str indicates the compression method sequence to use\n");
  printf("    -h stripHeight      Allows strip height definition - default=16 scanlines \n");
  printf("    -m mediaName        Specify the media size to use\n");
  printf("    -p lMargin          Specify the printableMargin to use for rendering (floating point)\n");
  printf("    -r resolution       Specifies the output resolution\n");
  printf("    -z colorSpace       Specifies the output color space for the document [RGB|AdobeRGB|GRAY]\n");
  printf("    -n                  Specifies naming of output file: media_resolution_compression_stripHeight_colorSpace\n");
  printf("    -u [s|l]            Configure genPCLm to generate duplex oriented output for odd\n");
  printf("                           pages; 's' specifies short-edge, 'l' specifies long-edge\n");
  printf("    -w                  Specifies that genPCLm generate mirrored backside-duplex images\n");
  printf("    -e                  Specifies genPCLm inject blank page if duplexing and page count is odd\n");
  printf("    -x str              Specifies the addition of STR to the output name; only valid with -n\n");
  printf("    -y                  Specifies genPCLm generate PCLmS job ticket \n");

  printf("    \nDebug Options:\n");
  printf("    -d                  Turn on debugging option\n");
  printf("    -o                  Overrides image orientation - use 'p' for portrait and 'l' landscape\n");
  printf("    -s factor           Allows scaling of the PDF file\n");
  printf("    -v                  genPCLm version\n");
  printf("    -a                  Prints a list of all media sizes supported by genPCLm\n");
  printf("    -?                  Prints help and version\n");

  exit(-1);
}

boolean renameTheOutFileStr(colorSpaceDisposition destColorSpace)
{
  strcpy(outFileName,"PCLm_");
  
  if(strlen(mediaRequested))
    strcat(outFileName,mediaRequested);
  else
    strcat(outFileName,"letter");

  sprintf(tmpStr,"_%d_",currResolution);
  strcat(outFileName,tmpStr);

  if(compSequenceFlag)
  {
    sprintf(tmpStr,"C%s_",compSequence);
    strcat(outFileName,tmpStr);
  }
  else
  {
    sprintf(tmpStr,"c%s_",compType);
    strcat(outFileName,tmpStr);
  }

  sprintf(tmpStr,"H%d_",currStripHeight);
  strcat(outFileName,tmpStr);

  sprintf(tmpStr,"PgCnt%d",currPage);
  strcat(outFileName,tmpStr);

  if(destColorSpace==deviceRGB)
  {
    sprintf(tmpStr,"_RGB");
    strcat(outFileName,tmpStr);
  }
  else if(destColorSpace==adobeRGB)
  {
    sprintf(tmpStr,"_ADOBE-RGB");
    strcat(outFileName,tmpStr);
  }
  else
  {
    sprintf(tmpStr,"_GRAY");
    strcat(outFileName,tmpStr);
  }

  if(leftMargin || topMargin)
  {
    sprintf(tmpStr,"_P%0.1f",leftMargin*(600.0/72.0));
    strcat(outFileName,tmpStr);
  }  
 
  if(currDuplex==duplex_longEdge)
  {
    sprintf(tmpStr,"_duplexLong");
    strcat(outFileName,tmpStr);
  }
  else if(currDuplex==duplex_shortEdge)
  {
    sprintf(tmpStr,"_duplexShort");
    strcat(outFileName,tmpStr);
  }

  if(currDuplex!=simplex)
  {
    if(mirrorBackside)
    {
      sprintf(tmpStr,"_mirror");
      strcat(outFileName,tmpStr);
    }
    else
    {
      sprintf(tmpStr,"_noMirror");
      strcat(outFileName,tmpStr);
    }
  }

  if(strlen(additionalStr))
  {
    sprintf(tmpStr,"__%s",additionalStr);
    strcat(outFileName,tmpStr);
  }

  strcat(outFileName,".pdf");

  return(true);
}

boolean saveStrip(ubyte *buffPtr, sint32 buffSize)
{
  stripContainer[currStripCnt].data=(ubyte*)malloc(buffSize);
  memcpy(stripContainer[currStripCnt].data,buffPtr,buffSize);
  stripContainer[currStripCnt].cnt=buffSize;
  currStripCnt++;

  if(currStripCnt>=STRIP_CONTAINER_SIZE)
    assert(0);
  return(true);
}

boolean putBufferedStrips()
{
#define OLDWAY
#ifdef OLDWAY
  currStripCnt--;
// printf("currStripCnt=%d\n",currStripCnt);
  for(int i=currStripCnt;i>=0;i--)
  {
     fwrite(stripContainer[i].data,stripContainer[i].cnt,1,outputFile);
  }
#else
// printf("currStripCnt=%d\n",currStripCnt);
  for(int i=0;i<currStripCnt;i++)
  {
     fwrite(stripContainer[i].data,stripContainer[i].cnt,1,outputFile);
  }
#endif
  currStripCnt=0;
  return(true);
}


void figureOutMediaSize(const char *mediaRequested)
{
  int mediaNumEntries=sizeof(MasterMediaSizeTable)/sizeof(MediaSizeTableElement);
  int i;
  float width=0,height=0;

  for(i=0;i<mediaNumEntries;i++)
  {
    //printf("%s\n",MasterMediaSizeTable[i].PCL6Name);
    if(strcasecmp(mediaRequested,MasterMediaSizeTable[i].PCL6Name)==0)
    {
      // (8500 / 1000) 
      width =(((MasterMediaSizeTable[i].WidthInInches/1000.0)*(float)currResolution)/STANDARD_SCALE);
      mediaWidthInPixels=(MasterMediaSizeTable[i].WidthInInches/1000.0)*currResolution;
      mediaHeightInPixels=(MasterMediaSizeTable[i].HeightInInches/1000.0)*currResolution;
      height=(((MasterMediaSizeTable[i].HeightInInches/1000.0)*(float)currResolution)/STANDARD_SCALE);
      mediaWidth=width;
      mediaHeight=height;
      if(DebugIt)
      {
        printf("match found: %s, %s\n",mediaRequested,MasterMediaSizeTable[i].PCL6Name);
        printf("Calculated mediaWidth=%f\n",mediaWidth);
        printf("Calculated mediaHeight=%f\n",mediaHeight);
      }
      break;  // we found a match, so break out of loop
    }
  }
  if(width==0)
  {
    printf("No media match found, defaulting to letter\n");
  }
}

int parseInput(int argc, char *argv[])
{
  int c;
  int i;
  /* Parse input */

  while ((c = getopt(argc, argv, "s:dc:C:h:o:?f:vm:ar:z:nu:ewx:p:y")) != EOF)
  {
    switch (c)
    {
      case 'u': 
        if(strcasecmp(optarg,"s")==0)
          currDuplex=duplex_shortEdge;
        else if(strcasecmp(optarg,"l")==0)
          currDuplex=duplex_longEdge;
        else
        {
          printf(" warning: no specification for long|short edge, assuming simplex\n");
          currDuplex=simplex;
        }
        break;
      case 'z':
        if(strcasecmp(optarg,"rgb")==0)
          destColorSpace=deviceRGB; 
        else if(strcasecmp(optarg,"adobergb")==0)
          destColorSpace=adobeRGB; 
        else if(strcasecmp(optarg,"gray")==0)
          destColorSpace=grayScale; 
        else
        {
          printf("Invalid color space setting; supported values are 'RGB', 'AdobeRGB' and 'gray'\n");
          exit(-1);
        }
        break;
      case 'r':
        {
          sint32 oldResolution=currResolution;
          currResolution=atoi(optarg);
          STANDARD_SCALE=(float)currResolution/(float)72;
          mediaWidthInPixels=(mediaWidthInPixels/oldResolution)*currResolution;
          mediaHeightInPixels=(mediaHeightInPixels/oldResolution)*currResolution;
        }
        break;
      case 's':
        scaleFactor=atoi(optarg);
        break;
      case 'd':
        DebugIt=1;
        break;
      case 'e':
        genExtraPage=true;
        break;
      case 'c':
        strcpy(compType,optarg);
        // printf("Found compType %s\n",compType);
        if(strcasecmp(compType,"jpeg")==0)
          cType=jpeg;
        else if(strcasecmp(compType,"zlib")==0)
          cType=zlib;
        else if(strcasecmp(compType,"rle")==0)
          cType=rle;
        else
          assert(1);
        break;
      case 'C':
        strcpy(compSequence,optarg);
        compSequenceFlag=true;
        compSequenceStrSize=strlen(compSequence);
        break;
      case 'h':
        setStripHeight=atoi(optarg);
        currStripHeight=setStripHeight;
        break;
      case 'v':
        printf("genPCLm version: %f\n",PCLM_Ver);
        exit(0);
        break;
      case 'o':
        if(optarg[0]=='l')
          orientation=LANDSCAPE;
        else if(optarg[0]=='p')
          orientation=PORTRAIT;
        else
        {
          printf("Illegial orientation type, exiting\n");
          exit(-1);
        }
        break;
      case 'a':
        printAllMediaSupported();
        exit(0);
        break;
      case 'n':
        renameOutFile=true;
        // Specification to rename output to input-attributes
        break;
      case 'p':
        // Media Named
        leftMargin=atoi(optarg);
        topMargin=atoi(optarg);
        if(DebugIt)
        {
          printf("left margin=%f\n",leftMargin);
          printf("top margin=%f\n",topMargin);
        }
        break;
      case 'f':
        strcpy(Pages[currPage],optarg);
        currPage++;
        break;
      case 'w':
        mirrorBackside=true;
        break;
      case 'x':
        strcpy(additionalStr,optarg);
        break;
      case 'm':
        strcpy(mediaRequested,optarg);
        break;
      case 'y':
        usePCLmS=true;
        break;
      case '?':
        usage_blurb();
        exit(0);
        break;
    }
  }

  if(currPage==0)
  {
    printf("Must supply at least one jpeg file as input\n");
    usage_blurb();
    exit(-1);
  }

  if(DebugIt)
  {
    printf("Generating %d page(s) in PDF file.\n",currPage);
    for(i=0;i<currPage;i++)
      printf(" Page %i=%s\n",i+1,Pages[i]);
  }
  return(1);
}

ubyte *scaleToFit(ubyte *strip, sint32 origWidth, float *float_newWidth, sint32 origHeight, float *float_newHeight, sint32 numComponents)
{
  ubyte *origP=strip;
  sint32 newWidth=(sint32)*float_newWidth;
  sint32 newHeight=(sint32)*float_newHeight;
  // assert(newWidth>origWidth);
  // assert(newHeight>origHeight);
  sint32 newScanlineWidth=newWidth * numComponents;
  sint32 oldScanlineWidth=origWidth* numComponents;
  ubyte *newAlloc=(ubyte *)malloc(newScanlineWidth*(newHeight+64));
  ubyte *newP=newAlloc;


  if(newWidth<origWidth || newHeight<origHeight)
  {
    // the source image is smaller than the meida size, so just return the original image
    printf("warning: genPCLm does not support scaledown of source image yet\n");
    *float_newWidth=(float)origWidth;
    *float_newHeight=(float)origHeight;
    return(strip);
  }

  if(numComponents==3)
  {
    // Additive space, white point=0xff
    memset(newAlloc,0xff,newScanlineWidth*newHeight);
  }
  else
  {
    // Subtractive space, white point=0x0
    memset(newAlloc,0x0,newScanlineWidth);
  }

  // we only copy origHeight data
  for(int i=0;i<origHeight;i++)
  {
    memcpy(newP,origP,oldScanlineWidth);
    newP +=newScanlineWidth;
    origP+=oldScanlineWidth;
  }
  free(strip);
  return(newAlloc);
}

int statImageFile(char * jpeg_filename, int *fileSize)
{
  struct stat statBuff;

  stat(jpeg_filename,&statBuff);
  if(DebugIt2)
    fprintf(stderr, "%s image file is %d bytes in size\n",jpeg_filename, (int)statBuff.st_size);
  *fileSize=statBuff.st_size;
  return(1);
}


// Read JPEG file; image will be allocated and deposited in myImageBuffer.

boolean readInputImage()
{
    char           imageFileName[255];
    static sint32         thisPage=0;
    int            result, imageFileSize;


    // Read in image file
    strcpy(imageFileName,Pages[thisPage]);
    thisPage++;
    if(strcasestr(imageFileName,".jpg"))
    {
      // Input is JPEG
      result=statImageFile(imageFileName, &imageFileSize);
      if(!result)
      {
        fprintf (stderr, "could not stat jpeg file\n");
        exit(-1);
      }
      result=read_JPEG_file (imageFileName);
  
      if(!result)
      {
            fprintf (stderr, "could not open jpeg file\n");
            exit(-1);
      }
  
      if(image_numComponents==3)
        sourceColorSpace=deviceRGB;
      else
        sourceColorSpace=grayScale;

      // NEED TO CHANGE imageFileSize TO THE NEW JPEG FILESIZE
      if(currStripHeight==0)
      {
        numImageStrips=1;
        currStripHeight=image_height;
        partialStrip=1;
      }
      else
      {
        float numImageStripsReal=ceil((float)image_height/(float)currStripHeight); // Need to know how many strips will be inserted into PDF file
        partialStrip=(float)image_height/(float)currStripHeight;
        partialStrip-=(int)partialStrip;
        numImageStrips=(int)numImageStripsReal;
      }
      if(DebugIt2)
        printf("numImageStrips=%d\n",numImageStrips);
  
      scanlineWidth=image_width*image_numComponents;
    }
    else if(strcasestr(imageFileName,".png"))
    {
      ubyte *pngPtr, color_type, bit_depth;
      // Input file is a PNG file
      printf("PNG File\n");
      extern void read_png_file(char* file_name);
      PCLm_read_png_file(imageFileName, &pngPtr, &image_width, &image_height, &color_type, &bit_depth);

      if(color_type==PNG_COLOR_TYPE_RGB)
      {
        sourceColorSpace=deviceRGB;
        image_numComponents=3;
      }
      else if(color_type==PNG_COLOR_TYPE_GRAY)
      {
        sourceColorSpace=grayScale;
        image_numComponents=1;
      }
      else
        assert(0);

      if(currStripHeight==0)
      {
        numImageStrips=1;
        currStripHeight=image_height;
        partialStrip=1;
      }
      else
      {
        float numImageStripsReal=ceil((float)image_height/(float)currStripHeight); // Need to know how many strips will be inserted into PDF file
        partialStrip=(float)image_height/(float)currStripHeight;
        partialStrip-=(int)partialStrip;
        numImageStrips=(int)numImageStripsReal;
      }
      if(DebugIt2)
        printf("numImageStrips=%d\n",numImageStrips);
  
      scanlineWidth=image_width*image_numComponents;
      myImageBuffer=pngPtr;
    }
    else
      assert(0);
    return(true);
}

extern boolean writeOutputFile(int numBytes, ubyte *ptr);

int main(int argc, char *argv[])
{

#if 0
    int result, numCompBytes;
    int scanlineWidth;
    char tmpStr[100];
    int compSize;
#endif

  uint32 numLinesToSend;
  ubyte *srcImageBuffer;
  boolean stripContent=1;
  int PageCount=0;
  int tmpNumLinesToSend;


  int result,inBuffSize;
  int outBuffSize=0;
  // ubyte *inBuffer;

  myGenerator=new PCLmGenerator;

  // fileIO=true;

  result=parseInput(argc, argv);

  strcpy(outFileName,"t.pdf");

  // If user specified outputFile rename, do it now
  if(renameOutFile)
  {
    renameTheOutFileStr(destColorSpace);
  }

  if(!result)
  {
    printf("unknown error - exiting\n");
    exit(-1);
  }

  figureOutMediaSize(mediaRequested);

  // Open output PDF file
  if (!(outputFile = fopen (outFileName, "w")))
  {
       fprintf (stderr, "Could not open the output file out - %s.\n", "t.pdf");
       exit (-1);
  }

  PCLmPageSetup myPageInfo;

  memset((void*)&myPageInfo,0x0,sizeof(PCLmPageSetup));
  strcpy(myPageInfo.mediaSizeName,mediaRequested);
  strcpy(myPageInfo.clientLocale,"USA");
  myPageInfo.mediaHeight=mediaHeight;
  myPageInfo.mediaWidth =mediaWidth;

  myPageInfo.mediaWidthOffset=leftMargin;
  myPageInfo.mediaHeightOffset=topMargin;


  myPageInfo.pageOrigin=top_left;
  if(cType==jpeg)
    myPageInfo.compTypeRequested=compressDCT;
  else if(cType==zlib)
    myPageInfo.compTypeRequested=compressFlate;
  else if(cType==rle)
    myPageInfo.compTypeRequested=compressRLE;
  else
    assert(0);

  if(destColorSpace==grayScale)
    myPageInfo.dstColorSpaceSpefication=grayScale;
  else if(destColorSpace==deviceRGB)
    myPageInfo.dstColorSpaceSpefication=deviceRGB;
  else if(destColorSpace==adobeRGB)
    myPageInfo.dstColorSpaceSpefication=adobeRGB;
  else
    assert(0);

  myPageInfo.stripHeight=setStripHeight;

  if(currResolution==300)
    myPageInfo.destinationResolution=res300;
  else if(currResolution==600)
    myPageInfo.destinationResolution=res600;
  else if(currResolution==1200)
    myPageInfo.destinationResolution=res1200;
  else
  {
    printf("Unknown resolution, exiting\n");
    exit(-1);
  }

  if(currDuplex==duplex_longEdge)
    myPageInfo.duplexDisposition=duplex_longEdge;
  else if(currDuplex==duplex_shortEdge)
    myPageInfo.duplexDisposition=duplex_shortEdge;
  else
    myPageInfo.duplexDisposition=simplex;

  myPageInfo.mirrorBackside=mirrorBackside;

  // Start a PCLm Job

#ifdef STANDALONE
  myGenerator->StartJob((void**)&myOutBuffer,&outBuffSize,DebugIt);
#else
  myGenerator->StartJob((void**)&myOutBuffer,&outBuffSize);
#endif
  fwrite(myOutBuffer,outBuffSize,1,outputFile);
  pageCount=0;

  for(int numPages=0;numPages<currPage;numPages++)
  {

    pageCount++;
    // Start PCLm Page

    currStripHeight=setStripHeight;

    numLinesToSend=currStripHeight;
    // numLinesToSend=96;

    readInputImage();

    myPageInfo.sourceHeight=(image_height/STANDARD_SCALE);
    myPageInfo.sourceWidth=(image_width/STANDARD_SCALE);

    if(sourceColorSpace==grayScale)
    {
      myPageInfo.colorContent=gray_content;
      myPageInfo.srcColorSpaceSpefication=grayScale;
    }
    else
    {
      myPageInfo.colorContent=color_content;
      myPageInfo.srcColorSpaceSpefication=deviceRGB;
    }

    if(compSequenceFlag)
    {
      int  pos=numPages%compSequenceStrSize;
      char c=compSequence[pos];
      if(c=='j')
        myPageInfo.compTypeRequested=compressDCT;
      else if(c=='r')
        myPageInfo.compTypeRequested=compressRLE;
      else if(c=='l')
        myPageInfo.compTypeRequested=compressFlate;
      else
        assert(0);
    }

    if(usePCLmS)
    {
      PCLmSUserSettingsType myUserSettings;
      PCLmSSetup myPCLmSSetup;

      myUserSettings.userCromaticMode=color_content;
      myUserSettings.userPageQuality=normal;
      myUserSettings.userOrientation=portraitOrientation;
      strcpy(myUserSettings.userMediaType,"plain");
      myUserSettings.userInputBin=tray_2;
      myUserSettings.userCopies=10;
      myUserSettings.userInputBin=tray_1;
      myUserSettings.userInputBin=tray_1;
      myUserSettings.userOutputBin=face_down;
      myUserSettings.userOutputBin=face_down;
      strcpy(myUserSettings.userDocumentName,"myJob");
      myUserSettings.userOutputBin=face_down;
  
      myPCLmSSetup.PCLmPageContent=&myPageInfo;
      myPCLmSSetup.PCLmSUserSettings=&myUserSettings;
      myGenerator->StartPage(&myPCLmSSetup,true,(void**)&myOutBuffer,&outBuffSize);
    }
    else
      myGenerator->StartPage(&myPageInfo,(void**)&myOutBuffer,&outBuffSize);
    fwrite(myOutBuffer,outBuffSize,1,outputFile);

    // reset stripHeight, as previous page could have set it to < setStripHeight
 
    // this is a counter for buffering strips
    currStripCnt=0;

    inBuffSize=currStripHeight*scanlineWidth;

    //writeOutputFile(image_height*image_width*3, (ubyte*)myImageBuffer);

    for(int i=0;i<numImageStrips;i++)
    {
      if(!currStripHeight)
            break; // Break out of loop when we run out of image data

      // printf("srcImageBuffer offset=%ld\n",srcImageBuffer-myImageBuffer);

      if(cType==jpeg)
      {
        tmpNumLinesToSend=numLinesToSend;
        if(partialStrip && i==numImageStrips-1)
        {
          // last strip
          tmpNumLinesToSend=(int)(partialStrip*currStripHeight);
        }
        srcImageBuffer=(ubyte *)myImageBuffer+(i*(numLinesToSend*scanlineWidth));
        numLinesToSend=tmpNumLinesToSend;
        if(DebugIt)
          sprintf(tmpStr,"jpeg_chunk%d.jpg",i);
        else
          sprintf(tmpStr,"jpeg_chunk.jpg");
    
    #undef ELIMINATE_WHITE_STRIPS
    #ifdef ELIMINATE_WHITE_STRIPS
        stripContent=1;
        for(int j=0;j<numLinesToSend*scanlineWidth;j++)
        {
          if(*(srcImageBuffer+j)!=0xff)
          {
            stripContent=0;
            // printf("image non white\n");
            // break;
          }
        }
        if(stripContent==0)
          if(DebugIt)
            printf("Found non-white strip\n");
        // printf("num White strips=%d\n",numWhitePix);
    #endif
    
        // printf("stripNum=%d, width=%d, height=%d\n",i,image_width, currStripHeight);
        if(!numLinesToSend)
          break; // Break out of loop when we run out of image data
        if(stripContent)
        {
          //writeOutputFile(numLinesToSend*scanlineWidth, (ubyte*)srcImageBuffer);
          //writeOutputFile(image_height*image_width*3, (ubyte*)myImageBuffer);

          myGenerator->Encapsulate(srcImageBuffer, inBuffSize, numLinesToSend, (void**)&myOutBuffer, &outBuffSize);
          if(currDuplex==duplex_longEdge && pageCount && !(pageCount%2) && mirrorBackside)
          {
            // Need to buffer strip to reverse order
            saveStrip(myOutBuffer,outBuffSize);
          }
          else
            fwrite(myOutBuffer,outBuffSize,1,outputFile);
 
        }
      }
      else if(cType==zlib)
      {
        tmpNumLinesToSend=numLinesToSend;
        if(partialStrip && i==numImageStrips-1)
        {
          // last strip
          tmpNumLinesToSend=(int)(partialStrip*currStripHeight);
        }
        srcImageBuffer=(ubyte *)myImageBuffer+(i*(numLinesToSend*scanlineWidth));
        numLinesToSend=tmpNumLinesToSend;
        // uint32 len=currStripHeight*scanlineWidth;
        // lzrw1a_compress(myImageBuffer, &len, srcImageBuffer, len);
        // uLongf destSize=len;
        if(!numLinesToSend)
          break; // Break out of loop when we run out of image data
        if(DebugIt)
        {
          int lSize=srcImageBuffer-myImageBuffer;
          printf("Encapsulate: src=0x%lx, size=%d\n",(long int)srcImageBuffer,lSize);
        }
        myGenerator->Encapsulate(srcImageBuffer, inBuffSize, numLinesToSend, (void**)&myOutBuffer, &outBuffSize);
        if(currDuplex==duplex_longEdge && pageCount && !(pageCount%2) && mirrorBackside)
        {
            // Need to buffer strip to reverse order
            saveStrip(myOutBuffer,outBuffSize);
        }
        else
            fwrite(myOutBuffer,outBuffSize,1,outputFile);
      }
      else if(cType==rle)
      {
        tmpNumLinesToSend=numLinesToSend;
        if(partialStrip && i==numImageStrips-1)
        {
          // last strip
          tmpNumLinesToSend=(int)(partialStrip*currStripHeight);
        }
        srcImageBuffer=(ubyte *)myImageBuffer+(i*(numLinesToSend*scanlineWidth));
        numLinesToSend=tmpNumLinesToSend;
    // strcpy((char *)srcImageBuffer,"ABCDEFGHIJKLMNOPAAAABBBB");
        if(!numLinesToSend)
          break; // Break out of loop when we run out of image data
        myGenerator->Encapsulate(srcImageBuffer, inBuffSize, numLinesToSend, (void**)&myOutBuffer,&outBuffSize);
        if(currDuplex==duplex_longEdge && pageCount && !(pageCount%2) && mirrorBackside)
        {
            // Need to buffer strip to reverse order
            saveStrip(myOutBuffer,outBuffSize);
        }
        else
            fwrite(myOutBuffer,outBuffSize,1,outputFile);
      }
    }

    // If we buffered the strips, output them now
    if(currStripCnt)
       putBufferedStrips();

    // End PCLm Page
    myGenerator->EndPage((void**)&myOutBuffer,&outBuffSize);
    fwrite(myOutBuffer,outBuffSize,1,outputFile);
    
    // Free the JPEG buffer (if allocated), in case the next page is different
    if(myImageBuffer)
      free(myImageBuffer);
    myImageBuffer=NULL;

    PageCount++;
  }

  if((currDuplex==duplex_longEdge || currDuplex==duplex_shortEdge) && genExtraPage && PageCount%2)
  {
    // Allocate 1 strip
    int stripSize=scanlineWidth*currStripHeight;
    ubyte *blankPgBuffer=(ubyte *)malloc(stripSize);
    memset((void*)blankPgBuffer,0xff,stripSize);

    myPageInfo.sourceHeight=currStripHeight/STANDARD_SCALE;
    myPageInfo.sourceWidth=(image_width/STANDARD_SCALE);

    myGenerator->StartPage(&myPageInfo,(void**)&myOutBuffer,&outBuffSize);
    fwrite(myOutBuffer,outBuffSize,1,outputFile);

    if(cType==jpeg)
    {
       myGenerator->Encapsulate(blankPgBuffer, stripSize, currStripHeight, (void**)&myOutBuffer, &outBuffSize);
    }
    else if(cType==zlib)
    {
        myGenerator->Encapsulate(blankPgBuffer, stripSize, currStripHeight, (void**)&myOutBuffer, &outBuffSize);
    }
    else if(cType==rle)
    {
        myGenerator->Encapsulate(blankPgBuffer, stripSize, currStripHeight, (void**)&myOutBuffer,&outBuffSize);
    }
    else
      assert(1);
    
    fwrite(myOutBuffer,outBuffSize,1,outputFile);

    if(DebugIt)
      printf("adding blank page %s\n",Pages[currPage]);
    PageCount++;

    myGenerator->EndPage((void**)&myOutBuffer,&outBuffSize);
    fwrite(myOutBuffer,outBuffSize,1,outputFile);
  }

  // End PCLm Job
  myGenerator->EndJob((void**)&myOutBuffer,&outBuffSize);
  fwrite(myOutBuffer,outBuffSize,1,outputFile);

  myGenerator->FreeBuffer(myOutBuffer);
  fclose(outputFile);

  delete(myGenerator);
  myGenerator=NULL;

  printf("Successfully created PCLm file %s\n",outFileName);
}
