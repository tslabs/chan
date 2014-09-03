/*------------------------------------------------------------------------/
/  The Main Development Bench of TJpgDec Module 
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2011, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wingdi.h>
#include "tjpgd.h"



#define MODE	0	/* Test mode: 0:JPEG-BMP conversion, 1:Report about each files but output */
#define SCALE	0	/* Output scaling 0:1/1, 1:1/2, 2:1/4 or 3:1/8 */


/* User defined session identifier */

typedef struct {
	HANDLE hin;		/* Handle to input stream */
	BYTE *frmbuf;	/* Pointer to the frame buffer */
	DWORD wbyte;	/* Number of bytes a line of the frame buffer */
} IODEV;



/* User defined input function */

UINT input_func (
	JDEC* jd,		/* Decompression object */
	BYTE* buff,		/* Pointer to the read buffer (0:skip) */
	UINT ndata		/* Number of bytes to read/skip */
)
{
	DWORD rb;
	IODEV *dev = (IODEV*)jd->device;


	if (buff) {
		ReadFile(dev->hin, buff, ndata, &rb, 0);
		return rb;
	} else {
		rb = SetFilePointer(dev->hin, ndata, 0, FILE_CURRENT);
		return rb == 0xFFFFFFFF ? 0 : ndata;
	}
}



/* User defined output function */

UINT output_func (
	JDEC* jd,		/* Decompression object */
	void* bitmap,	/* Bitmap data to be output */
	JRECT* rect		/* Rectangular region to output */
)
{
	UINT ny, nbx, xc;
	DWORD wd;
	BYTE *src, *dst;
	IODEV *dev = (IODEV*)jd->device;


	/* Put progress indicator */
	if (MODE == 0 && rect->left == 0) {
		printf("\r%lu%%", (rect->top << jd->scale) * 100UL / jd->height);
	}

	nbx = (rect->right - rect->left + 1) * 3;	/* Number of bytes a line of the rectangular */
	ny = rect->bottom - rect->top + 1;			/* Number of lines of the rectangular */
	src = (BYTE*)bitmap;						/* RGB bitmap to be output */

	wd = dev->wbyte;							/* Number of bytes a line of the frame buffer */
	dst = dev->frmbuf + rect->top * wd + rect->left * 3;	/* Left-top of the destination rectangular in the frame buffer */

	do {	/* Copy the rectangular to the frame buffer */
		xc = nbx;
		do {
			*dst++ = *src++;
		} while (--xc);
		dst += wd - nbx;
	} while (--ny);

	return 1;	/* Continue to decompress */
}




void write_bmp (
	const char* fname,
	BYTE* buf,
	UINT width,
	UINT height
)
{
	DWORD wb, xb = ((DWORD)width * 3 + 3) & ~3;
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	HANDLE hbmp;
	BYTE *s, *d, r, g, b;
	UINT i;


	hbmp = CreateFile(fname, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hbmp == INVALID_HANDLE_VALUE) return;

	memset(&bfh, 0, sizeof bfh);
	bfh.bfType = 'MB';
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + xb * height;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	WriteFile(hbmp, &bfh, sizeof bfh, &wb, 0);

	memset(&bih, 0, sizeof bih);
	bih.biSize = sizeof bih;
	bih.biWidth = width;
	bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	WriteFile(hbmp, &bih, sizeof bih, &wb, 0);

	/* Flip top and down, swap byte order RGB to BGR */
	s = buf; d = buf + xb * (height - 1);
	while (s <= d) {
		for (i = 0; i < width * 3; i += 3) {
			r = s[i+0]; g = s[i+1]; b = s[i+2];
			s[i+0] = d[i+2]; s[i+1] = d[i+1]; s[i+2] = d[i+0];
			d[i+0] = b; d[i+1] = g; d[i+2] = r;
		}
		d -= xb; s += xb;
	}

	WriteFile(hbmp, buf, xb * height, &wb, 0);

	CloseHandle(hbmp);
}




void jpegtest (
	char* fn
)
{
	const size_t sz_work = 4096;	/* Size of working buffer for TJDEC module */
	void *jdwork;
	JDEC jd;		/* TJDEC decompression object */
	IODEV iodev;	/* Identifier of the decompression session (depends on application) */
	JRESULT rc;
	UINT xs, ys;
	DWORD xb;
	char str[256];


	printf("%s", fn);	/* Put file name */

	/* Open JPEG file */
	iodev.hin = CreateFile(fn, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (iodev.hin != INVALID_HANDLE_VALUE) {

		jdwork = VirtualAlloc(0, sz_work, MEM_COMMIT, PAGE_READWRITE);	/* Allocate a work area for TJDEC */

		rc = jd_prepare(&jd, input_func, jdwork, sz_work, &iodev);		/* Prepare to decompress the file */

		if (!rc) {
			if (MODE == 1) {	/* Put file properties */
				printf(",%u,%u", jd.width, jd.height);	/* Image size */
				printf(",[%d:%d:%d]", 4, 4 / jd.msx, (jd.msy == 2) ? 0 : (jd.msx == 1) ? 4 : 2);	/* Sampling ratio */
				printf(",%u", sz_work - jd.sz_pool);	/* Get used memory size by rest of memory pool */
			} else {
				printf("\n");
			}
			xs = jd.width >> SCALE;			/* Output size */
			ys = jd.height >> SCALE;
			xb = ((DWORD)xs * 3 + 3) & ~3;	/* Byte width of the frame buffer */
			iodev.wbyte = xb;
			iodev.frmbuf = VirtualAlloc(0, xb * ys, MEM_COMMIT, PAGE_READWRITE);	/* Allocate an output frame buffer */
			rc = jd_decomp(&jd, output_func, SCALE);	/* Start to decompress */
			if (!rc) {		/* Save the decompressed picture as a bmp file */
				if (MODE == 1) {
					printf(",%d", rc);
				} else {
					printf("\rOK  ");
					strcpy(str, fn);
					strcpy(str + strlen(str) - 4, ".bmp");
					write_bmp(str, (BYTE*)iodev.frmbuf, xs, ys);
				}
			} else {	/* Error occured on decompress */
				if (MODE == 1)
					printf(",%d", rc);
				else
					printf("\rError(%d)", rc);
			}
			VirtualFree(iodev.frmbuf, 0, MEM_RELEASE);	/* Discard frame buffer */
		} else {	/* Error occured on prepare */
			if (MODE == 1)
				printf(",,,,,%d", rc);
			else
				printf("\nErr: %d", rc);
		}

		VirtualFree(jdwork, 0, MEM_RELEASE);	/* Discard work area */

		CloseHandle(iodev.hin);	/* Close JPEG file */
	}

	printf("\n");
}





int main (int argc, char* argv[])
{
	HANDLE fd;
	WIN32_FIND_DATA ff;
	static int nest;


	if (!nest) {
		if (MODE == 1)
			printf("File Name,Width,Height,Sampling,Used Memory,Result\n");
		if (argc == 2) SetCurrentDirectory(argv[1]);
	}
	nest++;

	fd = FindFirstFile("*", &ff);
	if (fd != INVALID_HANDLE_VALUE) {
		do {
			if (ff.cFileName[0] == '.') continue;
			if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				SetCurrentDirectory(ff.cFileName);
				main(0, 0);
				SetCurrentDirectory("..");
			} else {
				if (strstr(ff.cFileName, ".jpg") || strstr(ff.cFileName, ".JPG"))
					jpegtest(ff.cFileName);
			}
		} while (FindNextFile(fd, &ff));

		FindClose(fd);
	}

	nest--;

	return 0;
}




