#include "hsvhistogram.h"

#include <QDebug>
#include <QColor>
#include <cmath>

HSVHistogram::HSVHistogram(int Hbits=3, int Sbits=3, int Vbits=3) : SearchAlgorithm(), 
	Hbits(Hbits), Sbits(Sbits), Vbits(Vbits)
{
	result.features.resize( pow(2, Hbits+Sbits+Vbits) );
}

FeatureVector HSVHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate histogram
	for (int i(0); i<size; i+=3) {
		int H, S, V;
		QColor color(imageData[i], imageData[i+1], imageData[i+2]);
		//color.getHsv(&H, &S, &V);
		color.getHsl(&H, &S, &V);
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		uint index = (H >> (8 - Hbits)) << (Sbits + Vbits);
		index += (S >> (8 - Sbits)) << Vbits;
		index += (V >> (8 - Vbits));
		
		result.features[index]++;
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

double HSVHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	// Euclidean distance
	int sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));
}

