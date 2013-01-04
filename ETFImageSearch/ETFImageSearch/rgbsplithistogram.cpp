#include "rgbsplithistogram.h"

#include <QDebug>
#include <cmath>
#include <QColor>

RGBSplitHistogram::RGBSplitHistogram(int Rbits=3, int Gbits=3, int Bbits=3) : SearchAlgorithm(), 
	Rbits(Rbits), Gbits(Gbits), Bbits(Bbits)
{
	result.features.resize( pow(2, Rbits)+pow(2,Gbits)+pow(2,Bbits) );
}

int toByte3(int x) {
	if (x<0) x=0;
	if (x>255) x=255;
	return x;
}

FeatureVector RGBSplitHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate histogram
	for (int i(0); i<size; i+=4) {
		int R = imageData[i+2], G = imageData[i+1], B = imageData[i];
		
		result.features[int(R) >> (8 - Rbits)]++;
		result.features[(int(G) >> (8 - Gbits)) + pow(2,Rbits)]++;
		result.features[(int(B) >> (8 - Bbits)) + pow(2,Rbits) + pow(2,Gbits)]++;

/*		int Y = toByte3 ( 0.299*R + 0.587 * G + 0.114 * B );
		int U = toByte3 ( 128 - 0.168736*R - 0.331264*G + 0.5*B );
		int V = toByte3 ( 128 + 0.5*R - 0.418688*G - 0.081312*B );
		
		result.features[int(Y) >> (8 - Rbits)]++;
		result.features[(int(U) >> (8 - Gbits)) + pow(2,Rbits)]++;
		result.features[(int(V) >> (8 - Bbits)) + pow(2,Rbits) + pow(2,Gbits)]++;*/
		
/*		int H, S, V;
		QColor color(imageData[i+2], imageData[i+1], imageData[i]);
		//color.getHsv(&H, &S, &V);
		color.getHsl(&H, &S, &V);
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;

		V = toByte3	 ( 0.299*R + 0.587 * G + 0.114 * B );
		
		result.features[int(H) >> (8 - Rbits)]++;
		result.features[(int(S) >> (8 - Gbits)) + pow(2,Rbits)]++;
		result.features[(int(V) >> (8 - Bbits)) + pow(2,Rbits) + pow(2,Gbits)]++;*/
	}
	
	// Renormalize feature vector to 1 byte per feature
	qreal max(0);
	for (int i(1); i<result.features.size(); i++) {
		result.features[i] += result.features[i-1];
		if (result.features[i] > max)
			max = result.features[i];
	}
	for (int i(0); i<result.features.size(); i++) {
		qreal x = qreal(result.features[i]) / max;
		result.features[i] = x*256;
	}
	
	return result;
}

double RGBSplitHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	// Euclidean distance
	int sum(0);
/*	double result(0);
	for (int i(0); i<pow(2, Rbits); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	result += sqrt(qreal(sum));
	sum = 0;
	for (int i(pow(2, Rbits)); i<pow(2, Rbits)+pow(2,Gbits); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	result += sqrt(qreal(sum));
	sum = 0;
	for (int i(pow(2, Rbits)+pow(2, Gbits)); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	result += sqrt(qreal(sum));
	
	return result;*/
	
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));
	
}

