#ifndef SEARCHALGORITHM_H
#define SEARCHALGORITHM_H

#include <QVector>
#include <QString>

// Libraries from the hacked version of libjpeg
extern "C" {
//	#include "jpeglib.h"
}



class FeatureVector {
public:
	// Here further metadata can be added
	QVector<double> features;
};

class SearchAlgorithm
{
public:
	SearchAlgorithm();
	
	virtual QString name()=0; // should be a short string of lowercase letters and numerals (used in filename)
	
	// Get feature vector from an array containing image data
	virtual FeatureVector extractFeatures(const uchar* imageData, int size)=0;
	
	virtual bool isDct() { return false; }
	
	// Calculate distance between two feature vectors
	virtual double distance(FeatureVector f1, FeatureVector f2)=0;
	
	bool debug;
};

class DCTSearchAlgorithm : public SearchAlgorithm
{
public:
	DCTSearchAlgorithm();
	
	virtual QString name()=0; // should be a short string of lowercase letters and numerals (used in filename)
	bool isDct() { return true; }
	
	// Call this before processing each image
	virtual void init()=0;
	// This will be called once for each 8x8 DCT block for given component
	virtual void processBlock(short int* block, int component)=0; 
	// Call this after whole image is processed to get the final feature vector
	virtual FeatureVector calculateVector()=0; 
	
	static void processBlockWrapper(void* object, short int* block, int component) {
		DCTSearchAlgorithm* myself = (DCTSearchAlgorithm*) object;
		myself->processBlock(block, component);
	}
	
	virtual double distance(FeatureVector f1, FeatureVector f2)=0;
	
private:
	// Disable this method
	FeatureVector extractFeatures(const uchar* imageData, int size) {}
};

#endif // SEARCHALGORITHM_H
