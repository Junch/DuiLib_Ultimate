﻿// xImaDsp.cpp : DSP functions
/* 07/08/2001 v1.00 - Davide Pizzolato - www.xdp.it
 * CxImage version 7.0.1 07/Jan/2011
 */

#include "ximage.h"

#include "ximaiter.h"

#if CXIMAGE_SUPPORT_DSP

////////////////////////////////////////////////////////////////////////////////
/**
 * Converts the image to B&W.
 * The OptimalThreshold() function can be used for calculating the optimal threshold.
 * \param level: the lightness threshold.
 * \return true if everything is ok
 */
bool CxImage::Threshold(uint8_t level)
{
	if (!pDib) return false;
	if (head.biBitCount == 1) return true;

	GrayScale();

	CxImage tmp(head.biWidth,head.biHeight,1);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	for (int32_t y=0;y<head.biHeight;y++){
		info.nProgress = (int32_t)(100*y/head.biHeight);
		if (info.nEscape) break;
		for (int32_t x=0;x<head.biWidth;x++){
			if (BlindGetPixelIndex(x,y)>level)
				tmp.BlindSetPixelIndex(x,y,1);
			else
				tmp.BlindSetPixelIndex(x,y,0);
		}
	}
	tmp.SetPaletteColor(0,0,0,0);
	tmp.SetPaletteColor(1,255,255,255);
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Converts the image to B&W, using a threshold mask
 * \param pThresholdMask: the lightness threshold mask.
 * the pThresholdMask image must be grayscale with same with and height of the current image
 * \return true if everything is ok
 */
bool CxImage::Threshold(CxImage* pThresholdMask)
{
	if (!pDib) return false;
	if (head.biBitCount == 1) return true;

	if (!pThresholdMask) return false;
	
	if (!pThresholdMask->IsValid() ||
		!pThresholdMask->IsGrayScale() ||
		pThresholdMask->GetWidth() != GetWidth() ||
		pThresholdMask->GetHeight() != GetHeight()){
		strcpy(info.szLastError,"invalid ThresholdMask");
		return false;
	}

	GrayScale();

	CxImage tmp(head.biWidth,head.biHeight,1);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	for (int32_t y=0;y<head.biHeight;y++){
		info.nProgress = (int32_t)(100*y/head.biHeight);
		if (info.nEscape) break;
		for (int32_t x=0;x<head.biWidth;x++){
			if (BlindGetPixelIndex(x,y)>pThresholdMask->BlindGetPixelIndex(x,y))
				tmp.BlindSetPixelIndex(x,y,1);
			else
				tmp.BlindSetPixelIndex(x,y,0);
		}
	}
	tmp.SetPaletteColor(0,0,0,0);
	tmp.SetPaletteColor(1,255,255,255);
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Filters only the pixels with a lightness less (or more) than the threshold level,
 * and preserves the colors for the unfiltered pixels.
 * \param level = the lightness threshold.
 * \param bDirection = false: filter dark pixels, true: filter light pixels
 * \param nBkgndColor =  filtered pixels are set to nBkgndColor color
 * \param bSetAlpha = if true, sets also the alpha component for the filtered pixels, with nBkgndColor.rgbReserved
 * \return true if everything is ok
 * \author [DP], [wangsongtao]
 */
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Threshold2(uint8_t level, bool bDirection, RGBQUAD nBkgndColor, bool bSetAlpha)
{
	if (!pDib) return false;
	if (head.biBitCount == 1) return true;

	CxImage tmp(*this, true, false, false);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	tmp.GrayScale();

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				uint8_t i = tmp.BlindGetPixelIndex(x,y);
				if (!bDirection && i<level) BlindSetPixelColor(x,y,nBkgndColor,bSetAlpha);
				if (bDirection && i>=level) BlindSetPixelColor(x,y,nBkgndColor,bSetAlpha);
			}
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Extract RGB channels from the image. Each channel is an 8 bit grayscale image. 
 * \param r,g,b: pointers to CxImage objects, to store the splited channels
 * \return true if everything is ok
 */
bool CxImage::SplitRGB(CxImage* r,CxImage* g,CxImage* b)
{
	if (!pDib) return false;
	if (r==NULL && g==NULL && b==NULL) return false;

	CxImage tmpr(head.biWidth,head.biHeight,8);
	CxImage tmpg(head.biWidth,head.biHeight,8);
	CxImage tmpb(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(int32_t y=0; y<head.biHeight; y++){
		for(int32_t x=0; x<head.biWidth; x++){
			color = BlindGetPixelColor(x,y);
			if (r) tmpr.BlindSetPixelIndex(x,y,color.rgbRed);
			if (g) tmpg.BlindSetPixelIndex(x,y,color.rgbGreen);
			if (b) tmpb.BlindSetPixelIndex(x,y,color.rgbBlue);
		}
	}

	if (r) tmpr.SetGrayPalette();
	if (g) tmpg.SetGrayPalette();
	if (b) tmpb.SetGrayPalette();

	/*for(int32_t j=0; j<256; j++){
		uint8_t i=(uint8_t)j;
		if (r) tmpr.SetPaletteColor(i,i,0,0);
		if (g) tmpg.SetPaletteColor(i,0,i,0);
		if (b) tmpb.SetPaletteColor(i,0,0,i);
	}*/

	if (r) r->Transfer(tmpr);
	if (g) g->Transfer(tmpg);
	if (b) b->Transfer(tmpb);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Extract CMYK channels from the image. Each channel is an 8 bit grayscale image. 
 * \param c,m,y,k: pointers to CxImage objects, to store the splited channels
 * \return true if everything is ok
 */
bool CxImage::SplitCMYK(CxImage* c,CxImage* m,CxImage* y,CxImage* k)
{
	if (!pDib) return false;
	if (c==NULL && m==NULL && y==NULL && k==NULL) return false;

	CxImage tmpc(head.biWidth,head.biHeight,8);
	CxImage tmpm(head.biWidth,head.biHeight,8);
	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpk(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(int32_t yy=0; yy<head.biHeight; yy++){
		for(int32_t xx=0; xx<head.biWidth; xx++){
			color = BlindGetPixelColor(xx,yy);
			if (c) tmpc.BlindSetPixelIndex(xx,yy,(uint8_t)(255-color.rgbRed));
			if (m) tmpm.BlindSetPixelIndex(xx,yy,(uint8_t)(255-color.rgbGreen));
			if (y) tmpy.BlindSetPixelIndex(xx,yy,(uint8_t)(255-color.rgbBlue));
			if (k) tmpk.BlindSetPixelIndex(xx,yy,(uint8_t)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue));
		}
	}

	if (c) tmpc.SetGrayPalette();
	if (m) tmpm.SetGrayPalette();
	if (y) tmpy.SetGrayPalette();
	if (k) tmpk.SetGrayPalette();

	if (c) c->Transfer(tmpc);
	if (m) m->Transfer(tmpm);
	if (y) y->Transfer(tmpy);
	if (k) k->Transfer(tmpk);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Extract YUV channels from the image. Each channel is an 8 bit grayscale image. 
 * \param y,u,v: pointers to CxImage objects, to store the splited channels
 * \return true if everything is ok
 */
bool CxImage::SplitYUV(CxImage* y,CxImage* u,CxImage* v)
{
	if (!pDib) return false;
	if (y==NULL && u==NULL && v==NULL) return false;

	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpu(head.biWidth,head.biHeight,8);
	CxImage tmpv(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(int32_t yy=0; yy<head.biHeight; yy++){
		for(int32_t x=0; x<head.biWidth; x++){
			color = RGBtoYUV(BlindGetPixelColor(x,yy));
			if (y) tmpy.BlindSetPixelIndex(x,yy,color.rgbRed);
			if (u) tmpu.BlindSetPixelIndex(x,yy,color.rgbGreen);
			if (v) tmpv.BlindSetPixelIndex(x,yy,color.rgbBlue);
		}
	}

	if (y) tmpy.SetGrayPalette();
	if (u) tmpu.SetGrayPalette();
	if (v) tmpv.SetGrayPalette();

	if (y) y->Transfer(tmpy);
	if (u) u->Transfer(tmpu);
	if (v) v->Transfer(tmpv);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Extract YIQ channels from the image. Each channel is an 8 bit grayscale image. 
 * \param y,i,q: pointers to CxImage objects, to store the splited channels
 * \return true if everything is ok
 */
bool CxImage::SplitYIQ(CxImage* y,CxImage* i,CxImage* q)
{
	if (!pDib) return false;
	if (y==NULL && i==NULL && q==NULL) return false;

	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpi(head.biWidth,head.biHeight,8);
	CxImage tmpq(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(int32_t yy=0; yy<head.biHeight; yy++){
		for(int32_t x=0; x<head.biWidth; x++){
			color = RGBtoYIQ(BlindGetPixelColor(x,yy));
			if (y) tmpy.BlindSetPixelIndex(x,yy,color.rgbRed);
			if (i) tmpi.BlindSetPixelIndex(x,yy,color.rgbGreen);
			if (q) tmpq.BlindSetPixelIndex(x,yy,color.rgbBlue);
		}
	}

	if (y) tmpy.SetGrayPalette();
	if (i) tmpi.SetGrayPalette();
	if (q) tmpq.SetGrayPalette();

	if (y) y->Transfer(tmpy);
	if (i) i->Transfer(tmpi);
	if (q) q->Transfer(tmpq);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Extract XYZ channels from the image. Each channel is an 8 bit grayscale image. 
 * \param x,y,z: pointers to CxImage objects, to store the splited channels
 * \return true if everything is ok
 */
bool CxImage::SplitXYZ(CxImage* x,CxImage* y,CxImage* z)
{
	if (!pDib) return false;
	if (x==NULL && y==NULL && z==NULL) return false;

	CxImage tmpx(head.biWidth,head.biHeight,8);
	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpz(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(int32_t yy=0; yy<head.biHeight; yy++){
		for(int32_t xx=0; xx<head.biWidth; xx++){
			color = RGBtoXYZ(BlindGetPixelColor(xx,yy));
			if (x) tmpx.BlindSetPixelIndex(xx,yy,color.rgbRed);
			if (y) tmpy.BlindSetPixelIndex(xx,yy,color.rgbGreen);
			if (z) tmpz.BlindSetPixelIndex(xx,yy,color.rgbBlue);
		}
	}

	if (x) tmpx.SetGrayPalette();
	if (y) tmpy.SetGrayPalette();
	if (z) tmpz.SetGrayPalette();

	if (x) x->Transfer(tmpx);
	if (y) y->Transfer(tmpy);
	if (z) z->Transfer(tmpz);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Extract HSL channels from the image. Each channel is an 8 bit grayscale image. 
 * \param h,s,l: pointers to CxImage objects, to store the splited channels
 * \return true if everything is ok
 */
bool CxImage::SplitHSL(CxImage* h,CxImage* s,CxImage* l)
{
	if (!pDib) return false;
	if (h==NULL && s==NULL && l==NULL) return false;

	CxImage tmph(head.biWidth,head.biHeight,8);
	CxImage tmps(head.biWidth,head.biHeight,8);
	CxImage tmpl(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(int32_t y=0; y<head.biHeight; y++){
		for(int32_t x=0; x<head.biWidth; x++){
			color = RGBtoHSL(BlindGetPixelColor(x,y));
			if (h) tmph.BlindSetPixelIndex(x,y,color.rgbRed);
			if (s) tmps.BlindSetPixelIndex(x,y,color.rgbGreen);
			if (l) tmpl.BlindSetPixelIndex(x,y,color.rgbBlue);
		}
	}

	if (h) tmph.SetGrayPalette();
	if (s) tmps.SetGrayPalette();
	if (l) tmpl.SetGrayPalette();

	/* pseudo-color generator for hue channel (visual debug)
	if (h) for(int32_t j=0; j<256; j++){
		uint8_t i=(uint8_t)j;
		RGBQUAD hsl={120,240,i,0};
		tmph.SetPaletteColor(i,HSLtoRGB(hsl));
	}*/

	if (h) h->Transfer(tmph);
	if (s) s->Transfer(tmps);
	if (l) l->Transfer(tmpl);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#define  HSLMAX   255	/* H,L, and S vary over 0-HSLMAX */
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */
                        /* HSLMAX BEST IF DIVISIBLE BY 6 */
                        /* RGBMAX, HSLMAX must each fit in a uint8_t. */
/* Hue is undefined if Saturation is 0 (grey-scale) */
/* This value determines where the Hue scrollbar is */
/* initially set for achromatic colors */
#define HSLUNDEFINED (HSLMAX*2/3)
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoHSL(RGBQUAD lRGBColor)
{
	uint8_t R,G,B;					/* input RGB values */
	uint8_t H,L,S;					/* output HSL values */
	uint8_t cMax,cMin;				/* max and min RGB values */
	uint16_t Rdelta,Gdelta,Bdelta;	/* intermediate value: % of spread from max*/

	R = lRGBColor.rgbRed;	/* get R, G, and B out of uint32_t */
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

	cMax = max( max(R,G), B);	/* calculate lightness */
	cMin = min( min(R,G), B);
	L = (uint8_t)((((cMax+cMin)*HSLMAX)+RGBMAX)/(2*RGBMAX));

	if (cMax==cMin){			/* r=g=b --> achromatic case */
		S = 0;					/* saturation */
		H = HSLUNDEFINED;		/* hue */
	} else {					/* chromatic case */
		if (L <= (HSLMAX/2))	/* saturation */
			S = (uint8_t)((((cMax-cMin)*HSLMAX)+((cMax+cMin)/2))/(cMax+cMin));
		else
			S = (uint8_t)((((cMax-cMin)*HSLMAX)+((2*RGBMAX-cMax-cMin)/2))/(2*RGBMAX-cMax-cMin));
		/* hue */
		Rdelta = (uint16_t)((((cMax-R)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));
		Gdelta = (uint16_t)((((cMax-G)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));
		Bdelta = (uint16_t)((((cMax-B)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));

		if (R == cMax)
			H = (uint8_t)(Bdelta - Gdelta);
		else if (G == cMax)
			H = (uint8_t)((HSLMAX/3) + Rdelta - Bdelta);
		else /* B == cMax */
			H = (uint8_t)(((2*HSLMAX)/3) + Gdelta - Rdelta);

//		if (H < 0) H += HSLMAX;     //always false
		if (H > HSLMAX) H -= HSLMAX;
	}
	RGBQUAD hsl={L,S,H,0};
	return hsl;
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::HueToRGB(float n1,float n2, float hue)
{
	//<F. Livraghi> fixed implementation for HSL2RGB routine
	float rValue;

	if (hue > 360)
		hue = hue - 360;
	else if (hue < 0)
		hue = hue + 360;

	if (hue < 60)
		rValue = n1 + (n2-n1)*hue/60.0f;
	else if (hue < 180)
		rValue = n2;
	else if (hue < 240)
		rValue = n1+(n2-n1)*(240-hue)/60;
	else
		rValue = n1;

	return rValue;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::HSLtoRGB(COLORREF cHSLColor)
{
	return HSLtoRGB(RGBtoRGBQUAD(cHSLColor));
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::HSLtoRGB(RGBQUAD lHSLColor)
{ 
	//<F. Livraghi> fixed implementation for HSL2RGB routine
	float h,s,l;
	float m1,m2;
	uint8_t r,g,b;

	h = (float)lHSLColor.rgbRed * 360.0f/255.0f;
	s = (float)lHSLColor.rgbGreen/255.0f;
	l = (float)lHSLColor.rgbBlue/255.0f;

	if (l <= 0.5)	m2 = l * (1+s);
	else			m2 = l + s - l*s;

	m1 = 2 * l - m2;

	if (s == 0) {
		r=g=b=(uint8_t)(l*255.0f);
	} else {
		r = (uint8_t)(HueToRGB(m1,m2,h+120) * 255.0f);
		g = (uint8_t)(HueToRGB(m1,m2,h) * 255.0f);
		b = (uint8_t)(HueToRGB(m1,m2,h-120) * 255.0f);
	}

	RGBQUAD rgb = {b,g,r,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::YUVtoRGB(RGBQUAD lYUVColor)
{
	int32_t U,V,R,G,B;
	float Y = lYUVColor.rgbRed;
	U = lYUVColor.rgbGreen - 128;
	V = lYUVColor.rgbBlue - 128;

//	R = (int32_t)(1.164 * Y + 2.018 * U);
//	G = (int32_t)(1.164 * Y - 0.813 * V - 0.391 * U);
//	B = (int32_t)(1.164 * Y + 1.596 * V);
	R = (int32_t)( Y + 1.403f * V);
	G = (int32_t)( Y - 0.344f * U - 0.714f * V);
	B = (int32_t)( Y + 1.770f * U);

	R= min(255,max(0,R));
	G= min(255,max(0,G));
	B= min(255,max(0,B));
	RGBQUAD rgb={(uint8_t)B,(uint8_t)G,(uint8_t)R,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoYUV(RGBQUAD lRGBColor)
{
	int32_t Y,U,V,R,G,B;
	R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

//	Y = (int32_t)( 0.257 * R + 0.504 * G + 0.098 * B);
//	U = (int32_t)( 0.439 * R - 0.368 * G - 0.071 * B + 128);
//	V = (int32_t)(-0.148 * R - 0.291 * G + 0.439 * B + 128);
	Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
	U = (int32_t)((B-Y) * 0.565f + 128);
	V = (int32_t)((R-Y) * 0.713f + 128);

	Y= min(255,max(0,Y));
	U= min(255,max(0,U));
	V= min(255,max(0,V));
	RGBQUAD yuv={(uint8_t)V,(uint8_t)U,(uint8_t)Y,0};
	return yuv;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::YIQtoRGB(RGBQUAD lYIQColor)
{
	int32_t I,Q,R,G,B;
	float Y = lYIQColor.rgbRed;
	I = lYIQColor.rgbGreen - 128;
	Q = lYIQColor.rgbBlue - 128;

	R = (int32_t)( Y + 0.956f * I + 0.621f * Q);
	G = (int32_t)( Y - 0.273f * I - 0.647f * Q);
	B = (int32_t)( Y - 1.104f * I + 1.701f * Q);

	R= min(255,max(0,R));
	G= min(255,max(0,G));
	B= min(255,max(0,B));
	RGBQUAD rgb={(uint8_t)B,(uint8_t)G,(uint8_t)R,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoYIQ(RGBQUAD lRGBColor)
{
	int32_t Y,I,Q,R,G,B;
	R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

	Y = (int32_t)( 0.2992f * R + 0.5868f * G + 0.1140f * B);
	I = (int32_t)( 0.5960f * R - 0.2742f * G - 0.3219f * B + 128);
	Q = (int32_t)( 0.2109f * R - 0.5229f * G + 0.3120f * B + 128);

	Y= min(255,max(0,Y));
	I= min(255,max(0,I));
	Q= min(255,max(0,Q));
	RGBQUAD yiq={(uint8_t)Q,(uint8_t)I,(uint8_t)Y,0};
	return yiq;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::XYZtoRGB(RGBQUAD lXYZColor)
{
	int32_t X,Y,Z,R,G,B;
	X = lXYZColor.rgbRed;
	Y = lXYZColor.rgbGreen;
	Z = lXYZColor.rgbBlue;
	double k=1.088751;

	R = (int32_t)(  3.240479f * X - 1.537150f * Y - 0.498535f * Z * k);
	G = (int32_t)( -0.969256f * X + 1.875992f * Y + 0.041556f * Z * k);
	B = (int32_t)(  0.055648f * X - 0.204043f * Y + 1.057311f * Z * k);

	R= min(255,max(0,R));
	G= min(255,max(0,G));
	B= min(255,max(0,B));
	RGBQUAD rgb={(uint8_t)B,(uint8_t)G,(uint8_t)R,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoXYZ(RGBQUAD lRGBColor)
{
	int32_t X,Y,Z,R,G,B;
	R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

	X = (int32_t)( 0.412453f * R + 0.357580f * G + 0.180423f * B);
	Y = (int32_t)( 0.212671f * R + 0.715160f * G + 0.072169f * B);
	Z = (int32_t)((0.019334f * R + 0.119193f * G + 0.950227f * B)*0.918483657f);

	//X= min(255,max(0,X));
	//Y= min(255,max(0,Y));
	//Z= min(255,max(0,Z));
	RGBQUAD xyz={(uint8_t)Z,(uint8_t)Y,(uint8_t)X,0};
	return xyz;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Generates a "rainbow" palette with saturated colors
 * \param correction: 1 generates a single hue spectrum. 0.75 is nice for scientific applications.
 */
void CxImage::HuePalette(float correction)
{
	if (head.biClrUsed==0) return;

	for(uint32_t j=0; j<head.biClrUsed; j++){
		uint8_t i=(uint8_t)(j*correction*(255/(head.biClrUsed-1)));
		RGBQUAD hsl={120,240,i,0};
		SetPaletteColor((uint8_t)j,HSLtoRGB(hsl));
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Replaces the original hue and saturation values.
 * \param hue: hue
 * \param sat: saturation
 * \param blend: can be from 0 (no effect) to 1 (full effect)
 * \return true if everything is ok
 */
bool CxImage::Colorize(uint8_t hue, uint8_t sat, float blend)
{
	if (!pDib) return false;

	if (blend < 0.0f) blend = 0.0f;
	if (blend > 1.0f) blend = 1.0f;
	int32_t a0 = (int32_t)(256*blend);
	int32_t a1 = 256 - a0;

	bool bFullBlend = false;
	if (blend > 0.999f)	bFullBlend = true;

	RGBQUAD color,hsl;
	if (head.biClrUsed==0){

		int32_t xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			xmin = ymin = 0;
			xmax = head.biWidth; ymax=head.biHeight;
		}

		for(int32_t y=ymin; y<ymax; y++){
			info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
			if (info.nEscape) break;
			for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					if (bFullBlend){
						color = RGBtoHSL(BlindGetPixelColor(x,y));
						color.rgbRed=hue;
						color.rgbGreen=sat;
						BlindSetPixelColor(x,y,HSLtoRGB(color));
					} else {
						color = BlindGetPixelColor(x,y);
						hsl.rgbRed=hue;
						hsl.rgbGreen=sat;
						hsl.rgbBlue = (uint8_t)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue);
						hsl = HSLtoRGB(hsl);
						//BlendPixelColor(x,y,hsl,blend);
						//color.rgbRed = (uint8_t)(hsl.rgbRed * blend + color.rgbRed * (1.0f - blend));
						//color.rgbBlue = (uint8_t)(hsl.rgbBlue * blend + color.rgbBlue * (1.0f - blend));
						//color.rgbGreen = (uint8_t)(hsl.rgbGreen * blend + color.rgbGreen * (1.0f - blend));
						color.rgbRed = (uint8_t)((hsl.rgbRed * a0 + color.rgbRed * a1)>>8);
						color.rgbBlue = (uint8_t)((hsl.rgbBlue * a0 + color.rgbBlue * a1)>>8);
						color.rgbGreen = (uint8_t)((hsl.rgbGreen * a0 + color.rgbGreen * a1)>>8);
						BlindSetPixelColor(x,y,color);
					}
				}
			}
		}
	} else {
		for(uint32_t j=0; j<head.biClrUsed; j++){
			if (bFullBlend){
				color = RGBtoHSL(GetPaletteColor((uint8_t)j));
				color.rgbRed=hue;
				color.rgbGreen=sat;
				SetPaletteColor((uint8_t)j,HSLtoRGB(color));
			} else {
				color = GetPaletteColor((uint8_t)j);
				hsl.rgbRed=hue;
				hsl.rgbGreen=sat;
				hsl.rgbBlue = (uint8_t)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue);
				hsl = HSLtoRGB(hsl);
				color.rgbRed = (uint8_t)(hsl.rgbRed * blend + color.rgbRed * (1.0f - blend));
				color.rgbBlue = (uint8_t)(hsl.rgbBlue * blend + color.rgbBlue * (1.0f - blend));
				color.rgbGreen = (uint8_t)(hsl.rgbGreen * blend + color.rgbGreen * (1.0f - blend));
				SetPaletteColor((uint8_t)j,color);
			}
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Changes the brightness and the contrast of the image. 
 * \param brightness: can be from -255 to 255, if brightness is negative, the image becomes dark.
 * \param contrast: can be from -100 to 100, the neutral value is 0.
 * \return true if everything is ok
 */
bool CxImage::Light(int32_t brightness, int32_t contrast)
{
	if (!pDib) return false;
	float c=(100 + contrast)/100.0f;
	brightness+=128;

	uint8_t cTable[256]; //<nipper>
	for (int32_t i=0;i<256;i++)	{
		cTable[i] = (uint8_t)max(0,min(255,(int32_t)((i-128)*c + brightness + 0.5f)));
	}

	return Lut(cTable);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \return mean lightness of the image. Useful with Threshold() and Light()
 */
float CxImage::Mean()
{
	if (!pDib) return 0;

	CxImage tmp(*this,true);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	tmp.GrayScale();
	float sum=0;

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}
	if (xmin==xmax || ymin==ymax) return (float)0.0;

	uint8_t *iSrc=tmp.info.pImage;
	iSrc += tmp.info.dwEffWidth*ymin; // necessary for selections <Admir Hodzic>

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin)); //<zhanghk><Anatoly Ivasyuk>
		for(int32_t x=xmin; x<xmax; x++){
			sum+=iSrc[x];
		}
		iSrc+=tmp.info.dwEffWidth;
	}
	return sum/(xmax-xmin)/(ymax-ymin);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * 2D linear filter
 * \param kernel: convolving matrix, in row format.
 * \param Ksize: size of the kernel.
 * \param Kfactor: normalization constant.
 * \param Koffset: bias.
 * \verbatim Example: the "soften" filter uses this kernel:
	1 1 1
	1 8 1
	1 1 1
 the function needs: kernel={1,1,1,1,8,1,1,1,1}; Ksize=3; Kfactor=16; Koffset=0; \endverbatim
 * \return true if everything is ok
 */
bool CxImage::Filter(int32_t* kernel, int32_t Ksize, int32_t Kfactor, int32_t Koffset)
{
	if (!pDib) return false;

	int32_t k2 = Ksize/2;
	int32_t kmax= Ksize-k2;
	int32_t r,g,b,i;
	int32_t ksumcur,ksumtot;
	RGBQUAD c;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	ksumtot = 0;
	for(int32_t j=-k2;j<kmax;j++){
		for(int32_t k=-k2;k<kmax;k++){
			ksumtot += kernel[(j+k2)+Ksize*(k+k2)];
		}
	}

	if ((head.biBitCount==8) && IsGrayScale())
	{
		uint8_t* cPtr;
		uint8_t* cPtr2;      
		int32_t iCount;
		int32_t iY, iY2, iY1;
		cPtr = info.pImage;
		cPtr2 = (uint8_t *)tmp.info.pImage;
		for(int32_t y=ymin; y<ymax; y++){
			info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
			if (info.nEscape) break;
			iY1 = y*info.dwEffWidth+xmin;
			for(int32_t x=xmin; x<xmax; x++, iY1++){
#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					b=ksumcur=0;
					iCount = 0;
					iY2 = ((y-k2)*info.dwEffWidth);
					for(int32_t j=-k2;j<kmax;j++, iY2+=info.dwEffWidth)
					{
						if (0>(y+j) || (y+j)>=head.biHeight) continue;
						iY = iY2+x;
						for(int32_t k=-k2;k<kmax;k++, iCount++)
						{
							if (0>(x+k) || (x+k)>=head.biWidth) continue;
							i=kernel[iCount];
							b += cPtr[iY+k] * i;
							ksumcur += i;
						}
					}
					if (Kfactor==0 || ksumcur==0){
						cPtr2[iY1] = (uint8_t)min(255, max(0,(int32_t)(b + Koffset)));
					} else if (ksumtot == ksumcur) {
						cPtr2[iY1] = (uint8_t)min(255, max(0,(int32_t)(b/Kfactor + Koffset)));
					} else {
						cPtr2[iY1] = (uint8_t)min(255, max(0,(int32_t)((b*ksumtot)/(ksumcur*Kfactor) + Koffset)));
					}
				}
			}
		}
	}
	else
	{
		for(int32_t y=ymin; y<ymax; y++){
			info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
			if (info.nEscape) break;
			for(int32_t x=xmin; x<xmax; x++){
	#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
	#endif //CXIMAGE_SUPPORT_SELECTION
					{
					r=b=g=ksumcur=0;
					for(int32_t j=-k2;j<kmax;j++){
						for(int32_t k=-k2;k<kmax;k++){
							if (!IsInside(x+j,y+k)) continue;
							c = BlindGetPixelColor(x+j,y+k);
							i = kernel[(j+k2)+Ksize*(k+k2)];
							r += c.rgbRed * i;
							g += c.rgbGreen * i;
							b += c.rgbBlue * i;
							ksumcur += i;
						}
					}
					if (Kfactor==0 || ksumcur==0){
						c.rgbRed   = (uint8_t)min(255, max(0,(int32_t)(r + Koffset)));
						c.rgbGreen = (uint8_t)min(255, max(0,(int32_t)(g + Koffset)));
						c.rgbBlue  = (uint8_t)min(255, max(0,(int32_t)(b + Koffset)));
					} else if (ksumtot == ksumcur) {
						c.rgbRed   = (uint8_t)min(255, max(0,(int32_t)(r/Kfactor + Koffset)));
						c.rgbGreen = (uint8_t)min(255, max(0,(int32_t)(g/Kfactor + Koffset)));
						c.rgbBlue  = (uint8_t)min(255, max(0,(int32_t)(b/Kfactor + Koffset)));
					} else {
						c.rgbRed   = (uint8_t)min(255, max(0,(int32_t)((r*ksumtot)/(ksumcur*Kfactor) + Koffset)));
						c.rgbGreen = (uint8_t)min(255, max(0,(int32_t)((g*ksumtot)/(ksumcur*Kfactor) + Koffset)));
						c.rgbBlue  = (uint8_t)min(255, max(0,(int32_t)((b*ksumtot)/(ksumcur*Kfactor) + Koffset)));
					}
					tmp.BlindSetPixelColor(x,y,c);
				}
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Enhance the dark areas of the image
 * \param Ksize: size of the kernel.
 * \return true if everything is ok
 */
bool CxImage::Erode(int32_t Ksize)
{
	if (!pDib) return false;

	int32_t k2 = Ksize/2;
	int32_t kmax= Ksize-k2;
	uint8_t r,g,b;
	RGBQUAD c;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				r=b=g=255;
				for(int32_t j=-k2;j<kmax;j++){
					for(int32_t k=-k2;k<kmax;k++){
						if (!IsInside(x+j,y+k)) continue;
						c = BlindGetPixelColor(x+j,y+k);
						if (c.rgbRed < r) r=c.rgbRed;
						if (c.rgbGreen < g) g=c.rgbGreen;
						if (c.rgbBlue < b) b=c.rgbBlue;
					}
				}
				c.rgbRed   = r;
				c.rgbGreen = g;
				c.rgbBlue  = b;
				tmp.BlindSetPixelColor(x,y,c);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Enhance the light areas of the image
 * \param Ksize: size of the kernel.
 * \return true if everything is ok
 */
bool CxImage::Dilate(int32_t Ksize)
{
	if (!pDib) return false;

	int32_t k2 = Ksize/2;
	int32_t kmax= Ksize-k2;
	uint8_t r,g,b;
	RGBQUAD c;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				r=b=g=0;
				for(int32_t j=-k2;j<kmax;j++){
					for(int32_t k=-k2;k<kmax;k++){
						if (!IsInside(x+j,y+k)) continue;
						c = BlindGetPixelColor(x+j,y+k);
						if (c.rgbRed > r) r=c.rgbRed;
						if (c.rgbGreen > g) g=c.rgbGreen;
						if (c.rgbBlue > b) b=c.rgbBlue;
					}
				}
				c.rgbRed   = r;
				c.rgbGreen = g;
				c.rgbBlue  = b;
				tmp.BlindSetPixelColor(x,y,c);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Enhance the variations between adjacent pixels.
 * Similar results can be achieved using Filter(),
 * but the algorithms are different both in Edge() and in Contour().
 * \param Ksize: size of the kernel.
 * \return true if everything is ok
 */
bool CxImage::Edge(int32_t Ksize)
{
	if (!pDib) return false;

	int32_t k2 = Ksize/2;
	int32_t kmax= Ksize-k2;
	uint8_t r,g,b,rr,gg,bb;
	RGBQUAD c;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				r=b=g=0;
				rr=bb=gg=255;
				for(int32_t j=-k2;j<kmax;j++){
					for(int32_t k=-k2;k<kmax;k++){
						if (!IsInside(x+j,y+k)) continue;
						c = BlindGetPixelColor(x+j,y+k);
						if (c.rgbRed > r) r=c.rgbRed;
						if (c.rgbGreen > g) g=c.rgbGreen;
						if (c.rgbBlue > b) b=c.rgbBlue;

						if (c.rgbRed < rr) rr=c.rgbRed;
						if (c.rgbGreen < gg) gg=c.rgbGreen;
						if (c.rgbBlue < bb) bb=c.rgbBlue;
					}
				}
				c.rgbRed   = (uint8_t)(255-abs(r-rr));
				c.rgbGreen = (uint8_t)(255-abs(g-gg));
				c.rgbBlue  = (uint8_t)(255-abs(b-bb));
				tmp.BlindSetPixelColor(x,y,c);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Blends two images
 * \param imgsrc2: image to be mixed with this
 * \param op: blending method; see ImageOpType
 * \param lXOffset, lYOffset: image displacement
 * \param bMixAlpha: if true and imgsrc2 has a valid alpha layer, it will be mixed in the destination image.
 * \return true if everything is ok
 * \author [Mwolski],[brunom]
 */
void CxImage::Mix(CxImage & imgsrc2, ImageOpType op, int32_t lXOffset, int32_t lYOffset, bool bMixAlpha)
{
    int32_t lWide = min(GetWidth(),imgsrc2.GetWidth()-lXOffset);
    int32_t lHeight = min(GetHeight(),imgsrc2.GetHeight()-lYOffset);

	bool bEditAlpha = false;

#if CXIMAGE_SUPPORT_ALPHA
	bEditAlpha = imgsrc2.AlphaIsValid() & bMixAlpha;
	if (bEditAlpha && AlphaIsValid()==false){
		AlphaCreate();
	}
#endif //CXIMAGE_SUPPORT_ALPHA

    RGBQUAD rgbBackgrnd1 = GetTransColor();
    RGBQUAD rgb1, rgb2, rgbDest;

    for(int32_t lY=0;lY<lHeight;lY++)
    {
		info.nProgress = (int32_t)(100*lY/head.biHeight);
		if (info.nEscape) break;

        for(int32_t lX=0;lX<lWide;lX++)
        {
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(lX,lY) && imgsrc2.SelectionIsInside(lX+lXOffset,lY+lYOffset))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				rgb1 = GetPixelColor(lX,lY);
				rgb2 = imgsrc2.GetPixelColor(lX+lXOffset,lY+lYOffset);
				switch(op)
				{
					case OpAvg:
						rgbDest.rgbBlue =  (uint8_t)((rgb1.rgbBlue+rgb2.rgbBlue)/2);
						rgbDest.rgbGreen = (uint8_t)((rgb1.rgbGreen+rgb2.rgbGreen)/2);
						rgbDest.rgbRed =   (uint8_t)((rgb1.rgbRed+rgb2.rgbRed)/2);
						if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)((rgb1.rgbReserved+rgb2.rgbReserved)/2);
					break;
					case OpAdd:
						rgbDest.rgbBlue = (uint8_t)max(0,min(255,rgb1.rgbBlue+rgb2.rgbBlue));
						rgbDest.rgbGreen = (uint8_t)max(0,min(255,rgb1.rgbGreen+rgb2.rgbGreen));
						rgbDest.rgbRed = (uint8_t)max(0,min(255,rgb1.rgbRed+rgb2.rgbRed));
						if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)max(0,min(255,rgb1.rgbReserved+rgb2.rgbReserved));
					break;
					case OpSub:
						rgbDest.rgbBlue = (uint8_t)max(0,min(255,rgb1.rgbBlue-rgb2.rgbBlue));
						rgbDest.rgbGreen = (uint8_t)max(0,min(255,rgb1.rgbGreen-rgb2.rgbGreen));
						rgbDest.rgbRed = (uint8_t)max(0,min(255,rgb1.rgbRed-rgb2.rgbRed));
						if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)max(0,min(255,rgb1.rgbReserved-rgb2.rgbReserved));
					break;
					case OpAnd:
						rgbDest.rgbBlue = (uint8_t)(rgb1.rgbBlue&rgb2.rgbBlue);
						rgbDest.rgbGreen = (uint8_t)(rgb1.rgbGreen&rgb2.rgbGreen);
						rgbDest.rgbRed = (uint8_t)(rgb1.rgbRed&rgb2.rgbRed);
						if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)(rgb1.rgbReserved&rgb2.rgbReserved);
					break;
					case OpXor:
						rgbDest.rgbBlue = (uint8_t)(rgb1.rgbBlue^rgb2.rgbBlue);
						rgbDest.rgbGreen = (uint8_t)(rgb1.rgbGreen^rgb2.rgbGreen);
						rgbDest.rgbRed = (uint8_t)(rgb1.rgbRed^rgb2.rgbRed);
						if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)(rgb1.rgbReserved^rgb2.rgbReserved);
					break;
					case OpOr:
						rgbDest.rgbBlue = (uint8_t)(rgb1.rgbBlue|rgb2.rgbBlue);
						rgbDest.rgbGreen = (uint8_t)(rgb1.rgbGreen|rgb2.rgbGreen);
						rgbDest.rgbRed = (uint8_t)(rgb1.rgbRed|rgb2.rgbRed);
						if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)(rgb1.rgbReserved|rgb2.rgbReserved);
					break;
					case OpMask:
						if(rgb2.rgbBlue==0 && rgb2.rgbGreen==0 && rgb2.rgbRed==0)
							rgbDest = rgbBackgrnd1;
						else
							rgbDest = rgb1;
						break;
					case OpSrcCopy:
						if(IsTransparent(lX,lY))
							rgbDest = rgb2;
						else // copy straight over
							rgbDest = rgb1;
						break;
					case OpDstCopy:
						if(imgsrc2.IsTransparent(lX+lXOffset,lY+lYOffset))
							rgbDest = rgb1;
						else // copy straight over
							rgbDest = rgb2;
						break;
					case OpScreen:
						{ 
							uint8_t a,a1; 
							
							if (imgsrc2.IsTransparent(lX+lXOffset,lY+lYOffset)){
								a=0;
#if CXIMAGE_SUPPORT_ALPHA
							} else if (imgsrc2.AlphaIsValid()){
								a=imgsrc2.AlphaGet(lX+lXOffset,lY+lYOffset);
								a =(uint8_t)((a*imgsrc2.info.nAlphaMax)/255);
#endif //CXIMAGE_SUPPORT_ALPHA
							} else {
								a=255;
							}

							if (a==0){ //transparent 
								rgbDest = rgb1; 
							} else if (a==255){ //opaque 
								rgbDest = rgb2; 
							} else { //blend 
								a1 = (uint8_t)~a; 
								rgbDest.rgbBlue = (uint8_t)((rgb1.rgbBlue*a1+rgb2.rgbBlue*a)/255); 
								rgbDest.rgbGreen = (uint8_t)((rgb1.rgbGreen*a1+rgb2.rgbGreen*a)/255); 
								rgbDest.rgbRed = (uint8_t)((rgb1.rgbRed*a1+rgb2.rgbRed*a)/255);  
							}

							if (bEditAlpha) rgbDest.rgbReserved = (uint8_t)((rgb1.rgbReserved*a)/255);
						} 
						break; 
					case OpSrcBlend:
						if(IsTransparent(lX,lY))
							rgbDest = rgb2;
						else
						{
							int32_t lBDiff = abs(rgb1.rgbBlue - rgbBackgrnd1.rgbBlue);
							int32_t lGDiff = abs(rgb1.rgbGreen - rgbBackgrnd1.rgbGreen);
							int32_t lRDiff = abs(rgb1.rgbRed - rgbBackgrnd1.rgbRed);

							double lAverage = (lBDiff+lGDiff+lRDiff)/3;
							double lThresh = 16;
							double dLarge = lAverage/lThresh;
							double dSmall = (lThresh-lAverage)/lThresh;
							double dSmallAmt = dSmall*((double)rgb2.rgbBlue);

							if( lAverage < lThresh+1){
								rgbDest.rgbBlue = (uint8_t)max(0,min(255,(int32_t)(dLarge*((double)rgb1.rgbBlue) +
												dSmallAmt)));
								rgbDest.rgbGreen = (uint8_t)max(0,min(255,(int32_t)(dLarge*((double)rgb1.rgbGreen) +
												dSmallAmt)));
								rgbDest.rgbRed = (uint8_t)max(0,min(255,(int32_t)(dLarge*((double)rgb1.rgbRed) +
												dSmallAmt)));
							}
							else
								rgbDest = rgb1;
						}
						break;
					case OpBlendAlpha:	//[brunom]
						if(rgb2.rgbReserved != 0)
						{
							// The lower value is almost transparent, or the overlying
							// almost transparent can not directly overlying the value taken
							if( (rgb1.rgbReserved < 5) || (rgb2.rgbReserved > 250) ){
								rgbDest = rgb2;
							} else {
								// Alpha Blending with associative calculation merge 
								// (http://en.wikipedia.org/wiki/Alpha_compositing)
								int32_t a0,a1,a2;
								// Transparency of the superimposed image
								a2 = rgb2.rgbReserved;
								// Calculation transparency of the underlying image
								a1 = (rgb1.rgbReserved * (255 - a2)) >> 8; 
								// total transparency of the new pixel
								a0 = a2 + a1;
								// New transparency assume (a0 == 0 is the restriction s.o. (range 5-250) intercepted) 
								if (bEditAlpha) rgbDest.rgbReserved = a0;
								// each color channel to calculate
								rgbDest.rgbBlue		= (BYTE)((rgb2.rgbBlue	* a2	+ a1 * rgb1.rgbBlue	)/a0);
								rgbDest.rgbGreen	= (BYTE)((rgb2.rgbGreen * a2	+ a1 * rgb1.rgbGreen)/a0);
								rgbDest.rgbRed		= (BYTE)((rgb2.rgbRed	* a2	+ a1 * rgb1.rgbRed	)/a0);
							}
						} else {
							rgbDest = rgb1;
							rgbDest.rgbReserved = 0;
						}
						break;
						default:
						return;
				}
				SetPixelColor(lX,lY,rgbDest,bEditAlpha);
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
// thanks to Kenneth Ballard
void CxImage::MixFrom(CxImage & imagesrc2, int32_t lXOffset, int32_t lYOffset)
{
    int32_t width = imagesrc2.GetWidth();
    int32_t height = imagesrc2.GetHeight();

    int32_t x, y;

	if (imagesrc2.IsTransparent()) {
		for(x = 0; x < width; x++) {
			for(y = 0; y < height; y++) {
				if(!imagesrc2.IsTransparent(x,y)){
					SetPixelColor(x + lXOffset, y + lYOffset, imagesrc2.BlindGetPixelColor(x, y));
				}
			}
		}
	} else { //no transparency so just set it <Matt>
		for(x = 0; x < width; x++) {
			for(y = 0; y < height; y++) {
				SetPixelColor(x + lXOffset, y + lYOffset, imagesrc2.BlindGetPixelColor(x, y)); 
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adjusts separately the red, green, and blue values in the image.
 * \param r, g, b: can be from -255 to +255.
 * \return true if everything is ok
 */
bool CxImage::ShiftRGB(int32_t r, int32_t g, int32_t b)
{
	if (!pDib) return false;
	RGBQUAD color;
	if (head.biClrUsed==0){

		int32_t xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			xmin = ymin = 0;
			xmax = head.biWidth; ymax=head.biHeight;
		}

		for(int32_t y=ymin; y<ymax; y++){
			for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = BlindGetPixelColor(x,y);
					color.rgbRed = (uint8_t)max(0,min(255,(int32_t)(color.rgbRed + r)));
					color.rgbGreen = (uint8_t)max(0,min(255,(int32_t)(color.rgbGreen + g)));
					color.rgbBlue = (uint8_t)max(0,min(255,(int32_t)(color.rgbBlue + b)));
					BlindSetPixelColor(x,y,color);
				}
			}
		}
	} else {
		for(uint32_t j=0; j<head.biClrUsed; j++){
			color = GetPaletteColor((uint8_t)j);
			color.rgbRed = (uint8_t)max(0,min(255,(int32_t)(color.rgbRed + r)));
			color.rgbGreen = (uint8_t)max(0,min(255,(int32_t)(color.rgbGreen + g)));
			color.rgbBlue = (uint8_t)max(0,min(255,(int32_t)(color.rgbBlue + b)));
			SetPaletteColor((uint8_t)j,color);
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adjusts the color balance of the image
 * \param gamma can be from 0.1 to 5.
 * \return true if everything is ok
 * \sa GammaRGB
 */
bool CxImage::Gamma(float gamma)
{
	if (!pDib) return false;

	if (gamma <= 0.0f) return false;

	double dinvgamma = 1/gamma;
	double dMax = pow(255.0, dinvgamma) / 255.0;

	uint8_t cTable[256]; //<nipper>
	for (int32_t i=0;i<256;i++)	{
		cTable[i] = (uint8_t)max(0,min(255,(int32_t)( pow((double)i, dinvgamma) / dMax)));
	}

	return Lut(cTable);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adjusts the color balance indipendent for each color channel
 * \param gammaR, gammaG, gammaB  can be from 0.1 to 5.
 * \return true if everything is ok
 * \sa Gamma
 */
bool CxImage::GammaRGB(float gammaR, float gammaG, float gammaB)
{
	if (!pDib) return false;

	if (gammaR <= 0.0f) return false;
	if (gammaG <= 0.0f) return false;
	if (gammaB <= 0.0f) return false;

	double dinvgamma, dMax;
	int32_t i;

	dinvgamma = 1/gammaR;
	dMax = pow(255.0, dinvgamma) / 255.0;
	uint8_t cTableR[256];
	for (i=0;i<256;i++)	{
		cTableR[i] = (uint8_t)max(0,min(255,(int32_t)( pow((double)i, dinvgamma) / dMax)));
	}

	dinvgamma = 1/gammaG;
	dMax = pow(255.0, dinvgamma) / 255.0;
	uint8_t cTableG[256];
	for (i=0;i<256;i++)	{
		cTableG[i] = (uint8_t)max(0,min(255,(int32_t)( pow((double)i, dinvgamma) / dMax)));
	}

	dinvgamma = 1/gammaB;
	dMax = pow(255.0, dinvgamma) / 255.0;
	uint8_t cTableB[256];
	for (i=0;i<256;i++)	{
		cTableB[i] = (uint8_t)max(0,min(255,(int32_t)( pow((double)i, dinvgamma) / dMax)));
	}

	return Lut(cTableR, cTableG, cTableB);
}
////////////////////////////////////////////////////////////////////////////////

//#if !defined (_WIN32_WCE)
/**
 * Adjusts the intensity of each pixel to the median intensity of its surrounding pixels.
 * \param Ksize: size of the kernel.
 * \return true if everything is ok
 */
bool CxImage::Median(int32_t Ksize)
{
	if (!pDib) return false;

	int32_t k2 = Ksize/2;
	int32_t kmax= Ksize-k2;
	int32_t i,j,k;

	RGBQUAD* kernel = (RGBQUAD*)malloc(Ksize*Ksize*sizeof(RGBQUAD));

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
				for(j=-k2, i=0;j<kmax;j++)
					for(k=-k2;k<kmax;k++)
						if (IsInside(x+j,y+k))
							kernel[i++]=BlindGetPixelColor(x+j,y+k);

				qsort(kernel, i, sizeof(RGBQUAD), CompareColors);
				tmp.SetPixelColor(x,y,kernel[i/2]);
			}
		}
	}
	free(kernel);
	Transfer(tmp);
	return true;
}
//#endif //_WIN32_WCE
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds an uniform noise to the image
 * \param level: can be from 0 (no noise) to 255 (lot of noise).
 * \return true if everything is ok
 */
bool CxImage::Noise(int32_t level)
{
	if (!pDib) return false;
	RGBQUAD color;

	int32_t xmin,xmax,ymin,ymax,n;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin)); //<zhanghk><Anatoly Ivasyuk>
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				color = BlindGetPixelColor(x,y);
				n=(int32_t)((rand()/(float)RAND_MAX - 0.5)*level);
				color.rgbRed = (uint8_t)max(0,min(255,(int32_t)(color.rgbRed + n)));
				n=(int32_t)((rand()/(float)RAND_MAX - 0.5)*level);
				color.rgbGreen = (uint8_t)max(0,min(255,(int32_t)(color.rgbGreen + n)));
				n=(int32_t)((rand()/(float)RAND_MAX - 0.5)*level);
				color.rgbBlue = (uint8_t)max(0,min(255,(int32_t)(color.rgbBlue + n)));
				BlindSetPixelColor(x,y,color);
			}
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Computes the bidimensional FFT or DFT of the image.
 * - The images are processed as grayscale
 * - If the dimensions of the image are a power of, 2 the FFT is performed automatically.
 * - If dstReal and/or dstImag are NULL, the resulting images replaces the original(s).
 * - Note: with 8 bits there is a HUGE loss in the dynamics. The function tries
 *   to keep an acceptable SNR, but 8bit = 48dB...
 *
 * \param srcReal, srcImag: source images: One can be NULL, but not both
 * \param dstReal, dstImag: destination images. Can be NULL.
 * \param direction: 1 = forward, -1 = inverse.
 * \param bForceFFT: if true, the images are resampled to make the dimensions a power of 2.
 * \param bMagnitude: if true, the real part returns the magnitude, the imaginary part returns the phase
 * \return true if everything is ok
 */
bool CxImage::FFT2(CxImage* srcReal, CxImage* srcImag, CxImage* dstReal, CxImage* dstImag,
				   int32_t direction, bool bForceFFT, bool bMagnitude)
{
	//check if there is something to convert
	if (srcReal==NULL && srcImag==NULL) return false;

	int32_t w,h;
	//get width and height
	if (srcReal) {
		w=srcReal->GetWidth();
		h=srcReal->GetHeight();
	} else {
		w=srcImag->GetWidth();
		h=srcImag->GetHeight();
	}

	bool bXpow2 = IsPowerof2(w);
	bool bYpow2 = IsPowerof2(h);
	//if bForceFFT, width AND height must be powers of 2
	if (bForceFFT && !(bXpow2 && bYpow2)) {
		int32_t i;
		
		i=0;
		while((1<<i)<w) i++;
		w=1<<i;
		bXpow2=true;

		i=0;
		while((1<<i)<h) i++;
		h=1<<i;
		bYpow2=true;
	}

	// I/O images for FFT
	CxImage *tmpReal,*tmpImag;

	// select output
	tmpReal = (dstReal) ? dstReal : srcReal;
	tmpImag = (dstImag) ? dstImag : srcImag;

	// src!=dst -> copy the image
	if (srcReal && dstReal) tmpReal->Copy(*srcReal,true,false,false);
	if (srcImag && dstImag) tmpImag->Copy(*srcImag,true,false,false);

	// dst&&src are empty -> create new one, else turn to GrayScale
	if (srcReal==0 && dstReal==0){
		tmpReal = new CxImage(w,h,8);
		tmpReal->Clear(0);
		tmpReal->SetGrayPalette();
	} else {
		if (!tmpReal->IsGrayScale()) tmpReal->GrayScale();
	}
	if (srcImag==0 && dstImag==0){
		tmpImag = new CxImage(w,h,8);
		tmpImag->Clear(0);
		tmpImag->SetGrayPalette();
	} else {
		if (!tmpImag->IsGrayScale()) tmpImag->GrayScale();
	}

	if (!(tmpReal->IsValid() && tmpImag->IsValid())){
		if (srcReal==0 && dstReal==0) delete tmpReal;
		if (srcImag==0 && dstImag==0) delete tmpImag;
		return false;
	}

	//resample for FFT, if necessary 
	tmpReal->Resample(w,h,0);
	tmpImag->Resample(w,h,0);

	//ok, here we have 2 (w x h), grayscale images ready for a FFT

	double* real;
	double* imag;
	int32_t j,k,m;

	_complex **grid;
	//double mean = tmpReal->Mean();
	/* Allocate memory for the grid */
	grid = (_complex **)malloc(w * sizeof(_complex));
	for (k=0;k<w;k++) {
		grid[k] = (_complex *)malloc(h * sizeof(_complex));
	}
	for (j=0;j<h;j++) {
		for (k=0;k<w;k++) {
			grid[k][j].x = tmpReal->GetPixelIndex(k,j)-128;
			grid[k][j].y = tmpImag->GetPixelIndex(k,j)-128;
		}
	}

	//DFT buffers
	double *real2,*imag2;
	real2 = (double*)malloc(max(w,h) * sizeof(double));
	imag2 = (double*)malloc(max(w,h) * sizeof(double));

	/* Transform the rows */
	real = (double *)malloc(w * sizeof(double));
	imag = (double *)malloc(w * sizeof(double));

	m=0;
	while((1<<m)<w) m++;

	for (j=0;j<h;j++) {
		for (k=0;k<w;k++) {
			real[k] = grid[k][j].x;
			imag[k] = grid[k][j].y;
		}

		if (bXpow2) FFT(direction,m,real,imag);
		else		DFT(direction,w,real,imag,real2,imag2);

		for (k=0;k<w;k++) {
			grid[k][j].x = real[k];
			grid[k][j].y = imag[k];
		}
	}
	free(real);
	free(imag);

	/* Transform the columns */
	real = (double *)malloc(h * sizeof(double));
	imag = (double *)malloc(h * sizeof(double));

	m=0;
	while((1<<m)<h) m++;

	for (k=0;k<w;k++) {
		for (j=0;j<h;j++) {
			real[j] = grid[k][j].x;
			imag[j] = grid[k][j].y;
		}

		if (bYpow2) FFT(direction,m,real,imag);
		else		DFT(direction,h,real,imag,real2,imag2);

		for (j=0;j<h;j++) {
			grid[k][j].x = real[j];
			grid[k][j].y = imag[j];
		}
	}
	free(real);
	free(imag);

	free(real2);
	free(imag2);

	/* converting from double to byte, there is a HUGE loss in the dynamics
	  "nn" tries to keep an acceptable SNR, but 8bit=48dB: don't ask more */
	double nn=pow((double)2,(double)log((double)max(w,h))/(double)log((double)2)-4);
	//reversed gain for reversed transform
	if (direction==-1) nn=1/nn;
	//bMagnitude : just to see it on the screen
	if (bMagnitude) nn*=4;

	for (j=0;j<h;j++) {
		for (k=0;k<w;k++) {
			if (bMagnitude){
				tmpReal->SetPixelIndex(k,j,(uint8_t)max(0,min(255,(nn*(3+log(_cabs(grid[k][j])))))));
				if (grid[k][j].x==0){
					tmpImag->SetPixelIndex(k,j,(uint8_t)max(0,min(255,(128+(atan(grid[k][j].y/0.0000000001)*nn)))));
				} else {
					tmpImag->SetPixelIndex(k,j,(uint8_t)max(0,min(255,(128+(atan(grid[k][j].y/grid[k][j].x)*nn)))));
				}
			} else {
				tmpReal->SetPixelIndex(k,j,(uint8_t)max(0,min(255,(128 + grid[k][j].x*nn))));
				tmpImag->SetPixelIndex(k,j,(uint8_t)max(0,min(255,(128 + grid[k][j].y*nn))));
			}
		}
	}

	for (k=0;k<w;k++) free (grid[k]);
	free (grid);

	if (srcReal==0 && dstReal==0) delete tmpReal;
	if (srcImag==0 && dstImag==0) delete tmpImag;

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::IsPowerof2(int32_t x)
{
	int32_t i=0;
	while ((1<<i)<x) i++;
	if (x==(1<<i)) return true;
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of n=2^m points.
   o(n)=n*log2(n)
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
   Written by Paul Bourke, July 1998
   FFT algorithm by Cooley and Tukey, 1965 
*/
bool CxImage::FFT(int32_t dir,int32_t m,double *x,double *y)
{
	int32_t nn,i,i1,j,k,i2,l,l1,l2;
	double c1,c2,tx,ty,t1,t2,u1,u2,z;

	/* Calculate the number of points */
	nn = 1<<m;

	/* Do the bit reversal */
	i2 = nn >> 1;
	j = 0;
	for (i=0;i<nn-1;i++) {
		if (i < j) {
			tx = x[i];
			ty = y[i];
			x[i] = x[j];
			y[i] = y[j];
			x[j] = tx;
			y[j] = ty;
		}
		k = i2;
		while (k <= j) {
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	/* Compute the FFT */
	c1 = -1.0;
	c2 = 0.0;
	l2 = 1;
	for (l=0;l<m;l++) {
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;
		for (j=0;j<l1;j++) {
			for (i=j;i<nn;i+=l2) {
				i1 = i + l1;
				t1 = u1 * x[i1] - u2 * y[i1];
				t2 = u1 * y[i1] + u2 * x[i1];
				x[i1] = x[i] - t1;
				y[i1] = y[i] - t2;
				x[i] += t1;
				y[i] += t2;
			}
			z =  u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = sqrt((1.0 - c1) / 2.0);
		if (dir == 1)
			c2 = -c2;
		c1 = sqrt((1.0 + c1) / 2.0);
	}

	/* Scaling for forward transform */
	if (dir == 1) {
		for (i=0;i<nn;i++) {
			x[i] /= (double)nn;
			y[i] /= (double)nn;
		}
	}

   return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
   Direct fourier transform o(n)=n^2
   Written by Paul Bourke, July 1998 
*/
bool CxImage::DFT(int32_t dir,int32_t m,double *x1,double *y1,double *x2,double *y2)
{
   int32_t i,k;
   double arg;
   double cosarg,sinarg;
   
   for (i=0;i<m;i++) {
      x2[i] = 0;
      y2[i] = 0;
      arg = - dir * 2.0 * PI * i / (double)m;
      for (k=0;k<m;k++) {
         cosarg = cos(k * arg);
         sinarg = sin(k * arg);
         x2[i] += (x1[k] * cosarg - y1[k] * sinarg);
         y2[i] += (x1[k] * sinarg + y1[k] * cosarg);
      }
   }
   
   /* Copy the data back */
   if (dir == 1) {
      for (i=0;i<m;i++) {
         x1[i] = x2[i] / m;
         y1[i] = y2[i] / m;
      }
   } else {
      for (i=0;i<m;i++) {
         x1[i] = x2[i];
         y1[i] = y2[i];
      }
   }
   
   return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Combines different color components into a single image
 * \param r,g,b: color channels
 * \param a: alpha layer, can be NULL
 * \param colorspace: 0 = RGB, 1 = HSL, 2 = YUV, 3 = YIQ, 4 = XYZ 
 * \return true if everything is ok
 */
bool CxImage::Combine(CxImage* r,CxImage* g,CxImage* b,CxImage* a, int32_t colorspace)
{
	if (r==0 || g==0 || b==0) return false;

	int32_t w = r->GetWidth();
	int32_t h = r->GetHeight();

	Create(w,h,24);

	g->Resample(w,h);
	b->Resample(w,h);

	if (a) {
		a->Resample(w,h);
#if CXIMAGE_SUPPORT_ALPHA
		AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	RGBQUAD c;
	for (int32_t y=0;y<h;y++){
		info.nProgress = (int32_t)(100*y/h); //<Anatoly Ivasyuk>
		for (int32_t x=0;x<w;x++){
			c.rgbRed=r->GetPixelIndex(x,y);
			c.rgbGreen=g->GetPixelIndex(x,y);
			c.rgbBlue=b->GetPixelIndex(x,y);
			switch (colorspace){
			case 1:
				BlindSetPixelColor(x,y,HSLtoRGB(c));
				break;
			case 2:
				BlindSetPixelColor(x,y,YUVtoRGB(c));
				break;
			case 3:
				BlindSetPixelColor(x,y,YIQtoRGB(c));
				break;
			case 4:
				BlindSetPixelColor(x,y,XYZtoRGB(c));
				break;
			default:
				BlindSetPixelColor(x,y,c);
			}
#if CXIMAGE_SUPPORT_ALPHA
			if (a) AlphaSet(x,y,a->GetPixelIndex(x,y));
#endif //CXIMAGE_SUPPORT_ALPHA
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Smart blurring to remove small defects, dithering or artifacts.
 * \param radius: normally between 0.01 and 0.5
 * \param niterations: should be trimmed with radius, to avoid blurring should be (radius*niterations)<1
 * \param colorspace: 0 = RGB, 1 = HSL, 2 = YUV, 3 = YIQ, 4 = XYZ 
 * \return true if everything is ok
 */
bool CxImage::Repair(float radius, int32_t niterations, int32_t colorspace)
{
	if (!IsValid()) return false;

	int32_t w = GetWidth();
	int32_t h = GetHeight();

	CxImage r,g,b;

	r.Create(w,h,8);
	g.Create(w,h,8);
	b.Create(w,h,8);

	switch (colorspace){
	case 1:
		SplitHSL(&r,&g,&b);
		break;
	case 2:
		SplitYUV(&r,&g,&b);
		break;
	case 3:
		SplitYIQ(&r,&g,&b);
		break;
	case 4:
		SplitXYZ(&r,&g,&b);
		break;
	default:
		SplitRGB(&r,&g,&b);
	}
	
	for (int32_t i=0; i<niterations; i++){
		RepairChannel(&r,radius);
		RepairChannel(&g,radius);
		RepairChannel(&b,radius);
	}

	CxImage* a=NULL;
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		a = new CxImage();
		AlphaSplit(a);
	}
#endif

	Combine(&r,&g,&b,a,colorspace);

	delete a;

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::RepairChannel(CxImage *ch, float radius)
{
	if (ch==NULL) return false;

	CxImage tmp(*ch);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t w = ch->GetWidth()-1;
	int32_t h = ch->GetHeight()-1;

	double correction,ix,iy,ixx,ixy,iyy;
	int32_t x,y,xy0,xp1,xm1,yp1,ym1;

	for(x=1; x<w; x++){
		for(y=1; y<h; y++){

			xy0 = ch->BlindGetPixelIndex(x,y);
			xm1 = ch->BlindGetPixelIndex(x-1,y);
			xp1 = ch->BlindGetPixelIndex(x+1,y);
			ym1 = ch->BlindGetPixelIndex(x,y-1);
			yp1 = ch->BlindGetPixelIndex(x,y+1);

			ix= (xp1-xm1)/2.0;
			iy= (yp1-ym1)/2.0;
			ixx= xp1 - 2.0 * xy0 + xm1;
			iyy= yp1 - 2.0 * xy0 + ym1;
			ixy=(ch->BlindGetPixelIndex(x+1,y+1) + ch->BlindGetPixelIndex(x-1,y-1) -
				 ch->BlindGetPixelIndex(x-1,y+1) - ch->BlindGetPixelIndex(x+1,y-1))/4.0;

			correction = ((1.0+iy*iy)*ixx - ix*iy*ixy + (1.0+ix*ix)*iyy)/(1.0+ix*ix+iy*iy);

			tmp.BlindSetPixelIndex(x,y,(uint8_t)min(255,max(0,(xy0 + radius * correction + 0.5))));
		}
	}

	for (x=0;x<=w;x++){
		for(y=0; y<=h; y+=h){
			xy0 = ch->BlindGetPixelIndex(x,y);
			xm1 = ch->GetPixelIndex(x-1,y);
			xp1 = ch->GetPixelIndex(x+1,y);
			ym1 = ch->GetPixelIndex(x,y-1);
			yp1 = ch->GetPixelIndex(x,y+1);

			ix= (xp1-xm1)/2.0;
			iy= (yp1-ym1)/2.0;
			ixx= xp1 - 2.0 * xy0 + xm1;
			iyy= yp1 - 2.0 * xy0 + ym1;
			ixy=(ch->GetPixelIndex(x+1,y+1) + ch->GetPixelIndex(x-1,y-1) -
				 ch->GetPixelIndex(x-1,y+1) - ch->GetPixelIndex(x+1,y-1))/4.0;

			correction = ((1.0+iy*iy)*ixx - ix*iy*ixy + (1.0+ix*ix)*iyy)/(1.0+ix*ix+iy*iy);

			tmp.BlindSetPixelIndex(x,y,(uint8_t)min(255,max(0,(xy0 + radius * correction + 0.5))));
		}
	}
	for (x=0;x<=w;x+=w){
		for (y=0;y<=h;y++){
			xy0 = ch->BlindGetPixelIndex(x,y);
			xm1 = ch->GetPixelIndex(x-1,y);
			xp1 = ch->GetPixelIndex(x+1,y);
			ym1 = ch->GetPixelIndex(x,y-1);
			yp1 = ch->GetPixelIndex(x,y+1);

			ix= (xp1-xm1)/2.0;
			iy= (yp1-ym1)/2.0;
			ixx= xp1 - 2.0 * xy0 + xm1;
			iyy= yp1 - 2.0 * xy0 + ym1;
			ixy=(ch->GetPixelIndex(x+1,y+1) + ch->GetPixelIndex(x-1,y-1) -
				 ch->GetPixelIndex(x-1,y+1) - ch->GetPixelIndex(x+1,y-1))/4.0;

			correction = ((1.0+iy*iy)*ixx - ix*iy*ixy + (1.0+ix*ix)*iyy)/(1.0+ix*ix+iy*iy);

			tmp.BlindSetPixelIndex(x,y,(uint8_t)min(255,max(0,(xy0 + radius * correction + 0.5))));
		}
	}

	ch->Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Enhance the variations between adjacent pixels.
 * Similar results can be achieved using Filter(),
 * but the algorithms are different both in Edge() and in Contour().
 * \return true if everything is ok
 */
bool CxImage::Contour()
{
	if (!pDib) return false;

	int32_t Ksize = 3;
	int32_t k2 = Ksize/2;
	int32_t kmax= Ksize-k2;
	int32_t i,j,k;
	uint8_t maxr,maxg,maxb;
	RGBQUAD pix1,pix2;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
				pix1 = BlindGetPixelColor(x,y);
				maxr=maxg=maxb=0;
				for(j=-k2, i=0;j<kmax;j++){
					for(k=-k2;k<kmax;k++, i++){
						if (!IsInside(x+j,y+k)) continue;
						pix2 = BlindGetPixelColor(x+j,y+k);
						if ((pix2.rgbBlue-pix1.rgbBlue)>maxb) maxb = pix2.rgbBlue;
						if ((pix2.rgbGreen-pix1.rgbGreen)>maxg) maxg = pix2.rgbGreen;
						if ((pix2.rgbRed-pix1.rgbRed)>maxr) maxr = pix2.rgbRed;
					}
				}
				pix1.rgbBlue=(uint8_t)(255-maxb);
				pix1.rgbGreen=(uint8_t)(255-maxg);
				pix1.rgbRed=(uint8_t)(255-maxr);
				tmp.BlindSetPixelColor(x,y,pix1);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds a random offset to each pixel in the image
 * \param radius: maximum pixel displacement
 * \return true if everything is ok
 */
bool CxImage::Jitter(int32_t radius)
{
	if (!pDib) return false;

	int32_t nx,ny;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				nx=x+(int32_t)((rand()/(float)RAND_MAX - 0.5)*(radius*2));
				ny=y+(int32_t)((rand()/(float)RAND_MAX - 0.5)*(radius*2));
				if (!IsInside(nx,ny)) {
					nx=x;
					ny=y;
				}
				if (head.biClrUsed==0){
					tmp.BlindSetPixelColor(x,y,BlindGetPixelColor(nx,ny));
				} else {
					tmp.BlindSetPixelIndex(x,y,BlindGetPixelIndex(nx,ny));
				}
#if CXIMAGE_SUPPORT_ALPHA
				tmp.AlphaSet(x,y,AlphaGet(nx,ny));
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/** 
 * generates a 1-D convolution matrix to be used for each pass of 
 * a two-pass gaussian blur.  Returns the length of the matrix.
 * \author [nipper]
 */
int32_t CxImage::gen_convolve_matrix (float radius, float **cmatrix_p)
{
	int32_t matrix_length;
	int32_t matrix_midpoint;
	float* cmatrix;
	int32_t i,j;
	float std_dev;
	float sum;
	
	/* we want to generate a matrix that goes out a certain radius
	* from the center, so we have to go out ceil(rad-0.5) pixels,
	* inlcuding the center pixel.  Of course, that's only in one direction,
	* so we have to go the same amount in the other direction, but not count
	* the center pixel again.  So we double the previous result and subtract
	* one.
	* The radius parameter that is passed to this function is used as
	* the standard deviation, and the radius of effect is the
	* standard deviation * 2.  It's a little confusing.
	* <DP> modified scaling, so that matrix_lenght = 1+2*radius parameter
	*/
	radius = (float)fabs(0.5*radius) + 0.25f;
	
	std_dev = radius;
	radius = std_dev * 2;
	
	/* go out 'radius' in each direction */
	matrix_length = int32_t (2 * ceil(radius-0.5) + 1);
	if (matrix_length <= 0) matrix_length = 1;
	matrix_midpoint = matrix_length/2 + 1;
	*cmatrix_p = new float[matrix_length];
	cmatrix = *cmatrix_p;
	
	/*  Now we fill the matrix by doing a numeric integration approximation
	* from -2*std_dev to 2*std_dev, sampling 50 points per pixel.
	* We do the bottom half, mirror it to the top half, then compute the
	* center point.  Otherwise asymmetric quantization errors will occur.
	*  The formula to integrate is e^-(x^2/2s^2).
	*/
	
	/* first we do the top (right) half of matrix */
	for (i = matrix_length/2 + 1; i < matrix_length; i++)
    {
		float base_x = i - (float)floor((float)(matrix_length/2)) - 0.5f;
		sum = 0;
		for (j = 1; j <= 50; j++)
		{
			if ( base_x+0.02*j <= radius ) 
				sum += (float)exp (-(base_x+0.02*j)*(base_x+0.02*j) / 
				(2*std_dev*std_dev));
		}
		cmatrix[i] = sum/50;
    }
	
	/* mirror the thing to the bottom half */
	for (i=0; i<=matrix_length/2; i++) {
		cmatrix[i] = cmatrix[matrix_length-1-i];
	}
	
	/* find center val -- calculate an odd number of quanta to make it symmetric,
	* even if the center point is weighted slightly higher than others. */
	sum = 0;
	for (j=0; j<=50; j++)
    {
		sum += (float)exp (-(0.5+0.02*j)*(0.5+0.02*j) /
			(2*std_dev*std_dev));
    }
	cmatrix[matrix_length/2] = sum/51;
	
	/* normalize the distribution by scaling the total sum to one */
	sum=0;
	for (i=0; i<matrix_length; i++) sum += cmatrix[i];
	for (i=0; i<matrix_length; i++) cmatrix[i] = cmatrix[i] / sum;
	
	return matrix_length;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * generates a lookup table for every possible product of 0-255 and
 * each value in the convolution matrix.  The returned array is
 * indexed first by matrix position, then by input multiplicand (?)
 * value.
 * \author [nipper]
 */
float* CxImage::gen_lookup_table (float *cmatrix, int32_t cmatrix_length)
{
	float* lookup_table = new float[cmatrix_length * 256];
	float* lookup_table_p = lookup_table;
	float* cmatrix_p      = cmatrix;
	
	for (int32_t i=0; i<cmatrix_length; i++)
    {
		for (int32_t j=0; j<256; j++)
		{
			*(lookup_table_p++) = *cmatrix_p * (float)j;
		}
		cmatrix_p++;
    }
	
	return lookup_table;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * this function is written as if it is blurring a column at a time,
 * even though it can operate on rows, too.  There is no difference
 * in the processing of the lines, at least to the blur_line function.
 * \author [nipper]
 */
void CxImage::blur_line (float *ctable, float *cmatrix, int32_t cmatrix_length, uint8_t* cur_col, uint8_t* dest_col, int32_t y, int32_t bytes)
{
	float scale;
	float sum;
	int32_t i=0, j=0;
	int32_t row;
	int32_t cmatrix_middle = cmatrix_length/2;
	
	float *cmatrix_p;
	uint8_t  *cur_col_p;
	uint8_t  *cur_col_p1;
	uint8_t  *dest_col_p;
	float *ctable_p;
	
	/* this first block is the same as the non-optimized version --
	* it is only used for very small pictures, so speed isn't a
	* big concern.
	*/
	if (cmatrix_length > y)
    {
		for (row = 0; row < y ; row++)
		{
			scale=0;
			/* find the scale factor */
			for (j = 0; j < y ; j++)
			{
				/* if the index is in bounds, add it to the scale counter */
				if ((j + cmatrix_middle - row >= 0) &&
					(j + cmatrix_middle - row < cmatrix_length))
					scale += cmatrix[j + cmatrix_middle - row];
			}
			for (i = 0; i<bytes; i++)
			{
				sum = 0;
				for (j = 0; j < y; j++)
				{
					if ((j >= row - cmatrix_middle) &&
						(j <= row + cmatrix_middle))
						sum += cur_col[j*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (uint8_t)(0.5f + sum / scale);
			}
		}
    }
	else
    {
		/* for the edge condition, we only use available info and scale to one */
		for (row = 0; row < cmatrix_middle; row++)
		{
			/* find scale factor */
			scale=0;
			for (j = cmatrix_middle - row; j<cmatrix_length; j++)
				scale += cmatrix[j];
			for (i = 0; i<bytes; i++)
			{
				sum = 0;
				for (j = cmatrix_middle - row; j<cmatrix_length; j++)
				{
					sum += cur_col[(row + j-cmatrix_middle)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (uint8_t)(0.5f + sum / scale);
			}
		}
		/* go through each pixel in each col */
		dest_col_p = dest_col + row*bytes;
		for (; row < y-cmatrix_middle; row++)
		{
			cur_col_p = (row - cmatrix_middle) * bytes + cur_col;
			for (i = 0; i<bytes; i++)
			{
				sum = 0;
				cmatrix_p = cmatrix;
				cur_col_p1 = cur_col_p;
				ctable_p = ctable;
				for (j = cmatrix_length; j>0; j--)
				{
					sum += *(ctable_p + *cur_col_p1);
					cur_col_p1 += bytes;
					ctable_p += 256;
				}
				cur_col_p++;
				*(dest_col_p++) = (uint8_t)(0.5f + sum);
			}
		}
		
		/* for the edge condition , we only use available info, and scale to one */
		for (; row < y; row++)
		{
			/* find scale factor */
			scale=0;
			for (j = 0; j< y-row + cmatrix_middle; j++)
				scale += cmatrix[j];
			for (i = 0; i<bytes; i++)
			{
				sum = 0;
				for (j = 0; j<y-row + cmatrix_middle; j++)
				{
					sum += cur_col[(row + j-cmatrix_middle)*bytes + i] * cmatrix[j];
				}
				dest_col[row*bytes + i] = (uint8_t) (0.5f + sum / scale);
			}
		}
    }
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \author [DP]
 */
void CxImage::blur_text (uint8_t threshold, uint8_t decay, uint8_t max_depth, CxImage* iSrc, CxImage* iDst, uint8_t bytes)
{
	int32_t x,y,z,m;
	uint8_t *pSrc, *pSrc2, *pSrc3, *pDst;
	uint8_t step,n;
	int32_t pivot;

	if (max_depth<1) max_depth = 1;

	int32_t nmin,nmax,xmin,xmax,ymin,ymax;
	xmin = ymin = 0;
	xmax = iSrc->head.biWidth;
	ymax = iSrc->head.biHeight;

	if (xmin==xmax || ymin==ymax) return;

	nmin = xmin * bytes;
	nmax = xmax * bytes;

	CImageIterator itSrc(iSrc);
	CImageIterator itTmp(iDst);

	double dbScaler = 100.0f/(ymax-ymin)/bytes;

	for (n=0; n<bytes; n++){
		for (y=ymin+1;y<(ymax-1);y++)
		{
			if (info.nEscape) break;
			info.nProgress = (int32_t)((y-ymin)*dbScaler*(1+n));

			pSrc  = itSrc.GetRow(y);
			pSrc2 = itSrc.GetRow(y+1);
			pSrc3 = itSrc.GetRow(y-1);
			pDst  = itTmp.GetRow(y);

			//scan left to right
			for (x=n+nmin /*,i=xmin*/; x<(nmax-1); x+=bytes /*,i++*/)
			{
				z=x+bytes;
				pivot = pSrc[z]-threshold;
				//find upper corner
				if (pSrc[x]<pivot && pSrc2[z]<pivot && pSrc3[x]>=pivot){
					while (z<nmax && pSrc2[z]<pSrc[x+bytes] && pSrc[x+bytes]<=pSrc[z]){
						z+=bytes;
					}
					m = z-x;
					m = (decay>1) ? ((m/bytes)/decay+1) : m/bytes;
					if (m>max_depth) m = max_depth;
					step = (uint8_t)((pSrc[x+bytes]-pSrc[x])/(m+1));
					while (m-->1){
						pDst[x+m*bytes] = (uint8_t)(pDst[x]+(step*(m+1)));
					}
				}
				//find lower corner
				z=x+bytes;
				if (pSrc[x]<pivot && pSrc3[z]<pivot && pSrc2[x]>=pivot){
					while (z<nmax && pSrc3[z]<pSrc[x+bytes] && pSrc[x+bytes]<=pSrc[z]){
						z+=bytes;
					}
					m = z-x;
					m = (decay>1) ? ((m/bytes)/decay+1) : m/bytes;
					if (m>max_depth) m = max_depth;
					step = (uint8_t)((pSrc[x+bytes]-pSrc[x])/(m+1));
					while (m-->1){
						pDst[x+m*bytes] = (uint8_t)(pDst[x]+(step*(m+1)));
					}
				}
			}
			//scan right to left
			for (x=nmax-1-n /*,i=(xmax-1)*/; x>0; x-=bytes /*,i--*/)
			{
				z=x-bytes;
				pivot = pSrc[z]-threshold;
				//find upper corner
				if (pSrc[x]<pivot && pSrc2[z]<pivot && pSrc3[x]>=pivot){
					while (z>n && pSrc2[z]<pSrc[x-bytes] && pSrc[x-bytes]<=pSrc[z]){
						z-=bytes;
					}
					m = x-z;
					m = (decay>1) ? ((m/bytes)/decay+1) : m/bytes;
					if (m>max_depth) m = max_depth;
					step = (uint8_t)((pSrc[x-bytes]-pSrc[x])/(m+1));
					while (m-->1){
						pDst[x-m*bytes] = (uint8_t)(pDst[x]+(step*(m+1)));
					}
				}
				//find lower corner
				z=x-bytes;
				if (pSrc[x]<pivot && pSrc3[z]<pivot && pSrc2[x]>=pivot){
					while (z>n && pSrc3[z]<pSrc[x-bytes] && pSrc[x-bytes]<=pSrc[z]){
						z-=bytes;
					}
					m = x-z;
					m = (decay>1) ? ((m/bytes)/decay+1) : m/bytes;
					if (m>max_depth) m = max_depth;
					step = (uint8_t)((pSrc[x-bytes]-pSrc[x])/(m+1));
					while (m-->1){
						pDst[x-m*bytes] = (uint8_t)(pDst[x]+(step*(m+1)));
					}
				}
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \author [DP]
 */
bool CxImage::TextBlur(uint8_t threshold, uint8_t decay, uint8_t max_depth, bool bBlurHorizontal, bool bBlurVertical, CxImage* iDst)
{
	if (!pDib) return false;

	RGBQUAD* pPalette=NULL;
	uint16_t bpp = GetBpp();

	//the routine is optimized for RGB or GrayScale images
	if (!(head.biBitCount == 24 || IsGrayScale())){
		pPalette = new RGBQUAD[head.biClrUsed];
		memcpy(pPalette, GetPalette(),GetPaletteSize());
		if (!IncreaseBpp(24))
			return false;
	}

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	if (bBlurHorizontal)
		blur_text(threshold, decay, max_depth, this, &tmp, head.biBitCount>>3);

	if (bBlurVertical){
		CxImage src2(*this);
		src2.RotateLeft();
		tmp.RotateLeft();
		blur_text(threshold, decay, max_depth, &src2, &tmp, head.biBitCount>>3);
		tmp.RotateRight();
	}

#if CXIMAGE_SUPPORT_SELECTION
	//restore the non selected region
	if (pSelection){
		for(int32_t y=0; y<head.biHeight; y++){
			for(int32_t x=0; x<head.biWidth; x++){
				if (!BlindSelectionIsInside(x,y)){
					tmp.BlindSetPixelColor(x,y,BlindGetPixelColor(x,y));
				}
			}
		}
	}
#endif //CXIMAGE_SUPPORT_SELECTION

	//if necessary, restore the original BPP and palette
	if (pPalette){
		tmp.DecreaseBpp(bpp, true, pPalette);
		delete [] pPalette;
	}

	if (iDst) iDst->Transfer(tmp);
	else Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \author [nipper]; changes [DP]
 */
bool CxImage::GaussianBlur(float radius /*= 1.0f*/, CxImage* iDst /*= 0*/)
{
	if (!pDib) return false;

	RGBQUAD* pPalette=NULL;
	uint16_t bpp = GetBpp();

	//the routine is optimized for RGB or GrayScale images
	if (!(head.biBitCount == 24 || IsGrayScale())){
		pPalette = new RGBQUAD[head.biClrUsed];
		memcpy(pPalette, GetPalette(),GetPaletteSize());
		if (!IncreaseBpp(24))
			return false;
	}

	CxImage tmp_x(*this, false, true, true);
	if (!tmp_x.IsValid()){
		strcpy(info.szLastError,tmp_x.GetLastError());
		return false;
	}

	// generate convolution matrix and make sure it's smaller than each dimension
	float *cmatrix = NULL;
	int32_t cmatrix_length = gen_convolve_matrix(radius, &cmatrix);
	// generate lookup table
	float *ctable = gen_lookup_table(cmatrix, cmatrix_length);

	int32_t x,y;
	int32_t bypp = head.biBitCount>>3;

	CImageIterator itSrc(this);
	CImageIterator itTmp(&tmp_x);

	double dbScaler = 50.0f/head.biHeight;

	// blur the rows
    for (y=0;y<head.biHeight;y++)
	{
		if (info.nEscape) break;
		info.nProgress = (int32_t)(y*dbScaler);

		blur_line(ctable, cmatrix, cmatrix_length, itSrc.GetRow(y), itTmp.GetRow(y), head.biWidth, bypp);
	}

	CxImage tmp_y(tmp_x, false, true, true);
	if (!tmp_y.IsValid()){
		strcpy(info.szLastError,tmp_y.GetLastError());
		return false;
	}

	CImageIterator itDst(&tmp_y);

	// blur the cols
    uint8_t* cur_col = (uint8_t*)malloc(bypp*head.biHeight);
    uint8_t* dest_col = (uint8_t*)malloc(bypp*head.biHeight);

	dbScaler = 50.0f/head.biWidth;

	for (x=0;x<head.biWidth;x++)
	{
		if (info.nEscape) break;
		info.nProgress = (int32_t)(50.0f+x*dbScaler);

		itTmp.GetCol(cur_col, x);
		itDst.GetCol(dest_col, x);
		blur_line(ctable, cmatrix, cmatrix_length, cur_col, dest_col, head.biHeight, bypp);
		itDst.SetCol(dest_col, x);
	}

	free(cur_col);
	free(dest_col);

	delete [] cmatrix;
	delete [] ctable;

#if CXIMAGE_SUPPORT_SELECTION
	//restore the non selected region
	if (pSelection){
		for(y=0; y<head.biHeight; y++){
			for(x=0; x<head.biWidth; x++){
				if (!BlindSelectionIsInside(x,y)){
					tmp_y.BlindSetPixelColor(x,y,BlindGetPixelColor(x,y));
				}
			}
		}
	}
#endif //CXIMAGE_SUPPORT_SELECTION

	//if necessary, restore the original BPP and palette
	if (pPalette){
		tmp_y.DecreaseBpp(bpp, false, pPalette);
		if (iDst) DecreaseBpp(bpp, false, pPalette);
		delete [] pPalette;
	}

	if (iDst) iDst->Transfer(tmp_y);
	else Transfer(tmp_y);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \author [DP],[nipper]
 */
bool CxImage::SelectiveBlur(float radius, uint8_t threshold, CxImage* iDst)
{
	if (!pDib) return false;

	RGBQUAD* pPalette=NULL;
	uint16_t bpp = GetBpp();

	CxImage Tmp(*this, true, true, true);
	if (!Tmp.IsValid()){
		strcpy(info.szLastError,Tmp.GetLastError());
		return false;
	}

	//the routine is optimized for RGB or GrayScale images
	if (!(head.biBitCount == 24 || IsGrayScale())){
		pPalette = new RGBQUAD[head.biClrUsed];
		memcpy(pPalette, GetPalette(),GetPaletteSize());
		if (!Tmp.IncreaseBpp(24)){
			delete [] pPalette;
			return false;
		}
	}

	CxImage Dst(Tmp, true, true, true);
	if (!Dst.IsValid()){
		strcpy(info.szLastError,Dst.GetLastError());
		delete [] pPalette;
		return false;
	}

	//build the difference mask
	uint8_t thresh_dw = (uint8_t)max( 0 ,(int32_t)(128 - threshold));
	uint8_t thresh_up = (uint8_t)min(255,(int32_t)(128 + threshold));
	int32_t kernel[]={-100,-100,-100,-100,801,-100,-100,-100,-100};
	if (!Tmp.Filter(kernel,3,800,128)){
		strcpy(info.szLastError,Tmp.GetLastError());
		delete [] pPalette;
		return false;
	}

	//if the image has no selection, build a selection for the whole image
#if CXIMAGE_SUPPORT_SELECTION
	if (!Tmp.SelectionIsValid()){
		Tmp.SelectionCreate();
		Tmp.SelectionClear(255);
	}

	int32_t xmin,xmax,ymin,ymax;
	xmin = Tmp.info.rSelectionBox.left;
	xmax = Tmp.info.rSelectionBox.right;
	ymin = Tmp.info.rSelectionBox.bottom;
	ymax = Tmp.info.rSelectionBox.top;

	//modify the selection where the difference mask is over the threshold
	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
			if(Tmp.BlindSelectionIsInside(x,y)){
				RGBQUAD c = Tmp.BlindGetPixelColor(x,y);
				if ((c.rgbRed   < thresh_dw || c.rgbRed   > thresh_up) ||
					(c.rgbGreen < thresh_dw || c.rgbGreen > thresh_up) ||
					(c.rgbBlue  < thresh_dw || c.rgbBlue  > thresh_up))
				{
					Tmp.SelectionSet(x,y,0);
				}
			}
		}
	}

	//blur the image (only in the selected pixels)
	Dst.SelectionCopy(Tmp);
	if (!Dst.GaussianBlur(radius)){
		strcpy(info.szLastError,Dst.GetLastError());
		delete [] pPalette;
		return false;
	}

	//restore the original selection
	Dst.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

	//if necessary, restore the original BPP and palette
	if (pPalette){
		Dst.DecreaseBpp(bpp, false, pPalette);
		delete [] pPalette;
	}

	if (iDst) iDst->Transfer(Dst);
	else Transfer(Dst);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * sharpen the image by subtracting a blurred copy from the original image.
 * \param radius: width in pixels of the blurring effect. Range: >0; default = 5.
 * \param amount: strength of the filter. Range: 0.0 (none) to 1.0 (max); default = 0.5
 * \param threshold: difference, between blurred and original pixel, to trigger the filter
 *                   Range: 0 (always triggered) to 255 (never triggered); default = 0.
 * \return true if everything is ok
 * \author [nipper]; changes [DP]
 */
bool CxImage::UnsharpMask(float radius /*= 5.0*/, float amount /*= 0.5*/, int32_t threshold /*= 0*/)
{
	if (!pDib) return false;

	RGBQUAD* pPalette=NULL;
	uint16_t bpp = GetBpp();

	//the routine is optimized for RGB or GrayScale images
	if (!(head.biBitCount == 24 || IsGrayScale())){
		pPalette = new RGBQUAD[head.biClrUsed];
		memcpy(pPalette, GetPalette(),GetPaletteSize());
		if (!IncreaseBpp(24))
			return false;
	}

	CxImage iDst;
	if (!GaussianBlur(radius,&iDst))
		return false;

	CImageIterator itSrc(this);
	CImageIterator itDst(&iDst);

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	if (xmin==xmax || ymin==ymax)
		return false;

	double dbScaler = 100.0/(ymax-ymin);
	int32_t bypp = head.biBitCount>>3;
	
	// merge the source and destination (which currently contains
	// the blurred version) images
    for (int32_t y=ymin; y<ymax; y++)
	{
		if (info.nEscape) break;
		info.nProgress = (int32_t)((y-ymin)*dbScaler);

		// get source row
		uint8_t* cur_row = itSrc.GetRow(y);
		// get dest row
		uint8_t* dest_row = itDst.GetRow(y);
		// combine the two
		for (int32_t x=xmin; x<xmax; x++) {
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				for (int32_t b=0, z=x*bypp; b<bypp; b++, z++){
					int32_t diff = cur_row[z] - dest_row[z];

					// do tresholding
					if (abs(diff) < threshold){
						dest_row[z] = cur_row[z];
					} else {
						dest_row[z] = (uint8_t)min(255, max(0,(int32_t)(cur_row[z] + amount * diff)));
					}
				}
			}
		}
	}

	//if necessary, restore the original BPP and palette
	if (pPalette){
		iDst.DecreaseBpp(bpp, false, pPalette);
		delete [] pPalette;
	}

	Transfer(iDst);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Apply a look up table to the image. 
 * \param pLut: uint8_t[256] look up table
 * \return true if everything is ok
 */
bool CxImage::Lut(uint8_t* pLut)
{
	if (!pDib || !pLut) return false;
	RGBQUAD color;

	double dbScaler;
	if (head.biClrUsed==0){

		int32_t xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			// faster loop for full image
			uint8_t *iSrc=info.pImage;
			for(uint32_t i=0; i < head.biSizeImage ; i++){
				*iSrc++ = pLut[*iSrc];
			}
			return true;
		}

		if (xmin==xmax || ymin==ymax)
			return false;

		dbScaler = 100.0/(ymax-ymin);

		for(int32_t y=ymin; y<ymax; y++){
			info.nProgress = (int32_t)((y-ymin)*dbScaler); //<Anatoly Ivasyuk>
			for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = BlindGetPixelColor(x,y);
					color.rgbRed = pLut[color.rgbRed];
					color.rgbGreen = pLut[color.rgbGreen];
					color.rgbBlue = pLut[color.rgbBlue];
					BlindSetPixelColor(x,y,color);
				}
			}
		}
#if CXIMAGE_SUPPORT_SELECTION
	} else if (pSelection && (head.biBitCount==8) && IsGrayScale()){
		int32_t xmin,xmax,ymin,ymax;
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;

		if (xmin==xmax || ymin==ymax)
			return false;

		dbScaler = 100.0/(ymax-ymin);
		for(int32_t y=ymin; y<ymax; y++){
			info.nProgress = (int32_t)((y-ymin)*dbScaler);
			for(int32_t x=xmin; x<xmax; x++){
				if (BlindSelectionIsInside(x,y))
				{
					BlindSetPixelIndex(x,y,pLut[BlindGetPixelIndex(x,y)]);
				}
			}
		}
#endif //CXIMAGE_SUPPORT_SELECTION
	} else {
		bool bIsGrayScale = IsGrayScale();
		for(uint32_t j=0; j<head.biClrUsed; j++){
			color = GetPaletteColor((uint8_t)j);
			color.rgbRed = pLut[color.rgbRed];
			color.rgbGreen = pLut[color.rgbGreen];
			color.rgbBlue = pLut[color.rgbBlue];
			SetPaletteColor((uint8_t)j,color);
		}
		if (bIsGrayScale) GrayScale();
	}
	return true;

}
////////////////////////////////////////////////////////////////////////////////
/**
 * Apply an indipendent look up table for each channel
 * \param pLutR, pLutG, pLutB, pLutA: uint8_t[256] look up tables
 * \return true if everything is ok
 */
bool CxImage::Lut(uint8_t* pLutR, uint8_t* pLutG, uint8_t* pLutB, uint8_t* pLutA)
{
	if (!pDib || !pLutR || !pLutG || !pLutB) return false;
	RGBQUAD color;

	double dbScaler;
	if (head.biClrUsed==0){

		int32_t xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			xmin = ymin = 0;
			xmax = head.biWidth; ymax=head.biHeight;
		}

		if (xmin==xmax || ymin==ymax)
			return false;

		dbScaler = 100.0/(ymax-ymin);

		for(int32_t y=ymin; y<ymax; y++){
			info.nProgress = (int32_t)((y-ymin)*dbScaler);
			for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = BlindGetPixelColor(x,y);
					color.rgbRed =   pLutR[color.rgbRed];
					color.rgbGreen = pLutG[color.rgbGreen];
					color.rgbBlue =  pLutB[color.rgbBlue];
					if (pLutA) color.rgbReserved=pLutA[color.rgbReserved];
					BlindSetPixelColor(x,y,color,true);
				}
			}
		}
	} else {
		bool bIsGrayScale = IsGrayScale();
		for(uint32_t j=0; j<head.biClrUsed; j++){
			color = GetPaletteColor((uint8_t)j);
			color.rgbRed =   pLutR[color.rgbRed];
			color.rgbGreen = pLutG[color.rgbGreen];
			color.rgbBlue =  pLutB[color.rgbBlue];
			SetPaletteColor((uint8_t)j,color);
		}
		if (bIsGrayScale) GrayScale();
	}

	return true;

}
////////////////////////////////////////////////////////////////////////////////
/**
 * Use the RedEyeRemove function to remove the red-eye effect that frequently
 * occurs in photographs of humans and animals. You must select the region 
 * where the function will filter the red channel.
 * \param strength: range from 0.0f (no effect) to 1.0f (full effect). Default = 0.8
 * \return true if everything is ok
 */
bool CxImage::RedEyeRemove(float strength)
{
	if (!pDib) return false;
	RGBQUAD color;

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	if (xmin==xmax || ymin==ymax)
		return false;

	if (strength<0.0f) strength = 0.0f;
	if (strength>1.0f) strength = 1.0f;

	for(int32_t y=ymin; y<ymax; y++){
		info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				float a = 1.0f-5.0f*((float)((x-0.5f*(xmax+xmin))*(x-0.5f*(xmax+xmin))+(y-0.5f*(ymax+ymin))*(y-0.5f*(ymax+ymin))))/((float)((xmax-xmin)*(ymax-ymin)));
				if (a<0) a=0;
				color = BlindGetPixelColor(x,y);
				color.rgbRed = (uint8_t)(a*min(color.rgbGreen,color.rgbBlue)+(1.0f-a)*color.rgbRed);
				BlindSetPixelColor(x,y,color);
			}
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Changes the saturation of the image. 
 * \param saturation: can be from -100 to 100, positive values increase the saturation.
 * \param colorspace: can be 1 (HSL) or 2 (YUV).
 * \return true if everything is ok
 */
bool CxImage::Saturate(const int32_t saturation, const int32_t colorspace)
{
	if (!pDib)
		return false;

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	if (xmin==xmax || ymin==ymax)
		return false;

	uint8_t cTable[256];

	switch(colorspace)
	{
	case 1:
		{
			for (int32_t i=0;i<256;i++)	{
				cTable[i] = (uint8_t)max(0,min(255,(int32_t)(i + saturation)));
			}
			for(int32_t y=ymin; y<ymax; y++){
				info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
				if (info.nEscape) break;
				for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
					if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
					{
						RGBQUAD c = RGBtoHSL(BlindGetPixelColor(x,y));
						c.rgbGreen  = cTable[c.rgbGreen];
						c = HSLtoRGB(c);
						BlindSetPixelColor(x,y,c);
					}
				}
			}
		}
		break;
	case 2:
		{
			for (int32_t i=0;i<256;i++)	{
				cTable[i] = (uint8_t)max(0,min(255,(int32_t)((i-128)*(100 + saturation)/100.0f + 128.5f)));
			}
			for(int32_t y=ymin; y<ymax; y++){
				info.nProgress = (int32_t)(100*(y-ymin)/(ymax-ymin));
				if (info.nEscape) break;
				for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
					if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
					{
						RGBQUAD c = RGBtoYUV(BlindGetPixelColor(x,y));
						c.rgbGreen  = cTable[c.rgbGreen];
						c.rgbBlue = cTable[c.rgbBlue];
						c = YUVtoRGB(c);
						BlindSetPixelColor(x,y,c);
					}
				}
			}
		}
		break;
	default:
		strcpy(info.szLastError,"Saturate: wrong colorspace");
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Solarize: convert all colors above a given lightness level into their negative
 * \param  level : lightness threshold. Range = 0 to 255; default = 128.
 * \param  bLinkedChannels: true = compare with luminance, preserve colors (default)
 *                         false = compare with independent R,G,B levels
 * \return true if everything is ok
 * \author [Priyank Bolia] (priyank_bolia(at)yahoo(dot)com); changes [DP]
 */
bool CxImage::Solarize(uint8_t level, bool bLinkedChannels)
{
	if (!pDib) return false;

	int32_t xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	if (head.biBitCount<=8){
		if (IsGrayScale()){ //GRAYSCALE, selection
			for(int32_t y=ymin; y<ymax; y++){
				for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
					if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
					{
						uint8_t index = BlindGetPixelIndex(x,y);
						RGBQUAD color = GetPaletteColor(index);
						if ((uint8_t)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue)>level){
							BlindSetPixelIndex(x,y,255-index);
						}
					}
				}
			}
		} else { //PALETTE, full image
			RGBQUAD* ppal=GetPalette();
			for(uint32_t i=0;i<head.biClrUsed;i++){
				RGBQUAD color = GetPaletteColor((uint8_t)i);
				if (bLinkedChannels){
					if ((uint8_t)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue)>level){
						ppal[i].rgbBlue =(uint8_t)(255-ppal[i].rgbBlue);
						ppal[i].rgbGreen =(uint8_t)(255-ppal[i].rgbGreen);
						ppal[i].rgbRed =(uint8_t)(255-ppal[i].rgbRed);
					}
				} else {
					if (color.rgbBlue>level)	ppal[i].rgbBlue =(uint8_t)(255-ppal[i].rgbBlue);
					if (color.rgbGreen>level)	ppal[i].rgbGreen =(uint8_t)(255-ppal[i].rgbGreen);
					if (color.rgbRed>level)		ppal[i].rgbRed =(uint8_t)(255-ppal[i].rgbRed);
				}
			}
		}
	} else { //RGB, selection
		for(int32_t y=ymin; y<ymax; y++){
			for(int32_t x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					RGBQUAD color = BlindGetPixelColor(x,y);
					if (bLinkedChannels){
						if ((uint8_t)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue)>level){
							color.rgbRed = (uint8_t)(255-color.rgbRed);
							color.rgbGreen = (uint8_t)(255-color.rgbGreen);
							color.rgbBlue = (uint8_t)(255-color.rgbBlue);
						}
					} else {
						if (color.rgbBlue>level)	color.rgbBlue =(uint8_t)(255-color.rgbBlue);
						if (color.rgbGreen>level)	color.rgbGreen =(uint8_t)(255-color.rgbGreen);
						if (color.rgbRed>level)		color.rgbRed =(uint8_t)(255-color.rgbRed);
					}
					BlindSetPixelColor(x,y,color);
				}
			}
		}
	}

	//invert transparent color only in case of full image processing
	if (pSelection==0 || (!IsGrayScale() && IsIndexed())){
		if (bLinkedChannels){
			if ((uint8_t)RGB2GRAY(info.nBkgndColor.rgbRed,info.nBkgndColor.rgbGreen,info.nBkgndColor.rgbBlue)>level){
				info.nBkgndColor.rgbBlue = (uint8_t)(255-info.nBkgndColor.rgbBlue);
				info.nBkgndColor.rgbGreen = (uint8_t)(255-info.nBkgndColor.rgbGreen);
				info.nBkgndColor.rgbRed = (uint8_t)(255-info.nBkgndColor.rgbRed);
			} 
		} else {
			if (info.nBkgndColor.rgbBlue>level)	 info.nBkgndColor.rgbBlue = (uint8_t)(255-info.nBkgndColor.rgbBlue);
			if (info.nBkgndColor.rgbGreen>level) info.nBkgndColor.rgbGreen = (uint8_t)(255-info.nBkgndColor.rgbGreen);
			if (info.nBkgndColor.rgbRed>level)	 info.nBkgndColor.rgbRed = (uint8_t)(255-info.nBkgndColor.rgbRed);
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Converts the RGB triplets to and from different colorspace
 * \param dstColorSpace: destination colorspace; 0 = RGB, 1 = HSL, 2 = YUV, 3 = YIQ, 4 = XYZ 
 * \param srcColorSpace: source colorspace; 0 = RGB, 1 = HSL, 2 = YUV, 3 = YIQ, 4 = XYZ 
 * \return true if everything is ok
 */
bool CxImage::ConvertColorSpace(const int32_t dstColorSpace, const int32_t srcColorSpace)
{
	if (!pDib)
		return false;

	if (dstColorSpace == srcColorSpace)
		return true;

	int32_t w = GetWidth();
	int32_t h = GetHeight();

	for (int32_t y=0;y<h;y++){
		info.nProgress = (int32_t)(100*y/h);
		if (info.nEscape) break;
		for (int32_t x=0;x<w;x++){
			RGBQUAD c = BlindGetPixelColor(x,y);
			switch (srcColorSpace){
			case 0:
				break;
			case 1:
				c = HSLtoRGB(c);
				break;
			case 2:
				c = YUVtoRGB(c);
				break;
			case 3:
				c = YIQtoRGB(c);
				break;
			case 4:
				c = XYZtoRGB(c);
				break;
			default:
				strcpy(info.szLastError,"ConvertColorSpace: unknown source colorspace");
				return false;
			}
			switch (dstColorSpace){
			case 0:
				break;
			case 1:
				c = RGBtoHSL(c);
				break;
			case 2:
				c = RGBtoYUV(c);
				break;
			case 3:
				c = RGBtoYIQ(c);
				break;
			case 4:
				c = RGBtoXYZ(c);
				break;
			default:
				strcpy(info.szLastError,"ConvertColorSpace: unknown destination colorspace");
				return false;
			}
			BlindSetPixelColor(x,y,c);
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Finds the optimal (global or local) treshold for image binarization
 * \param method: 0 = average all methods (default); 1 = Otsu; 2 = Kittler & Illingworth; 3 = max entropy; 4 = potential difference;
 * \param pBox: region from where the threshold is computed; 0 = full image (default).
 * \param pContrastMask: limit the computation only in regions with contrasted (!=0) pixels; default = 0.
 * the pContrastMask image must be grayscale with same with and height of the current image,
 * can be obtained from the current image with a filter:
 *   CxImage iContrastMask(*image,true,false,false);
 *   iContrastMask.GrayScale();
 *   int32_t edge[]={-1,-1,-1,-1,8,-1,-1,-1,-1};
 *   iContrastMask.Filter(edge,3,1,0);
 *   int32_t blur[]={1,1,1,1,1,1,1,1,1};
 *   iContrastMask.Filter(blur,3,9,0);
 * \return optimal threshold; -1 = error.
 * \sa AdaptiveThreshold
 */
int32_t  CxImage::OptimalThreshold(int32_t method, RECT * pBox, CxImage* pContrastMask)
{
	if (!pDib)
		return false;

	if (head.biBitCount!=8){
		strcpy(info.szLastError,"OptimalThreshold works only on 8 bit images");
		return -1;
	}

	if (pContrastMask){
		if (!pContrastMask->IsValid() ||
			!pContrastMask->IsGrayScale() ||
			pContrastMask->GetWidth() != GetWidth() ||
			pContrastMask->GetHeight() != GetHeight()){
			strcpy(info.szLastError,"OptimalThreshold invalid ContrastMask");
			return -1;
		}
	}

	int32_t xmin,xmax,ymin,ymax;
	if (pBox){
		xmin = max(pBox->left,0);
		xmax = min(pBox->right,head.biWidth);
		ymin = max(pBox->bottom,0);
		ymax = min(pBox->top,head.biHeight);
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}
	
	if (xmin>=xmax || ymin>=ymax)
		return -1;

	double p[256];
	memset(p,  0, 256*sizeof(double));
	//build histogram
	for (int32_t y = ymin; y<ymax; y++){
		uint8_t* pGray = GetBits(y) + xmin;
		uint8_t* pContr = 0;
		if (pContrastMask) pContr = pContrastMask->GetBits(y) + xmin;
		for (int32_t x = xmin; x<xmax; x++){
			uint8_t n = *pGray++;
			if (pContr){
				if (*pContr) p[n]++;
				pContr++;
			} else {
				p[n]++;
			}
		}
	}

	//find histogram limits
	int32_t gray_min = 0;
	while (gray_min<255 && p[gray_min]==0) gray_min++;
	int32_t gray_max = 255;
	while (gray_max>0 && p[gray_max]==0) gray_max--;
	if (gray_min > gray_max)
		return -1;
	if (gray_min == gray_max){
		if (gray_min == 0)
			return 0;
		else
			return gray_max-1;
	}

	//compute total moments 0th,1st,2nd order
	int32_t i,k;
	double w_tot = 0;
	double m_tot = 0;
	double q_tot = 0;
	for (i = gray_min; i <= gray_max; i++){
		w_tot += p[i];
		m_tot += i*p[i];
		q_tot += i*i*p[i];
	}

	double L, L1max, L2max, L3max, L4max; //objective functions
	int32_t th1,th2,th3,th4; //optimal thresholds
	L1max = L2max = L3max = L4max = 0;
	th1 = th2 = th3 = th4 = -1;

	double w1, w2, m1, m2, q1, q2, s1, s2;
	w1 = m1 = q1 = 0;
	for (i = gray_min; i < gray_max; i++){
		w1 += p[i];
		w2 = w_tot - w1;
		m1 += i*p[i];
		m2 = m_tot - m1;
		q1 += i*i*p[i];
		q2 = q_tot - q1;
		s1 = q1/w1-m1*m1/w1/w1; //s1 = q1/w1-pow(m1/w1,2);
		s2 = q2/w2-m2*m2/w2/w2; //s2 = q2/w2-pow(m2/w2,2);

		//Otsu
		L = -(s1*w1 + s2*w2); //implemented as definition
		//L = w1 * w2 * (m2/w2 - m1/w1)*(m2/w2 - m1/w1); //implementation that doesn't need s1 & s2
		if (L1max < L || th1<0){
			L1max = L;
			th1 = i;
		}

		//Kittler and Illingworth
		if (s1>0 && s2>0){
			L = w1*log(w1/sqrt(s1))+w2*log(w2/sqrt(s2));
			//L = w1*log(w1*w1/s1)+w2*log(w2*w2/s2);
			if (L2max < L || th2<0){
				L2max = L;
				th2 = i;
			}
		}

		//max entropy
		L = 0;
		for (k=gray_min;k<=i;k++) if (p[k] > 0)	L -= p[k]*log(p[k]/w1)/w1;
		for (k;k<=gray_max;k++) if (p[k] > 0)	L -= p[k]*log(p[k]/w2)/w2;
		if (L3max < L || th3<0){
			L3max = L;
			th3 = i;
		}

		//potential difference (based on Electrostatic Binarization method by J. Acharya & G. Sreechakra)
		// L=-fabs(vdiff/vsum); è molto selettivo, sembra che L=-fabs(vdiff) o L=-(vsum)
		// abbiano lo stesso valore di soglia... il che semplificherebbe molto la routine
		double vdiff = 0;
		for (k=gray_min;k<=i;k++)
			vdiff += p[k]*(i-k)*(i-k);
		double vsum = vdiff;
		for (k;k<=gray_max;k++){
			double dv = p[k]*(k-i)*(k-i);
			vdiff -= dv;
			vsum += dv;
		}
		if (vsum>0) L = -fabs(vdiff/vsum); else L = 0;
		if (L4max < L || th4<0){
			L4max = L;
			th4 = i;
		}
	}

	int32_t threshold;
	switch (method){
	case 1: //Otsu
		threshold = th1;
		break;
	case 2: //Kittler and Illingworth
		threshold = th2;
		break;
	case 3: //max entropy
		threshold = th3;
		break;
	case 4: //potential difference
		threshold = th4;
		break;
	default: //auto
		{
			int32_t nt = 0;
			threshold = 0;
			if (th1>=0) { threshold += th1; nt++;}
			if (th2>=0) { threshold += th2; nt++;}
			if (th3>=0) { threshold += th3; nt++;}
			if (th4>=0) { threshold += th4; nt++;}
			if (nt)
				threshold /= nt;
			else
				threshold = (gray_min+gray_max)/2;

			/*better(?) but really expensive alternative:
			n = 0:255;
			pth1 = c1(th1)/sqrt(2*pi*s1(th1))*exp(-((n - m1(th1)).^2)/2/s1(th1)) + c2(th1)/sqrt(2*pi*s2(th1))*exp(-((n - m2(th1)).^2)/2/s2(th1));
			pth2 = c1(th2)/sqrt(2*pi*s1(th2))*exp(-((n - m1(th2)).^2)/2/s1(th2)) + c2(th2)/sqrt(2*pi*s2(th2))*exp(-((n - m2(th2)).^2)/2/s2(th2));
			...
			mse_th1 = sum((p-pth1).^2);
			mse_th2 = sum((p-pth2).^2);
			...
			select th# that gives minimum mse_th#
			*/

		}
	}

	if (threshold <= gray_min || threshold >= gray_max)
		threshold = (gray_min+gray_max)/2;
	
	return threshold;
}
///////////////////////////////////////////////////////////////////////////////
/**
 * Converts the image to B&W, using an optimal threshold mask
 * \param method: 0 = average all methods (default); 1 = Otsu; 2 = Kittler & Illingworth; 3 = max entropy; 4 = potential difference;
 * \param nBoxSize: the image is divided into "nBoxSize x nBoxSize" blocks, from where the threshold is computed; min = 8; default = 64.
 * \param pContrastMask: limit the computation only in regions with contrasted (!=0) pixels; default = 0.
 * \param nBias: global offset added to the threshold mask; default = 0.
 * \param fGlobalLocalBalance: balance between local and global threshold. default = 0.5
 * fGlobalLocalBalance can be from 0.0 (use only local threshold) to 1.0 (use only global threshold)
 * the pContrastMask image must be grayscale with same with and height of the current image,
 * \return true if everything is ok.
 * \sa OptimalThreshold
 */
bool CxImage::AdaptiveThreshold(int32_t method, int32_t nBoxSize, CxImage* pContrastMask, int32_t nBias, float fGlobalLocalBalance)
{
	if (!pDib)
		return false;

	if (pContrastMask){
		if (!pContrastMask->IsValid() ||
			!pContrastMask->IsGrayScale() ||
			pContrastMask->GetWidth() != GetWidth() ||
			pContrastMask->GetHeight() != GetHeight()){
			strcpy(info.szLastError,"AdaptiveThreshold invalid ContrastMask");
			return false;
		}
	}

	if (nBoxSize<8) nBoxSize = 8;
	if (fGlobalLocalBalance<0.0f) fGlobalLocalBalance = 0.0f;
	if (fGlobalLocalBalance>1.0f) fGlobalLocalBalance = 1.0f;

	int32_t mw = (head.biWidth + nBoxSize - 1)/nBoxSize;
	int32_t mh = (head.biHeight + nBoxSize - 1)/nBoxSize;

	CxImage mask(mw,mh,8);
	if(!mask.GrayScale())
		return false;

	if(!GrayScale())
		return false;

	int32_t globalthreshold = OptimalThreshold(method, 0, pContrastMask);
	if (globalthreshold <0)
		return false;

	for (int32_t y=0; y<mh; y++){
		for (int32_t x=0; x<mw; x++){
			info.nProgress = (int32_t)(100*(x+y*mw)/(mw*mh));
			if (info.nEscape) break;
			RECT r;
			r.left = x*nBoxSize;
			r.right = r.left + nBoxSize;
			r.bottom = y*nBoxSize;
			r.top = r.bottom + nBoxSize;
			int32_t threshold = OptimalThreshold(method, &r, pContrastMask);
			if (threshold <0) return false;
			mask.SetPixelIndex(x,y,(uint8_t)max(0,min(255,nBias+((1.0f-fGlobalLocalBalance)*threshold + fGlobalLocalBalance*globalthreshold))));
		}
	}

	mask.Resample(mw*nBoxSize,mh*nBoxSize,0);
	mask.Crop(0,head.biHeight,head.biWidth,0);

	if(!Threshold(&mask))
		return false;

	return true;
}
///////////////////////////////////////////////////////////////////////////////
/**
 * Finds the contour of an object with a given color
 * \param color_target: object color
 * \param color_trace: contour color
 * \return true if everything is ok.
 * \sa Edge, Contour
 */
bool CxImage::Trace(RGBQUAD color_target, RGBQUAD color_trace)
{
	if (!pDib) return false;

	RGBQUAD color;
	bool bFindStartPoint; 
	POINT StartPoint,CurrentPoint; 
	int32_t Direction[8][2]={{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}, {0,1},{1,1}};
	int8_t nFindPoint,nDirection;
	int32_t x,y;

	CxImage tmp;
	tmp.CopyInfo(*this);
	tmp.Create(head.biWidth,head.biHeight,24,info.dwType);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}
	tmp.Clear(255);

	CurrentPoint.x = StartPoint.x = CurrentPoint.y = StartPoint.y = 0;
	bFindStartPoint = false;
	for (y=head.biHeight-1;y>=0 && !bFindStartPoint;y--){
		info.nProgress = (int32_t)(100*y/head.biHeight);
		if (info.nEscape) break;
		for (x=0;x<head.biWidth && !bFindStartPoint;x++){
			color = BlindGetPixelColor(x,y);
			if (color.rgbRed   == color_target.rgbRed   &&
				color.rgbGreen == color_target.rgbGreen &&
				color.rgbBlue  == color_target.rgbBlue  )
			{
				bFindStartPoint = true;
				CurrentPoint.x = StartPoint.x = x;
				CurrentPoint.y = StartPoint.y = y; 
			}
		}
	}

	nDirection = nFindPoint = 0;
	while(bFindStartPoint && (nFindPoint<9))
	{
		x = CurrentPoint.x + Direction[nDirection][0];
		y = CurrentPoint.y + Direction[nDirection][1];
		color = GetPixelColor(x,y);
		if (IsInside(x,y) &&
			color.rgbRed   == color_target.rgbRed   &&
			color.rgbGreen == color_target.rgbGreen && 
			color.rgbBlue  == color_target.rgbBlue  ) 
		{
			CurrentPoint.x = x;
			CurrentPoint.y = y;

			nFindPoint = 0;
			if(x == StartPoint.x && y == StartPoint.y)
				bFindStartPoint = false;

			tmp.BlindSetPixelColor(x,y,color_trace);

			nDirection -= 2;
			if(nDirection == -2)
				nDirection = 6;
			if(nDirection == -1)
				nDirection = 7;
		}
		else
		{
			nDirection++;
			if(nDirection == 8)
				nDirection = 0;
			nFindPoint++;
		}
	}

	Transfer(tmp);
	return true;
}

#ifndef __MINGW32__ 
////////////////////////////////////////////////////////////////////////////////
#include <queue>
////////////////////////////////////////////////////////////////////////////////
/**
 * Flood Fill
 * \param xStart, yStart: starting point
 * \param cFillColor: filling color
 * \param nTolerance: deviation from the starting point color
 * \param nOpacity: can be from 0 (transparent) to 255 (opaque, default)
 * \param bSelectFilledArea: if true, the pixels in the region are also set in the selection layer; default = false
 * \param nSelectionLevel: if bSelectFilledArea is true, the selected pixels are set to nSelectionLevel; default = 255
 * Note: nOpacity=0 && bSelectFilledArea=true act as a "magic wand"
 * \return true if everything is ok
 */
bool CxImage::FloodFill(const int32_t xStart, const int32_t yStart, const RGBQUAD cFillColor, const uint8_t nTolerance,
						uint8_t nOpacity, const bool bSelectFilledArea, const uint8_t nSelectionLevel)
{
	if (!pDib)
		return false;

	if (!IsInside(xStart,yStart))
		return true;

#if CXIMAGE_SUPPORT_SELECTION
	if (!SelectionIsInside(xStart,yStart))
		return true;
#endif //CXIMAGE_SUPPORT_SELECTION

	RGBQUAD* pPalette=NULL;
	uint16_t bpp = GetBpp();
	//nTolerance or nOpacity implemented only for grayscale or 24bpp images
	if ((nTolerance || nOpacity != 255) &&	!(head.biBitCount == 24 || IsGrayScale())){
		pPalette = new RGBQUAD[head.biClrUsed];
		memcpy(pPalette, GetPalette(),GetPaletteSize());
		if (!IncreaseBpp(24))
			return false;
	}

	uint8_t* pFillMask = (uint8_t*)calloc(head.biWidth * head.biHeight,1);
	if (!pFillMask)
		return false;

//------------------------------------- Begin of Flood Fill
	POINT offset[4] = {{-1,0},{0,-1},{1,0},{0,1}};
	std::queue<POINT> q;
	POINT point = {xStart,yStart};
	q.push(point);

	if (IsIndexed()){ //--- Generic indexed image, no tolerance OR Grayscale image with tolerance
		uint8_t idxRef = GetPixelIndex(xStart,yStart);
		uint8_t idxFill = GetNearestIndex(cFillColor);
		uint8_t idxMin = (uint8_t)min(255, max(0,(int32_t)(idxRef - nTolerance)));
		uint8_t idxMax = (uint8_t)min(255, max(0,(int32_t)(idxRef + nTolerance)));

		while(!q.empty())
		{
			point = q.front();
			q.pop();

			for (int32_t z=0; z<4; z++){
				int32_t x = point.x + offset[z].x;
				int32_t y = point.y + offset[z].y;
				if(IsInside(x,y)){
#if CXIMAGE_SUPPORT_SELECTION
				  if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				  {
					uint8_t idx = BlindGetPixelIndex(x, y);
					uint8_t* pFill = pFillMask + x + y * head.biWidth;
					if (*pFill==0 && idxMin <= idx && idx <= idxMax )
					{
						if (nOpacity>0){
							if (nOpacity == 255)
								BlindSetPixelIndex(x, y, idxFill);
							else
								BlindSetPixelIndex(x, y, (uint8_t)((idxFill * nOpacity + idx * (255-nOpacity))>>8));
						}
						POINT pt = {x,y};
						q.push(pt);
						*pFill = 1;
					}
				  }
				}
			}
		}
	} else { //--- RGB image
		RGBQUAD cRef = GetPixelColor(xStart,yStart);
		RGBQUAD cRefMin, cRefMax;
		cRefMin.rgbRed   = (uint8_t)min(255, max(0,(int32_t)(cRef.rgbRed   - nTolerance)));
		cRefMin.rgbGreen = (uint8_t)min(255, max(0,(int32_t)(cRef.rgbGreen - nTolerance)));
		cRefMin.rgbBlue  = (uint8_t)min(255, max(0,(int32_t)(cRef.rgbBlue  - nTolerance)));
		cRefMax.rgbRed   = (uint8_t)min(255, max(0,(int32_t)(cRef.rgbRed   + nTolerance)));
		cRefMax.rgbGreen = (uint8_t)min(255, max(0,(int32_t)(cRef.rgbGreen + nTolerance)));
		cRefMax.rgbBlue  = (uint8_t)min(255, max(0,(int32_t)(cRef.rgbBlue  + nTolerance)));

		while(!q.empty())
		{
			point = q.front();
			q.pop();

			for (int32_t z=0; z<4; z++){
				int32_t x = point.x + offset[z].x;
				int32_t y = point.y + offset[z].y;
				if(IsInside(x,y)){
#if CXIMAGE_SUPPORT_SELECTION
				  if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				  {
					RGBQUAD cc = BlindGetPixelColor(x, y);
					uint8_t* pFill = pFillMask + x + y * head.biWidth;
					if (*pFill==0 &&
						cRefMin.rgbRed   <= cc.rgbRed   && cc.rgbRed   <= cRefMax.rgbRed   &&
						cRefMin.rgbGreen <= cc.rgbGreen && cc.rgbGreen <= cRefMax.rgbGreen &&
						cRefMin.rgbBlue  <= cc.rgbBlue  && cc.rgbBlue  <= cRefMax.rgbBlue )
					{
						if (nOpacity>0){
							if (nOpacity == 255)
								BlindSetPixelColor(x, y, cFillColor);
							else
							{
								cc.rgbRed   = (uint8_t)((cFillColor.rgbRed   * nOpacity + cc.rgbRed   * (255-nOpacity))>>8);
								cc.rgbGreen = (uint8_t)((cFillColor.rgbGreen * nOpacity + cc.rgbGreen * (255-nOpacity))>>8);
								cc.rgbBlue  = (uint8_t)((cFillColor.rgbBlue  * nOpacity + cc.rgbBlue  * (255-nOpacity))>>8);
								BlindSetPixelColor(x, y, cc);
							}
						}
						POINT pt = {x,y};
						q.push(pt);
						*pFill = 1;
					}
				  }
				}
			}
		}
	}
	if (pFillMask[xStart+yStart*head.biWidth] == 0 && nOpacity>0){
		if (nOpacity == 255)
			BlindSetPixelColor(xStart, yStart, cFillColor);
		else
		{
			RGBQUAD cc = BlindGetPixelColor(xStart, yStart);
			cc.rgbRed   = (uint8_t)((cFillColor.rgbRed   * nOpacity + cc.rgbRed   * (255-nOpacity))>>8);
			cc.rgbGreen = (uint8_t)((cFillColor.rgbGreen * nOpacity + cc.rgbGreen * (255-nOpacity))>>8);
			cc.rgbBlue  = (uint8_t)((cFillColor.rgbBlue  * nOpacity + cc.rgbBlue  * (255-nOpacity))>>8);
			BlindSetPixelColor(xStart, yStart, cc);
		}
	}
	pFillMask[xStart+yStart*head.biWidth] = 1;
//------------------------------------- End of Flood Fill

	//if necessary, restore the original BPP and palette
	if (pPalette){
		DecreaseBpp(bpp, false, pPalette);
		delete [] pPalette;
	}

#if CXIMAGE_SUPPORT_SELECTION
	if (bSelectFilledArea){
		if (!SelectionIsValid()){
			if (!SelectionCreate()){
				return false;
			}
			SelectionClear();
			info.rSelectionBox.right = head.biWidth;
			info.rSelectionBox.top = head.biHeight;
			info.rSelectionBox.left = info.rSelectionBox.bottom = 0;
		}
		RECT r;
		SelectionGetBox(r);
		for (int32_t y = r.bottom; y < r.top; y++){
			uint8_t* pFill = pFillMask + r.left + y * head.biWidth;
			for (int32_t x = r.left; x<r.right; x++){
				if (*pFill)	SelectionSet(x,y,nSelectionLevel);
				pFill++;
			}
		}
		SelectionRebuildBox();
	}
#endif //CXIMAGE_SUPPORT_SELECTION

	free(pFillMask);

	return true;
}
#endif //__MINGW32__

////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DSP
