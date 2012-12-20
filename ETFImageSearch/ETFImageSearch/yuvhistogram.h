#ifndef YUVHISTOGRAM_H
#define YUVHISTOGRAM_H

#include "searchalgorithm.h"

class YUVHistogram : public SearchAlgorithm
{
public:
	YUVHistogram(int Ybits, int Ubits, int Vbits);
	
	QString name() { return QString("yuvhistogram"); }
	FeatureVector extractFeatures(const uchar* imageData, int size);
	double distance(FeatureVector f1, FeatureVector f2);	
	
private:
	FeatureVector result;
	int Ybits, Ubits, Vbits;
};

#endif // YUVHISTOGRAM_H
