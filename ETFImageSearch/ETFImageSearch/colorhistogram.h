#ifndef RGBHISTOGRAM_H
#define RGBHISTOGRAM_H

#include "imagefeatures.h"
#include "distancemetric.h"
#include "pixel.h"

/**
 * Universal "Color Histogram" feature. Configure histogram with below easy methods.
 */

class ColorHistogram : public ImageFeatures
{
public:
	/**
	 * Approach to specifying the way color is quantized before calculating histogram (see setColorQuantization method).
	 */
	enum ColorQuantizationScheme { BINARY /** params are number of bits */, 
								   NORMAL /** params are range to quantize (doesnt work) */, 
								   CUSTOM /** custom quantization scheme (currently some schemes are hardcoded) */ 
								 };
	
	/**
	 * The way color components are represented in histogram
	 */
	enum HistogramType { COMBINEDHISTOGRAM /** a.k.a linked histogram, colors are concatenated into single index */, 
						 SPLITHISTOGRAM /** each component has separate histogram and then these histograms are concatenated */
					   };
	
	/**
	 * Is histogram normalized?
	 */
	enum HistogramNormalization { NO_NORMALIZATION /** Histogram is just rescaled given histogram quantization */, 
								  MAX_NORMALIZATION /** All values are scaled such that maximum is 1 */, 
								  BOTH_NORMALIZATION /** All values are scaled such that maximum is 1 and minimum is 0 */
								};
	
private:
	
	Pixel::ColorModel colorModel;
	int componentCount;
	
	int colorQuantization[4];
	ColorQuantizationScheme cqScheme;
	
	HistogramType histogramType;
	
	HistogramNormalization histogramNormalization;
	int histogramQuantization;
	bool histogramCumulative;
	
	DistanceMetric::Type distanceMetric;
	DistanceMetric dmObject;

public:
	ColorHistogram();
	
	virtual QString name() { return QString("Color Histogram"); }
	static QString static_name() { return QString("Color Histogram"); }
	
	// ImageFeatures methods
	FeatureVector extractFeatures(const uchar* imageData, int width, int height);
	double distance(FeatureVector f1, FeatureVector f2);
	virtual int size();
	DataType dataType();
	
	void setParams(QString params);
	QString getParams();
	
	/**
	 * Set current color model (see ColorModel enum).
	 */
	void setColorModel(Pixel::ColorModel cm) { 
		colorModel=cm; componentCount=3; 
		// Models with componentCount != 3
		if (cm == Pixel::HMMD) componentCount=4;
		if (cm == Pixel::HY) componentCount=2;
	}
	
	/**
	 * Set quantization of color components before calculating histogram using given color quantization scheme (see ColorQuantizationScheme enum).
	 */
	void setColorQuantization(int cq0, int cq1, int cq2, int cq3=0);
	
	/**
	 * Set histogram type (see HistogramType enum).
	 */
	void setHistogramType(HistogramType ht) { histogramType=ht; }
	
	/**
	 * Set histogram normalization approach (see HistogramNormalization enum).
	 */
	void setHistogramNormalization(HistogramNormalization hn) { histogramNormalization=hn; }
	
	/**
	 * Set histogram quantization (number of bits per bin).
	 * For values >= 32 no quantization will be performed and values will be stored as doubles in interval [0,1] and 64 bits per bin will be used.
	 * @param hq Number of bits per bin
	 */
	void setHistogramQuantization(int hq) { histogramQuantization=hq; }
	
	/**
	 * Get current histogram quantization.
	 */
	int getHistogramQuantization() { return histogramQuantization; }
	
	/**
	 * Is histogram cumulative?
	 */
	void setHistogramCumulative(bool hc) { histogramCumulative=hc; }
	
	/**
	 * Specify which distance metric is to be used.
	 */
	void setDistanceMetric(DistanceMetric::Type dm) { 
		DistanceMetric::Type oldDm = distanceMetric;
		distanceMetric=dm; 
		if (dm == DistanceMetric::QUADRATIC && dm != oldDm) setQuadratic();
	}
	
protected:
	/**
	 * All data is placed into this feature vector.
	 */
	FeatureVector result;
	
	/**
	 * Quantize one pixel given color quantization (see above).
	 * @param pixel Pointer to array of 4 ints representing Red, Green, Blue and Alpha respectively (Alpha is ignored).
	 */
	virtual void colorQuantize(Pixel& p);
	
	/**
	 * Increment all relevant histogram bins for pixel.
	 * @param pixel Pointer to array of 4 ints representing Red, Green, Blue and Alpha respectively (Alpha is ignored).
	 */
	virtual void incrementHistogram(const Pixel& p);
	
	/**
	 * Normalize and quantize current histogram given current settings.
	 * @param imageSize Number of pixels in image.
	 */
	virtual void histogramNormalizeQuantize(int imageSize);
	
	void resizeFeatureVector();
	
	void setQuadratic();
};

#endif // COLORHISTOGRAM_H
