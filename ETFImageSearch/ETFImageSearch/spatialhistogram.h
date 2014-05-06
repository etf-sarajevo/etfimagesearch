#ifndef BINDISTRIBUTIONHISTOGRAM_H
#define BINDISTRIBUTIONHISTOGRAM_H

#include "colorhistogram.h"

#include <QPointF>

class SpatialHistogram : public ColorHistogram
{
public:
	SpatialHistogram();
	
	QString name() { return QString("Spatial Histogram"); }
	static QString static_name() { return QString("Spatial Histogram"); }
	
	virtual FeatureVector extractFeatures(const uchar* imageData, int width, int height);
	
	int size() { return ColorHistogram::size() * numHistograms(); }
	
	virtual DataType dataType() {
		if (moments)
			return ImageFeatures::FLOAT32;
		return ColorHistogram::dataType();
	}

	enum SpatialType { GRID, ANNULAR, ANGULAR, ANNO_ANGULAR, BULLS_EYE };
	
	void setSpatialType(SpatialType t) { spatialType = t; }
	SpatialType getSpatialType() const { return spatialType; }
	
	enum AppType { IMAGE, BIN_DISTRIBUTION };
	
	void setAppType(AppType t) { appType = t; }
	AppType getAppType() const { return appType; }
	
	void useMoments(bool m) { moments = m; }
	
private:
	// Number of spatial histograms
	int numHistograms();
	
	uint getBinIndex(const Pixel &p);
	
	void findCentroids(const uchar* imageData, int width, int height);
	void findMaxDistance(const uchar* imageData, int width, int height);
	void spatialIncrement(const Pixel& p, double relX, double relY);
	
	SpatialType spatialType;
	AppType appType;
	bool moments;
	
	QVector<QPointF> centroid;
	QVector<int> binPixelCount;
	QVector<double> maxDistance;
	
};

#endif // BINDISTRIBUTIONHISTOGRAM_H
