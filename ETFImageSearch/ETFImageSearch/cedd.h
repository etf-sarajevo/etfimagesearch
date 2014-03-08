#ifndef CEDD_H
#define CEDD_H

#include "imagefeatures.h"
#include "distancemetric.h"


/**
 * CEDD features from paper:
 *  Chatzichristofis et al., "Accurate image retrieval based on compact composite descriptors and relevance feedback information", IJPRAI, Vol 24, No 2, 2010.
 * Implementation from LIRe */

class CEDD : public ImageFeatures
{
public:
	CEDD();
	
	QString name() { return QString("CEDD"); }
	static QString static_name() { return QString("CEDD"); }
	
	int size() { if (compact) return 60; else return 144; }
	DataType dataType() { return ImageFeatures::UINT8; }
	
	FeatureVector extractFeatures(const uchar* imageData, int width, int height);
	double distance(FeatureVector f1, FeatureVector f2);
	
private:
	bool compact;
	DistanceMetric dmObject;
};

#endif // CEDD_H
