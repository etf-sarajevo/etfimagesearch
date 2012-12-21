#include "rgbsplithistogram.h"

#include <QDebug>
#include <cmath>

RGBSplitHistogram::RGBSplitHistogram(int Rbits=3, int Gbits=3, int Bbits=3) : SearchAlgorithm(), 
	Rbits(Rbits), Gbits(Gbits), Bbits(Bbits)
{
	result.features.resize( pow(2, Rbits)+pow(2,Gbits)+pow(2,Bbits) );
}

FeatureVector RGBSplitHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate histogram
	for (int i(0); i<size; i+=3) {
/*		result.features[imageData[i] >> (8 - Rbits)]++;
		result.features[(imageData[i+1] >> (8 - Gbits)) + pow(2,Rbits)]++;
		result.features[(imageData[i+2] >> (8 - Bbits)) + pow(2,Rbits) + pow(2,Gbits)]++;*/

		double R=double(imageData[i])/256, G=double(imageData[i+1])/256, B=double(imageData[i+2])/256;
		
		// YUV formula
		double Y, U, V;
		Y = 0.299*R + 0.587*G + 0.114*B;
		U = 0.492*(B-Y);
		V = 0.877*(R-Y);
		
		// Ranges are now Ye[0,1], Ue[-0.436,0.436], Ve[-0.615,0.615]
		Y *= 256; if (Y==256) Y=255;
		U = (U+0.436)*256 / 0.872; if (U<0) U=0; if (U>255) U=255;
		V = (V+0.615)*256 / 1.230; if (V<0) V=0; if (V>255) V=255;
		
		result.features[int(Y) >> (8 - Rbits)]++;
		result.features[(int(U) >> (8 - Gbits)) + pow(2,Rbits)]++;
		result.features[(int(V) >> (8 - Bbits)) + pow(2,Rbits) + pow(2,Gbits)]++;
	}
	
	// Renormalize feature vector to 1 byte per feature
	qreal max(0);
	for (int i(0); i<result.features.size(); i++)
		if (result.features[i] > max)
			max = result.features[i];
	for (int i(0); i<result.features.size(); i++) {
		qreal x = qreal(result.features[i]) / max;
		result.features[i] = x*256;
	}
	
	return result;
}

double RGBSplitHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	// Euclidean distance
	int sum(0);
/*	double result(0);
	for (int i(0); i<pow(2, Rbits); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	result += sqrt(qreal(sum));
	sum = 0;
	for (int i(pow(2, Rbits)); i<pow(2, Rbits)+pow(2,Gbits); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	result += sqrt(qreal(sum));
	sum = 0;
	for (int i(pow(2, Rbits)+pow(2, Gbits)); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	result += sqrt(qreal(sum));
	
	return result;*/
	
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));
	
}

