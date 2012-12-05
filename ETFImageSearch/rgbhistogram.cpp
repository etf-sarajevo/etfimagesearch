#include "rgbhistogram.h"

#include <QDebug>
#include <cmath>

RGBHistogram::RGBHistogram(int bins = 512) : SearchAlgorithm(), bins(bins)
{
	result.features.resize(bins);
}

FeatureVector RGBHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate divisor for color values
	int divisorR(0), divisorG(0), divisorB(0);
	
	// Simple case
	for (int i(1); i<=8; i++) {
		if (bins == pow(2,3*i))
			divisorR = divisorG = divisorB = 8-i;
	}
	
	// A more complex case
	if (divisorR == 0)
		for (int i(1); i<=24; i++)
			if (bins == pow(2,i)) {
				divisorR = divisorG = divisorB = 8-i/3;
				if (i%3 == 1)
					divisorG--;
				if (i%3 == 2) {
					divisorR--;
					divisorG--;
				}
			}
	
	int powerR = (8 - divisorG) + (8 - divisorB);
	int powerG = 8 - divisorB;

	for (int i(0); i<size; i+=3) {
		uint index = (imageData[i] >> divisorR) << powerR;
		index += (imageData[i+1] >> divisorG) << powerG;
		index += (imageData[i+2] >> divisorB);
		
		result.features[index]++;
	}
	
	// Renormalize feature vector to 1 byte per feature
	qreal max(0);
	for (int i(0); i<bins; i++)
		if (result.features[i] > max)
			max = result.features[i];
	for (int i(0); i<bins; i++) {
		qreal x = qreal(result.features[i]) / max;
		result.features[i] = x*256;
	}
	
	return result;
}

qreal RGBHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	// Euclidean distance
	int sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));
}

