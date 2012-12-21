#include "rgbhistogram.h"

#include <QDebug>
#include <cmath>

RGBHistogram::RGBHistogram(int Rbits=3, int Gbits=3, int Bbits=3) : SearchAlgorithm(), 
	Rbits(Rbits), Gbits(Gbits), Bbits(Bbits)
{
	result.features.resize( pow(2, Rbits+Gbits+Bbits) );
}

FeatureVector RGBHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate histogram
	for (int i(0); i<size; i+=3) {
		uint index = (imageData[i] >> (8 - Rbits) ) << (Gbits + Bbits);
		index += (imageData[i+1] >> (8 - Gbits)) << Bbits;
		index += (imageData[i+2] >> (8 - Bbits));
		
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
		result.features[i] = x*256;
	}
	
	return result;
}

double RGBHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	// Euclidean distance
	int sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));
}

