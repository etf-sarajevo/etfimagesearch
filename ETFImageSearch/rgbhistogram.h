#ifndef RGBHISTOGRAM_H
#define RGBHISTOGRAM_H

#include "searchalgorithm.h"

class RGBHistogram : public SearchAlgorithm
{
public:
	RGBHistogram(int bins);
	
	QString name() { return QString("rgbhistogram"); }
	FeatureVector extractFeatures(const uchar* imageData, int size);
	bool isDct() { return false; }
	qreal distance(FeatureVector f1, FeatureVector f2);	
	
private:
	FeatureVector result;
	int bins;
};

#endif // RGBHISTOGRAM_H
