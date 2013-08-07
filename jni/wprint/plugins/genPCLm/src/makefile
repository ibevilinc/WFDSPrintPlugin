export LD_LIBRARY_PATH=/usr/lib/i386-linux-gnu
#export LD_LIBRARY_PATH=/usr/lib64
#export LD_DEBUG=all
GLIB_INCS = $(shell pkg-config --cflags glib-2.0 gthread-2.0 gobject-2.0)
GLIB_INC_x86 = -m32 $(GLIB_INCS)
M32       = $(GLIB_INC_x86)
CC         = /usr/bin/g++
#CC        = c++ 
#CFLAGS    = -L /usr/lib64 -m64 -g -I../inc -Wall -Werror
CFLAGS    = -L /usr/lib/i386-linux-gnu $(M32) -g -I../inc -Wall
LIBS      = -lpng -ljpeg -lz
#LIBS      = -l jpeg -l z -l png
#LIBS      = -l jpeg -l z -l tiffxx
#LIBS      = -l jpeg -l z 
SRC_FILES = genJPEGStrips.cpp genPCLm.cpp genPCLmTestHarness.c png.c
#SRC_FILES = genJPEGStrips.cpp myrle.c genPCLm.cpp genPCLmTestHarness.c 
#SRC_FILES = genJPEGStrips.c myrle.c genPCLm.c
INC       = ../inc/

all: genPCLm

genPCLm: $(SRC_FILES) $(INC)common_defines.h $(INC)genPCLm.h $(INC)media.h $(INC)PCLmGenerator.h $(INC)flate_colorspace.h
	$(CC) $(CFLAGS) $(LIBS) $(SRC_FILES) -o genPCLm

tarit: 
	cd ..;tar -cvzf ./genPCLm_tar.tar src/genPCLmTestHarness.c src/genPCLm.cpp \
	src/genJPEGStrips.c src/genJPEGStrips.cpp inc/genPCLm.h inc/flate_colorspace.h src/makefile src/README \
	src/flate_colorspace.bin testfiles/HiResolutionTrain.jpg testfiles/testit inc/PCLmGenerator.h \
	inc/common_defines.h inc/media.h src/myrle.c inc/genPCLm_proto.h src/png.c testfiles/testit.ini

release: 
	inc_ver_num
	cd ..;tar -cvzf ./genPCLm_tar.tar src/genPCLmTestHarness.c src/genPCLm.cpp \
	src/genJPEGStrips.c src/genJPEGStrips.cpp inc/genPCLm.h inc/flate_colorspace.h src/makefile src/README \
	src/flate_colorspace.bin testfiles/HiResolutionTrain.jpg testfiles/testit inc/PCLmGenerator.h \
	inc/common_defines.h inc/media.h src/myrle.c inc/genPCLm_proto.h src/png.c testfiles/testit.ini


clean: 
	rm -f jpeg_chunk* genPCLm rleFile*

doxygen:
	doxygen genPCLm.cpp


