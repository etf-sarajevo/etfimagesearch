#ifndef RGBSPLITHISTOGRAM_H
#define RGBSPLITHISTOGRAM_H

#include "searchalgorithm.h"

class RGBSplitHistogram : public SearchAlgorithm
{
public:
	RGBSplitHistogram(int Rbits, int Gbits, int Bbits);
	
	QString name() { return QString("rgbhistogram"); }
	FeatureVector extractFeatures(const uchar* imageData, int size);
	double distance(FeatureVector f1, FeatureVector f2);	
	
private:
	FeatureVector result;
	int Rbits, Gbits, Bbits;
};

#endif // RGBSPLITHISTOGRAM_H
