#include "yuvhistogram.h"

#include <QDebug>
#include <QColor>
#include <cmath>

YUVHistogram::YUVHistogram(int Ybits = 4, int Ubits = 2, int Vbits = 2) : SearchAlgorithm(), Ybits(Ybits), Ubits(Ubits), Vbits(Vbits)
{
	result.features.resize( pow (2,Ybits+Ubits+Vbits) );
}

int toByte(int x) {
	if (x<0) x=0;
	if (x>255) x=255;
	return x;
}

FeatureVector YUVHistogram::extractFeatures(const uchar *imageData, int size)
{
	// Calculate histogram
	for (int i(0); i<size; i+=3) {
		// Translate RGB to interval [0,1]
/*		double R=double(imageData[i])/256, G=double(imageData[i+1])/256, B=double(imageData[i+2])/256;
		
		// YUV formula
		double Y, U, V;
		Y = 0.299*R + 0.587*G + 0.114*B;
		U = 0.492*(B-Y);
		V = 0.877*(R-Y);
		
		// Ranges are now Ye[0,1], Ue[-0.436,0.436], Ve[-0.615,0.615]
		Y *= 256; if (Y==256) Y=255;
		U = (U+0.436)*256 / 0.872; if (U<0) U=0; if (U>255) U=255;
		V = (V+0.615)*256 / 1.230; if (V<0) V=0; if (V>255) V=255;*/
		
		int R = imageData[i], G = imageData[i+1], B = imageData[i+2];
		
		int Y = toByte ( 0.299*R + 0.587 * G + 0.114 * B );
		int U = toByte ( 128 - 0.168736*R - 0.331264*G + 0.5*B );
		int V = toByte ( 128 + 0.5*R - 0.418688*G - 0.081312*B );
		
		uint index = (int(Y) >> (8-Ybits)) << (Ubits+Vbits);
		index += (int(U) >> (8-Ubits)) << Vbits;
		index += (int(V) >> (8-Vbits));
		
		result.features[index]++;
	}
	
	// Renormalize feature vector to 1 byte per feature
	qreal max(0);
	for (int i(1); i<result.features.size(); i++) {
//		result.features[i] += result.features[i-1];
		if (result.features[i] > max)
			max = result.features[i];
	}
	for (int i(0); i<result.features.size(); i++) {
		qreal x = qreal(result.features[i]) / max;
		result.features[i] = x*256;
	}
	
	return result;
}

double YUVHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	// Euclidean distance
	int sum(0);
	for (int i(0); i<f1.features.size(); i++) {
		sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
	}
	return sqrt(qreal(sum));
}

