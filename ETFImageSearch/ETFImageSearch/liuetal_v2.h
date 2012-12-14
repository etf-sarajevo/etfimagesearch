#ifndef LIUETAL_V2_H
#define LIUETAL_V2_H

#include "searchalgorithm.h"

#define MAX_COMPONENTS 6

class LiuEtAl_v2 : public DCTSearchAlgorithm
{
public:
	LiuEtAl_v2();
	
	QString name() { return QString("liuetal_v2"); }
	
	void init();
	void processBlock(short int* block, int component);
	FeatureVector calculateVector();
	
	qreal distance(FeatureVector f1, FeatureVector f2);
	
	void setkMEkD(double kME, double kD) { this->kME=kME; this->kD=kD; }
	
private:
	// Stats
	double colorFeatures[MAX_COMPONENTS][4];
	int colorFeaturesLast1000[MAX_COMPONENTS][4];
	int colorFeaturesCounters[MAX_COMPONENTS];
	int colorFeaturesDoubleCounters[MAX_COMPONENTS];
	
	int totalMaxM, totalMinM;
	int maxM, minM;
	
	int colorHistogram[MAX_COMPONENTS][64];
	int colorHistogramCounter;
	
	int bigHistogram[512];
	int previousComponent[MAX_COMPONENTS][4];
	
	double specificBlocks[MAX_COMPONENTS][6];
	int specificBlocksLast1000[MAX_COMPONENTS][6];
	double specificBlocksSquares[MAX_COMPONENTS][6];
	int specificBlocksSquaresLast1000[MAX_COMPONENTS][6];
	
	double kME, kD;
};

#endif // LIUETAL_V2_H
