#include "zhangetal.h"

ZhangEtAl::ZhangEtAl() : ColorHistogram()
{
	setColorModel(Pixel::HSV);
	setColorQuantization(12,4,4);
}


void ZhangEtAl::colorQuantize(Pixel &p) 
{
	// Color quantization scheme 12x4x4
	// H is quantized by table, S&V uniformly
	
	int quantizationTable[12] = { 25, 45, 75, 95, 145, 165, 195, 215, 265, 285, 315, 335 };
	
	double hue = double(p.c[0]) * 360 / 256;
	int i(0);
	for (; i<12; i++)
		if (hue < quantizationTable[i]) {
			p.c[0] = i; break;
		}
	if (i==12) p.c[0] = 0; // Last bracket
	
	p.c[1] = p.c[1] >> 6;
	p.c[2] = p.c[2] >> 6;
}

