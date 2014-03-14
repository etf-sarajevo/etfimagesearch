#ifndef HSL10BIN_H
#define HSL10BIN_H

#include "colorhistogram.h"


/**
 * Fuzzy color histogram with 10 bins in HSL color model
 */

class HSL10bin : public ColorHistogram
{
public:
	HSL10bin();
	
	// Some params (not used actually I think)
	void setParams(double blackThreshold, double whiteThreshold, double grayThreshold, int hueQuant);
	
	QString name() { return QString("HSL Fuzzy Histogram"); }
	static QString static_name() { return QString("HSL Fuzzy Histogram"); }
	int size() { return 10; }
	
	virtual void colorQuantize(Pixel &p);
	virtual void incrementHistogram(const Pixel& p);
	
private:
	double blackThreshold, whiteThreshold, grayThreshold;
	int hueQuant;
};

#endif // HSL10BIN_H
