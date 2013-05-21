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
	for (int i(0); i<size; i+=4) {
		int R = imageData[i+2], G = imageData[i+1], B = imageData[i];
		
		uint index = (R >> (8 - Rbits) ) << (Gbits + Bbits);
		index += (G >> (8 - Gbits)) << Bbits;
		index += (B >> (8 - Bbits));
		
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

	/*qreal max(0), min(result.features[0]);
	for (int i(0); i<result.features.size(); i++) {
//		result.features[i] += result.features[i-1];
		if (result.features[i] > max)
			max = result.features[i];
		if (result.features[i] < min)
			min = result.features[i];
	}
	for (int i(0); i<result.features.size(); i++) {
		qreal x = qreal(result.features[i] - min) / (max - min);
		result.features[i] = x*256;
	}*/
	
	/*for (int i = 0; i < result.features.size(); i++) {
		result.features[i] = int ( (result.features[i] / (size/4)) * 256 );
	}*/
	
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
	
	/*// Matshita distance
	double sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		double k = sqrt(f2.features[i]) - sqrt(f1.features[i]);
		sum += k * k;
	}
	return sqrt(sum);*/
	
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
	return double(sum) / f1.features.size();*/
}

