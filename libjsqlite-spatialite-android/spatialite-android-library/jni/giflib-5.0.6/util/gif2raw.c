/*****************************************************************************

gif2raw - convert raw pixel data to a GIF

*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#ifdef _WIN32
#include <io.h>
#endif /* _WIN32 */

#include "getarg.h"
#include "gif_lib.h"

#define PROGRAM_NAME	"gif2raw"

static char
    *VersionStr =
	PROGRAM_NAME
	VERSION_COOKIE
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber.\n";
static char
    *CtrlStr =
	PROGRAM_NAME
	" v%- s%-Width|Height!d!d p%-ColorMapFile!s t%- h%- RawFile!*s";

static GifColorType EGAPalette[] =      /* Default color map is EGA palette. */
{
    {   0,   0,   0 },   /* 0. Black */
    {   0,   0, 170 },   /* 1. Blue */
    {   0, 170,   0 },   /* 2. Green */
    {   0, 170, 170 },   /* 3. Cyan */
    { 170,   0,   0 },   /* 4. Red */
    { 170,   0, 170 },   /* 5. Magenta */
    { 170, 170,   0 },   /* 6. Brown */
    { 170, 170, 170 },   /* 7. LightGray */
    {  85,  85,  85 },   /* 8. DarkGray */
    {  85,  85, 255 },   /* 9. LightBlue */
    {  85, 255,  85 },   /* 10. LightGreen */
    {  85, 255, 255 },   /* 11. LightCyan */
    { 255,  85,  85 },   /* 12. LightRed */
    { 255,  85, 255 },   /* 13. LightMagenta */
    { 255, 255,  85 },   /* 14. Yellow */
    { 255, 255, 255 },   /* 15. White */
};
#define EGA_PALETTE_SIZE (sizeof(EGAPalette) / sizeof(GifColorType))

static void Raw2Gif(int ImagwWidth, int ImagwHeight, ColorMapObject *ColorMap);
static void Gif2Raw(GifFileType *GifFile, bool Textify);

/******************************************************************************
 Interpret the command line, prepar global data and call the Gif routines.
******************************************************************************/
int main(int argc, char **argv)
{
    int	NumFiles, ImageWidth, ImageHeight, Dummy, Red, Green, Blue, ErrorCode;
    static bool Error,
	ImageSizeFlag = false, ColorMapFlag = false, HelpFlag = false,
	TextifyFlag = false;
    char **FileName = NULL, *ColorMapFile;

    if ((Error = GAGetArgs(argc, argv, CtrlStr, &GifNoisyPrint,
		&ImageSizeFlag, &ImageWidth, &ImageHeight,
		&ColorMapFlag, &ColorMapFile,
		&TextifyFlag,
		&HelpFlag,
		&NumFiles, &FileName)) != false ||
		(NumFiles > 1 && !HelpFlag)) {
	if (Error)
	    GAPrintErrMsg(Error);
	else if (NumFiles > 1)
	    GIF_MESSAGE("Error in command line parsing - one GIF file please.");
	GAPrintHowTo(CtrlStr);
	exit(EXIT_FAILURE);
    }

    if (HelpFlag) {
	(void)fprintf(stderr, VersionStr, GIFLIB_MAJOR, GIFLIB_MINOR);
	GAPrintHowTo(CtrlStr);
	exit(EXIT_SUCCESS);
    }

    if (ImageSizeFlag) {
	ColorMapObject *ColorMap;
	if (ColorMapFlag) {
	    FILE *InColorMapFile;
	    int ColorMapSize;

	    /* Read color map from given file: */
	    if ((InColorMapFile = fopen(ColorMapFile, "rt")) == NULL) {
		GIF_MESSAGE("Failed to open COLOR MAP file (not exists!?).");
		exit(EXIT_FAILURE);
	    }
	    if ((ColorMap = GifMakeMapObject(256, NULL)) == NULL) {
		GIF_MESSAGE("Failed to allocate bitmap, aborted.");
		exit(EXIT_FAILURE);
	    }

	    for (ColorMapSize = 0;
		 ColorMapSize < 256 && !feof(InColorMapFile);
		 ColorMapSize++) {
		if (fscanf(InColorMapFile, "%3d %3d %3d %3d\n",
			   &Dummy, &Red, &Green, &Blue) == 4) {
		    ColorMap->Colors[ColorMapSize].Red = Red;
		    ColorMap->Colors[ColorMapSize].Green = Green;
		    ColorMap->Colors[ColorMapSize].Blue = Blue;
		}
	    }
	    (void)fclose(InColorMapFile);
	}
	else {
	    ColorMap = GifMakeMapObject(EGA_PALETTE_SIZE, EGAPalette);
	}

	if (NumFiles == 1) {
	    int InFileHandle;
    #ifdef _WIN32
	    if ((InFileHandle = open(*FileName, O_RDONLY | O_BINARY)) == -1) {
    #else
	    if ((InFileHandle = open(*FileName, O_RDONLY)) == -1) {
    #endif /* _WIN32 */
		GIF_MESSAGE("Failed to open RAW image file (not exists!?).");
		exit(EXIT_FAILURE);
	    }
	    dup2(InFileHandle, 0);		       /* Make stdin from this file. */
	}
	else {
    #ifdef _WIN32
	    _setmode(0, O_BINARY);		  /* Make sure it is in binary mode. */
    #endif /* _WIN32 */
	}

	/* Convert Raw image from stdin to GIF file in stdout: */
	Raw2Gif(ImageWidth, ImageHeight, ColorMap);
    }
    else {
	GifFileType *GifFile;

	if (NumFiles == 1) {
	    if ((GifFile = DGifOpenFileName(*FileName, &ErrorCode)) == NULL) {
		PrintGifError(ErrorCode);
		exit(EXIT_FAILURE);
	    }
	}
	else {
	    /* Use stdin instead: */
	    if ((GifFile = DGifOpenFileHandle(0, &ErrorCode)) == NULL) {
		PrintGifError(ErrorCode);
		exit(EXIT_FAILURE);
	    }
	}
	Gif2Raw(GifFile, TextifyFlag);
    }

    return 0;
}

/******************************************************************************
 Convert raw image (One byte per pixel) into GIF file. Raw data is read from
 stdin, and GIF is dumped to stdout. ImagwWidth times ImageHeight bytes are
 read. Color map is dumped from ColorMap.
******************************************************************************/
void Raw2Gif(int ImageWidth, int ImageHeight, ColorMapObject *ColorMap)
{
    int i, j, ErrorCode;
    static GifPixelType *ScanLine;
    GifFileType *GifFile;

    if ((ScanLine = (GifPixelType *) malloc(sizeof(GifPixelType) * ImageWidth))
								== NULL) {
	GIF_MESSAGE("Failed to allocate scan line, aborted.");
	exit(EXIT_FAILURE);
    }

    if ((GifFile = EGifOpenFileHandle(1, &ErrorCode)) == NULL) {	   /* Gif to stdout. */
	free((char *) ScanLine);
	PrintGifError(ErrorCode);
	exit(EXIT_FAILURE);
    }

    if (EGifPutScreenDesc(GifFile, ImageWidth, ImageHeight, ColorMap->BitsPerPixel,
			  0, ColorMap) == GIF_ERROR) {
	free((char *) ScanLine);
	PrintGifError(GifFile->Error);
	exit(EXIT_FAILURE);
    }

    if (EGifPutImageDesc(GifFile, 0, 0, ImageWidth, ImageHeight, false,
			 NULL) == GIF_ERROR) {
	free((char *) ScanLine);
	PrintGifError(GifFile->Error);
	exit(EXIT_FAILURE);
    }

    /* Here it is - get one raw line from stdin, and dump to stdout Gif: */
    GifQprintf("\n%s: Image 1 at (0, 0) [%dx%d]:     ",
	PROGRAM_NAME, ImageWidth, ImageHeight);
    for (i = 0; i < ImageHeight; i++) {
	/* Note we assume here PixelSize == Byte, which is not necessarily   */
	/* so. If not - must read one byte at a time, and coerce to pixel.   */
	if (fread(ScanLine, 1, ImageWidth, stdin) != (unsigned)ImageWidth) {
	    GIF_MESSAGE("RAW input file ended prematurely.");
	    exit(EXIT_FAILURE);
	}

	for (j = 0; j < ImageWidth; j++)
	    if (ScanLine[j] >= ColorMap->ColorCount)
		GIF_MESSAGE("Warning: RAW data color > maximum color map entry.");

	if (EGifPutLine(GifFile, ScanLine, ImageWidth) == GIF_ERROR) {
	    free((char *) ScanLine);
	    PrintGifError(GifFile->Error);
	    exit(EXIT_FAILURE);
	}
	GifQprintf("\b\b\b\b%-4d", i);
    }

    if (EGifCloseFile(GifFile) == GIF_ERROR) {
	free((char *) ScanLine);
	PrintGifError(GifFile->Error);
	exit(EXIT_FAILURE);
    }

    free((char *) ScanLine);
}

static void Gif2Raw(GifFileType *GifFile, bool Textify)
{
    int i, j, ExtCode;
    GifPixelType *Line;
    GifRecordType RecordType;
    GifByteType *Extension;

#ifdef _WIN32
    _setmode(1, O_BINARY);             /* Make sure it is in binary mode. */
#endif /* _WIN32 */
 
   do {
	if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
	    PrintGifError(GifFile->Error);
	    exit(EXIT_FAILURE);
	}
	switch (RecordType) {
	    case IMAGE_DESC_RECORD_TYPE:
		if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
		    PrintGifError(GifFile->Error);
		    exit(EXIT_FAILURE);
		}

		Line = (GifPixelType *) malloc(GifFile->Image.Width *
					    sizeof(GifPixelType));
		for (i = 0; i < GifFile->Image.Height; i++) {
		    if (DGifGetLine(GifFile, Line, GifFile->Image.Width)
			== GIF_ERROR) {
			PrintGifError(GifFile->Error);
			exit(EXIT_FAILURE);
		    }
		    if (Textify)
			for (j = 0; j < GifFile->Image.Height; j++)
			    Line[j] += ' ';
		    fwrite(Line, 1, GifFile->Image.Width, stdout);
		    if (Textify)
			putchar('\n');
		}
		free((char *) Line);
		break;
	    case EXTENSION_RECORD_TYPE:
		if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
		    PrintGifError(GifFile->Error);
		    exit(EXIT_FAILURE);
		}
		for (;;) {
		    if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
			PrintGifError(GifFile->Error);
			exit(EXIT_FAILURE);
		    }
		    if (Extension == NULL)
			break;
		}
		break;
	    case TERMINATE_RECORD_TYPE:
		break;
	    default:		     /* Should be trapped by DGifGetRecordType */
		break;
	}
    }
    while (RecordType != TERMINATE_RECORD_TYPE);

    if (DGifCloseFile(GifFile) == GIF_ERROR) {
	PrintGifError(GifFile->Error);
	exit(EXIT_FAILURE);
    }
}

/* end */
