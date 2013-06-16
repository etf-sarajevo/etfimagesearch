#ifndef RGBHISTOGRAM_H
#define RGBHISTOGRAM_H

#include "searchalgorithm.h"

class ColorHistogram : public SearchAlgorithm
{
public:
	enum ColorModel { RGB, YUV, HSV, HSL, YIQ, XYZ, LAB, LUV };
	enum ColorQuantizationScheme { BINARY, NORMAL, CUSTOM };
	enum HistogramType { COMBINEDHISTOGRAM, SPLITHISTOGRAM };
	enum HistogramNormalization { NO_NORMALIZATION, MAX_NORMALIZATION, BOTH_NORMALIZATION };
	enum DistanceMetric { EUCLIDEAN, MATSUSHITA, BRAY_CURTIS, MANHATTAN, SOERGEL, BHATTACHARYA, CHI_SQUARE, CANBERRA, HIST_INT, JSD, ANGULAR, CHORD, WAVE_HEDGES, WED, K_S, KUIPER, MEAN };
	
private:
	
	FeatureVector result;
	
	ColorModel colorModel;
	int componentCount;
	
	int colorQuantization[4];
	ColorQuantizationScheme cqScheme;
	
	HistogramType histogramType;
	
	HistogramNormalization histogramNormalization;
	int histogramQuantization;
	bool histogramCumulative;
	
	DistanceMetric distanceMetric;

public:
	ColorHistogram();
	
	QString name() { return QString("colorhistogram"); }
	FeatureVector extractFeatures(const uchar* imageData, int size);
	double distance(FeatureVector f1, FeatureVector f2);
	
	void setColorModel(ColorModel cm) { colorModel=cm; componentCount=3; }
	virtual void convertColorModel(int* pixel);
	
	void setColorQuantization(int cq0, int cq1, int cq2, int cq3=0);
	virtual void colorQuantize(int *pixel);
	
	virtual void incrementHistogram(int *pixel);

	void setHistogramType(HistogramType ht) { histogramType=ht; }
	
	void setHistogramNormalization(HistogramNormalization hn) { histogramNormalization=hn; }
	void setHistogramQuantization(int hq) { histogramQuantization=hq; }
	void setHistogramCumulative(bool hc) { histogramCumulative=hc; }
	
	virtual void histogramNormalizeQuantize(int imageSize);
	
	void setDistanceMetric(DistanceMetric dm) { distanceMetric=dm; }

	void resizeFeatureVector();
};

#endif // COLORHISTOGRAM_H
