#ifndef SEARCHALGORITHM_H
#define SEARCHALGORITHM_H

#include <QVector>
#include <QString>

class FeatureVector {
public:
	// Here further metadata can be added
	QVector<int> features;
};

class SearchAlgorithm
{
public:
	SearchAlgorithm();
	
	virtual QString name()=0; // should be a short string of lowercase letters and numerals (used in filename)
	virtual FeatureVector extractFeatures(const uchar* imageData, int size)=0;
	virtual bool isDct()=0;
	virtual qreal distance(FeatureVector f1, FeatureVector f2)=0;
};

#endif // SEARCHALGORITHM_H
