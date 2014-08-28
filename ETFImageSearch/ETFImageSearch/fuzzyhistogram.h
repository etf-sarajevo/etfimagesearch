#ifndef FUZZYHISTOGRAM_H
#define FUZZYHISTOGRAM_H

#include "colorhistogram.h"


/**
 * Fuzzy color histogram with 10 bins in HSL color model
 */

class FuzzyHistogram : public ColorHistogram
{
public:
	FuzzyHistogram();
	
	QString name() { return QString("Fuzzy Histogram"); }
	static QString static_name() { return QString("Fuzzy Histogram"); }
	int size() { return numberOfBins; }
	
	virtual void colorQuantize(Pixel &p);
	virtual void incrementHistogram(const Pixel& p);
	virtual FeatureVector extractFeatures(const uchar* imageData, int width, int height) {
		return ColorHistogram::extractFeatures(imageData, width, height);
	}
	
	void setParams(QString params);
	QString getParams();
	
	// Some variable values are invalid e.g. lower boundaries can't be greater than higher
	void setVariable(QString name, double value);

private:
	Pixel::ColorModel blackModel, whiteModel, grayModel, hueModel;
	int numberOfBins;
	int fuzzyHueBoundsCount;
	double fuzzyHueBounds[1000];
	void convertModels(double& hue, double& black, double& white, double& gray, const Pixel& p);
};

#endif // HSL10BIN_H
