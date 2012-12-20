#ifndef RGBHISTOGRAM_H
#define RGBHISTOGRAM_H

#include "searchalgorithm.h"

class RGBHistogram : public SearchAlgorithm
{
public:
	RGBHistogram(int Rbits, int Gbits, int Bbits);
	
	QString name() { return QString("rgbhistogram"); }
	FeatureVector extractFeatures(const uchar* imageData, int size);
	double distance(FeatureVector f1, FeatureVector f2);	
	
private:
	FeatureVector result;
	int Rbits, Gbits, Bbits;
};

#endif // RGBHISTOGRAM_H
