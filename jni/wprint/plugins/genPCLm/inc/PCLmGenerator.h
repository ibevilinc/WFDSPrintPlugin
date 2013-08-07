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
#ifndef _PCLM_GENERATOR
#define _PCLM_GENERATOR
#define SUPPORT_WHITE_STRIPS

#include "common_defines.h"

class PCLmGenerator
{
public:

    PCLmGenerator();
    ~PCLmGenerator();

#ifdef STANDALONE
    int StartJob    (void **pOutBuffer, int *iOutBufferSize, bool debug);
#else
    int StartJob    (void **pOutBuffer, int *iOutBufferSize);
#endif
    int EndJob      (void **pOutBUffer, int *iOutBifferSize);
    int StartPage   (PCLmPageSetup *PCLmPageContent, void **pOutBuffer, int *iOutBufferSize);
    int StartPage   (PCLmSSetup *PCLmSPageContent, bool generatePCLmS, void **pOutBuffer, int *iOutBufferSize);
    int EndPage     (void **pOutBuffer, int *iOutBufferSize);
    int Encapsulate (void *pInBuffer, int inBufferSize, int numLines, void **pOutBuffer, int *iOutBufferSize);
    int get_pclm_media_dimensions(const char *mediaRequested, PCLmPageSetup *myPageInfo);
    int SkipLines   (int iSkipLines);
    void FreeBuffer (void *pBuffer);

private:
    bool colorConvertSource(colorSpaceDisposition srcCS, colorSpaceDisposition dstCS, ubyte *strip, sint32 stripWidth, sint32 stripHeight);
    void writePDFGrammarPage(int imageWidth, int imageHeight, int numStrips, colorSpaceDisposition destColorSpace);
    void writePDFGrammarHeader();
    int injectRLEStrip(ubyte *RLEBuffer, int numBytes, int imageWidth, int imageHeight, colorSpaceDisposition destColorSpace, bool);
    int injectLZStrip(ubyte *LZBuffer, int numBytes, int imageWidth, int imageHeight, colorSpaceDisposition destColorSpace, bool);
    int injectJPEG(char *jpeg_Buff, int imageWidth, int imageHeight, int numCompBytes, colorSpaceDisposition destColorSpace, bool);
    void initOutBuff(char *buff, sint32 size);
    void writeStr2OutBuff(char *str);
    void write2Buff(ubyte *buff, int buffSize);
    int statOutputFileSize();
    void writePDFGrammarTrailer(int imageWidth, int imageHeight);
    bool injectAdobeRGBCS();
    bool addKids(sint32 kidObj);
    bool addXRef(sint32 xRefObj);
    void fixXRef();
    int errorOutAndCleanUp();
    void Cleanup(void);
    void writeJobTicket(void);
    void injectImageTransform();
#ifdef SUPPORT_WHITE_STRIPS
    bool isWhiteStrip(void *, int);
    void injectWhiteData(void);
#endif
    bool getInputBinString(jobInputBin bin, char *);
    bool getOutputBin(jobOutputBin bin, char *);
    int RLEEncodeImage(ubyte *in, ubyte *out, int inLength);

    sint32                      currStripHeight;
    char                        currMediaName[256];
    duplexDispositionEnum       currDuplexDisposition;
    colorSpaceDisposition       currColorSpaceDisposition;
    debugDisposition            currDebugDisposition;
    compressionDisposition      currCompressionDisposition;
    mediaOrientationDisposition currMediaOrientationDisposition;
    renderResolution            currRenderResolution;
    int                         currRenderResolutionInteger;
    void                        *allocatedOutputBuffer;
    void                        *leftoverScanlineBuffer;

    int                         mediaWidth;
    int                         mediaHeight;
    int                         mediaWidthInPixels;
    int                         mediaHeightInPixels;
    colorSpaceDisposition       destColorSpace;
    colorSpaceDisposition       sourceColorSpace;
    int                         scaleFactor;
    bool                        genExtraPage;
    jobStateEnum                jobOpen;
    int                         currSourceWidth;
    int                         currSourceHeight;
    int                         srcNumComponents;
    int                         dstNumComponents;
    int                         numLeftoverScanlines;
    ubyte                       *scratchBuffer;
    int                         pageCount;
    bool                        reverseOrder;
    int                         outBuffSize;
    int                         currOutBuffSize;
    int                         totalBytesWrittenToPCLmFile;
    int                         totalBytesWrittenToCurrBuff;
    char                        *outBuffPtr;
    char                        *currBuffPtr;
    sint32                      currResolution;
    float                       STANDARD_SCALE;
    sint32                      DebugIt;
    sint32                      DebugIt2;
    sint32                      objCounter;

    sint32                      yPosition;
    sint32                      infoObj;
    sint32                      pageOrigin;
    sint32                      *KidsArray;
    sint32                      numKids;
// XRefTable storage 
    sint32                      *xRefTable;
    sint32                      xRefIndex;
    sint32                      xRefStart;
    char                        pOutStr[256];
    bool                        adobeRGBCS_firstTime;
    bool                        mirrorBackside;
    sint32                      topMarginInPix;
    sint32                      leftMarginInPix;
    bool                        firstStrip;
    sint32                      numFullInjectedStrips;
    sint32                      numFullScanlinesToInject;
    sint32                      numPartialScanlinesToInject;

    PCLmSUserSettingsType *m_pPCLmSSettings;
    char returnStr[256];
};

#endif /* _PCLM_PARSER_ */



