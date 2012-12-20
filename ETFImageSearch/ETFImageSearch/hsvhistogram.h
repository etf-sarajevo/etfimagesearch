#ifndef HSVHISTOGRAM_H
#define HSVHISTOGRAM_H

#include "searchalgorithm.h"

class HSVHistogram : public SearchAlgorithm
{
public:
	HSVHistogram(int Hbits, int Sbits, int Vbits);
	
	QString name() { return QString("hsvhistogram"); }
	FeatureVector extractFeatures(const uchar* imageData, int size);
	double distance(FeatureVector f1, FeatureVector f2);	
	
private:
	FeatureVector result;
	int Hbits, Sbits, Vbits;
};


#endif // HSVHISTOGRAM_H
