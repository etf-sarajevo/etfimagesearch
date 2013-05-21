#include "hsvhistogram.h"

#include <QDebug>
#include <QColor>
#include <cmath>

HSVHistogram::HSVHistogram(int Hbits=3, int Sbits=3, int Vbits=3) : SearchAlgorithm(), 
	Hbits(Hbits), Sbits(Sbits), Vbits(Vbits)
{
	result.features.resize( pow(2, Hbits+Sbits+Vbits) );
//	result.features.resize( 162 );
//	result.features.resize( 192 );
}

int toByte2(int x) {
	if (x<0) x=0;
	if (x>255) x=255;
	return x;
}

FeatureVector HSVHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate histogram
	for (int i(0); i<size; i+=4) {
		int H, S, V;
		
		QColor color(imageData[i+2], imageData[i+1], imageData[i]);
		//color.getHsv(&H, &S, &V);
		color.getHsl(&H, &S, &V);

		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		//int R = imageData[i+2], G = imageData[i+1], B = imageData[i];
		//V = toByte2	 ( 0.299*R + 0.587 * G + 0.114 * B );
		
		uint index = (H >> (8 - Hbits)) << (Sbits + Vbits);
		index += (S >> (8 - Sbits)) << Vbits;
		index += (V >> (8 - Vbits));
		
		/*// Zhang et al.
		int Hq;
		if (H<=25) Hq=0;
		else if (H<=45) Hq=1;
		else if (H<=75) Hq=2;
		else if (H<=95) Hq=3;
		else if (H<=145) Hq=4;
		else if (H<=165) Hq=5;
		else if (H<=195) Hq=6;
		else if (H<=215) Hq=7;
		else if (H<=265) Hq=8;
		else if (H<=285) Hq=9;
		else if (H<=315) Hq=10;
		else if (H<=335) Hq=11;
		else Hq=0;
		uint index = Hq * 16 + (S >> 6) * 4 + (V >> 6);*/
		
		/*// Ngo et al.
		H = (H*18) / 360; if (H>=18) H=17;
		S = (S*3) / 256;
		V = (V*3) / 256;
		uint index = V + S * 3 + H * 9;*/
		
		result.features[index]++;
	}
	
	// Renormalize feature vector to 1 byte per feature
	qreal max(0);
	for (int i(0); i<result.features.size(); i++) {
//		result.features[i] += result.features[i-1];
		if (result.features[i] > max)
			max = result.features[i];
	}
	for (int i(0); i<result.features.size(); i++) {
		qreal x = qreal(result.features[i]) / max;
		result.features[i] = int(x+0.5);
	}
	
	/*for (int i = 0; i < result.features.size(); i++) {
		result.features[i] = int ( (result.features[i] / (size/4)) * 65536 );
	}*/
	return result;
}

double HSVHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	/*// Euclidean distance
	int sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));*/
	
	// Matshita distance
	double sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		double k = sqrt(f2.features[i]) - sqrt(f1.features[i]);
		sum += k * k;
	}
	return sqrt(sum);
	
	/*// Bray-Curtis distance
	double sum1(0), sum2(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum1 += double(abs(f2.features[i] - f1.features[i]));
		sum2 += (f2.features[i] + f1.features[i]);
	}
	return sum1 / sum2;*/
	
	/*// Manhattan distance
	int sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum += abs(f2.features[i] - f1.features[i]);
	}
	return sum;*/
	
	/*// Soergel distance
	double sum1(0), sum2(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum1 += double(abs(f2.features[i] - f1.features[i]));
		sum2 += qMax(f2.features[i], f1.features[i]);
	}
	return sum1 / sum2;*/
}

