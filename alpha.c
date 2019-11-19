//  Copyright (C) 2005 SoftKAM. All Rights Reserved.
//
//  AlphaBlend is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.

#include "alpha.h"

/******************************************************************************/
void AlphaBlt(unsigned char *dst, unsigned char *src, int w, int h)
{
	int i;
	int srcB, srcG, srcR, srcA;
	int dstB, dstG, dstR, dstA;

	for (i=0; i < w * h * 4; i += 4)
	{
		// grab src rgba
		srcB = src[i + 0];
		srcG = src[i + 1];
		srcR = src[i + 2];
		srcA = src[i + 3];
		// grab dst rgba
		dstB = dst[i + 0];
		dstG = dst[i + 1];
		dstR = dst[i + 2];
		dstA = dst[i + 3];
		// calculate blit rgba
		dstB = (srcA * (srcB - dstB) + dstB * 256) / 256;
		dstG = (srcA * (srcG - dstG) + dstG * 256) / 256;
		dstR = (srcA * (srcR - dstR) + dstR * 256) / 256;
		dstA = srcA;
		// apply blit to dst
		dst[i + 0] = dstB;
		dst[i + 1] = dstG;
		dst[i + 2] = dstR;
		dst[i + 3] = dstA;
	}
}

/******************************************************************************/
void AlphaBltSSE(unsigned char *dst, unsigned char *src, int w, int h)
{
	int	wmul4 = w << 2;

	if (w==0) return;
	w >>= 1;
	_asm {
// For each pixel: dst = (src_alpha * (src-dst)  + dst * 256) / 256
		mov			edi,dst
		mov			esi,src
		mov			edx,h
		pxor		mm6,mm6
		pxor		mm7,mm7
		xor			eax,eax
scan_loop:
		mov			ecx,w
		xor			ebx,ebx
pix_loop:
		movq		mm4,[esi+ebx*8]		// mm0 = src (RG BA RG BA)
		movq		mm5,[edi+ebx*8]		// mm1 = dst (RG BA RG BA)
// FIRST PIXEL
		movq		mm0,mm4				// mm0 = 00 00 RG BA
		movq		mm1,mm5				// mm1 = 00 00 RG BA
		punpcklbw	mm0,mm6				// mm0 = (0R 0G 0B 0A)
		punpcklbw	mm1,mm7				// mm0 = (0R 0G 0B 0A)
		pshufw		mm2,mm0,0ffh		// mm2 = 0A 0A 0A 0A
		movq		mm3,mm1				// mm3 = mm1
		psubw		mm0,mm1				// mm0 = mm0 - mm1
		psllw		mm3,8				// mm3 = mm1 * 256
		pmullw		mm0,mm2				// mm0 = (src-dst)*alpha
		paddw		mm0,mm3				// mm0 = (src-dst)*alpha+dst*256
		psrlw		mm0,8				// mm0 = ((src - dst) * alpha + dst * 256) / 256
// SECOND PIXEL
		punpckhbw	mm5,mm7				// mm5 = (0R 0G 0B 0A)
		punpckhbw	mm4,mm6				// mm4 = (0R 0G 0B 0A)
		movq		mm3,mm5				// mm3 = mm5
		pshufw		mm2,mm4,0ffh		// mm2 = 0A 0A 0A 0A
		psllw		mm3,8				// mm3 = mm5 * 256
		psubw		mm4,mm5				// mm4 = mm4 - mm5
		pmullw		mm4,mm2				// mm4 = (src-dst)*alpha
		paddw		mm4,mm3				// mm4 = (src-dst)*alpha+dst*256
		psrlw		mm4,8				// mm4 = ((src - dst) * alpha + dst * 256) / 256
		packuswb	mm0,mm4				// mm0 = RG BA RG BA
		movq		[edi+ebx*8],mm0		// dst = mm0
		inc			ebx
		loop		pix_loop
//
		mov			ebx, wmul4
		add			esi, ebx
		add			edi, ebx
		dec			edx
		jnz			scan_loop
	}
}

/******************************************************************************/
void AlphaBltMMX(unsigned char *dst, unsigned char *src, int w, int h)
{
	int	wmul4 = w << 2;

	if (w==0) return;
	w >>= 1;
	_asm {
// For each pixel: dst = (src_alpha * (src-dst)  + dst * 256) / 256
		mov			edi,dst
		mov			esi,src
		mov			edx,h
		pxor		mm6,mm6
		pxor		mm7,mm7
		xor			eax,eax
scan_loop:
		mov			ecx,w
		xor			ebx,ebx
pix_loop:
		movq		mm4,[esi+ebx*8]		// mm4 = src (RG BA RG BA)
		movq		mm5,[edi+ebx*8]		// mm5 = dst (RG BA RG BA)
// FIRST PIXEL
		movq		mm0,mm4				// mm0 = src (-- -- RG BA)
		movq		mm1,mm5				// mm1 = dst (-- -- RG BA)
		punpcklbw	mm0,mm6				// mm0 = (0R 0G 0B 0A)
		mov			al,[esi+ebx*8+3]	// eax = pixel alpha (0 - 255)
		punpcklbw	mm1,mm7				// mm1 = (0R 0G 0B 0A)
		movd		mm2,eax				// 00 00 00 0A
		movq		mm3,mm1				// mm3 = mm1: dst (0R 0G 0B 0A)
		punpcklwd	mm2,mm2				// 00 00 0A 0A
		psubw		mm0,mm1				// mm0 = mm0 - mm1
		punpckldq	mm2,mm2				// 0A 0A 0A 0A
		psllw		mm3,8				// mm3 = mm1 * 256
		pmullw		mm0,mm2				// mm0 = (src - dst) * alpha
		paddw		mm0,mm3				// mm0 = (src - dst) * alpha + dst * 256
		psrlw		mm0,8				// mm0 = ((src - dst) * alpha + dst * 256) / 256
		packuswb	mm0,mm6				// mm0 = RGBA
// SECOND PIXEL
		punpckhbw	mm4,mm6				// mm4 = (0R 0G 0B 0A)
		mov			al,[esi+ebx*8+7]	// eax = pixel alpha (0 - 255)
		punpckhbw	mm5,mm7				// mm5 = (0R 0G 0B 0A)
		movd		mm2,eax				// 00 00 00 0A
		movq		mm3,mm5				// mm3 = mm5: dst (0R 0G 0B 0A)
		punpcklwd	mm2,mm2				// 00 00 0A 0A
		psubw		mm4,mm5				// mm4 = mm4 - mm5
		punpckldq	mm2,mm2				// 0A 0A 0A 0A
		psllw		mm3,8				// mm3 = mm5 * 256
		pmullw		mm4,mm2				// mm4 = (src - dst) * alpha
		paddw		mm4,mm3				// mm4 = (src - dst) * alpha + dst * 256
		psrlw		mm4,8				// mm4 = ((src - dst) * alpha + dst * 256) / 256
		packuswb	mm4,mm6				// mm4 = RGBA
		punpckldq	mm0,mm4				// mm0 = RG BA RG BA
		movq		[edi+ebx*8],mm0		// dst = mm0
		inc			ebx
// REPEAT
		loop		pix_loop
		mov			ebx, wmul4
		add			esi, ebx
		add			edi, ebx
		dec			edx
		jnz			scan_loop
	}
}

