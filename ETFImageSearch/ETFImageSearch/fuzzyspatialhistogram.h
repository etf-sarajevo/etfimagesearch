#ifndef FUZZYSPATIALHISTOGRAM_H
#define FUZZYSPATIALHISTOGRAM_H

#include "fuzzyhistogram.h"

class FuzzySpatialHistogram : public FuzzyHistogram
{
public:
	FuzzySpatialHistogram();
	
	QString name() { return QString("Fuzzy Spatial Histogram"); }
	static QString static_name() { return QString("Fuzzy Spatial Histogram"); }
	virtual FeatureVector extractFeatures(const uchar* imageData, int width, int height);
	
	int size() { return FuzzyHistogram::size() * numHistograms(); }

	enum FSHType { GRID, ANNULAR, ANGULAR, ANNO_ANGULAR, BULLS_EYE };
	
	void setType(FSHType t) { type = t; }
	FSHType getType() const { return type; }
	
	void setFuzzy(bool f) { fuzzy = f; }
	bool getFuzzy() const { return fuzzy; }
	
private:
	// Number of spatial histograms
	int numHistograms();
	
	// Custom function to increment fuzzy-spatial color histogram
	void incrementFSH(const Pixel& p, int width, int height, int x, int y);

	// 
	void precalculateFuzzyBoundsTable(int width, int height);
	
	
	FSHType type;
	bool fuzzy;
	QVector<QVector<double> > participation;
};

#endif // FUZZYSPATIALHISTOGRAM_H
