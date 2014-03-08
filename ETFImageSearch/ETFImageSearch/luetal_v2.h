#ifndef LUETAL_V2_H
#define LUETAL_V2_H

#include "imagefeatures.h"

#define MAX_COMPONENTS 6

class LuEtAl_v2 : public DCTFeatures
{
public:
	LuEtAl_v2();
	
	QString name() { return QString("Lu et al. v2"); }
	static QString static_name() { return QString("Lu et al. v2"); }
	
	int size() { return 804; }
	DataType dataType() { return ImageFeatures::FLOAT32; }
	
	void init();
	void processBlock(short int* block, int component);
	FeatureVector calculateVector();
	
	double distance(FeatureVector f1, FeatureVector f2);
	
	void setkMEkD(double kME, double kD) { this->kME=kME; this->kD=kD; }
	
private:
	// Stats
	double colorFeatures[MAX_COMPONENTS][4];
	int colorFeaturesLast1000[MAX_COMPONENTS][4];
	int colorFeaturesCounters[MAX_COMPONENTS];
	int colorFeaturesDoubleCounters[MAX_COMPONENTS];
	
	int totalMaxM, totalMinM;
	int maxM, minM;
	
	int colorHistogram[MAX_COMPONENTS][256];
	int colorHistogramCounter;
	
	int bigHistogram[512];
	int previousComponent[MAX_COMPONENTS][4];
	
	double specificBlocks[MAX_COMPONENTS][6];
	int specificBlocksLast1000[MAX_COMPONENTS][6];
	double specificBlocksSquares[MAX_COMPONENTS][6];
	int specificBlocksSquaresLast1000[MAX_COMPONENTS][6];
	
	double kME, kD;
};

#endif // LUETAL_V2_H
