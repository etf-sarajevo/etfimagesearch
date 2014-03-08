#include "hmmdquant.h"

HMMDquant::HMMDquant() : ColorHistogram()
{
	setColorModel(Pixel::HMMD);
	// We set some pointless quantization just so that it adds up to 184
	setColorQuantization(23,8,0,0); // Only two components participate in histogram
}

void HMMDquant::colorQuantize(Pixel &p) 
{
	// Quantization will be performed while incrementing histogram
}

void HMMDquant::incrementHistogram(const Pixel &p) 
{
	// First we quantize last component (diff) to 5 levels to form 5 subspaces
	// For each subspace we use levels from table to quantize hue and sum of M+M
	
	int D = (p.c[3] * 5) >> 8;
	int hue = p.c[0];
	int sum = (p.c[1]+p.c[2]) >> 1;
	
	int idx(0);
	switch (D) {
	case 0:
		sum = sum >> 5;
		idx = sum;
		break;
	case 1:
		hue = hue >> 5;
		sum = sum >> 6;
		idx =  8 + hue*4 + sum;
		break;
	case 2:
		hue = (hue * 12) >> 8;
		sum = sum >> 6;
		idx = 40 + hue*4 + sum;
		break;
	case 3:
		hue = (hue * 12) >> 8;
		sum = sum >> 6;
		idx = 88 + hue*4 + sum;
		break;
	case 4:
		hue = (hue * 24) >> 8;
		sum = sum >> 7;
		idx = 136 + hue*2 + sum;
	}
	
	result[idx]++;
}
