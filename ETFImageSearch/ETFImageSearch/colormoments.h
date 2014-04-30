#ifndef COLORMOMENTS_H
#define COLORMOMENTS_H

#include "colorhistogram.h"

class ColorMoments : public ColorHistogram
{
public:
	ColorMoments();
	
	virtual QString name() { return QString("Color Moments"); }
	static QString static_name() { return QString("Color Moments"); }
	
	virtual int size() {
			if (histogramType == ColorHistogram::SPLITHISTOGRAM)
					return 3 * componentCount;
			else
					return 3;
	}
	virtual DataType dataType() {
			return ImageFeatures::FLOAT32;
	}
	virtual FeatureVector extractFeatures(const uchar* imageData, int width, int height);
};

#endif // COLORMOMENTS_H
