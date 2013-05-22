#include "colorhistogram.h"

#include <QDebug>
#include <QColor>
#include <cmath>

ColorHistogram::ColorHistogram() : SearchAlgorithm(),
	colorModel(RGB), componentCount(3), cqScheme(BINARY), histogramType(COMBINEDHISTOGRAM)
{
	colorQuantization[0]=colorQuantization[1]=colorQuantization[2]=3;
	colorQuantization[3]=0;
}

int toByte123(int x) 
{
	if (x<0) x=0;
	if (x>255) x=255;
	return x;
}

void ColorHistogram::convertColorModel(int* pixel) 
{
	if (colorModel == RGB) return; // Nothing to do
	
	int R = pixel[0], G = pixel[1], B = pixel[2];
	
	if (colorModel == YUV) {
		int Y = toByte123 ( 0.299 * R + 0.587 * G + 0.114 * B );
		int U = toByte123 ( 128 - 0.168736 * R - 0.331264 * G + 0.5 * B );
		int V = toByte123 ( 128 + 0.5 * R - 0.418688 * G - 0.081312 * B );
		
		pixel[0] = Y; pixel[1] = U; pixel[2] = V;
		return;
	}
	
	if (colorModel == HSV) {
		int H, S, V;
		
		QColor color(R, G, B);
		color.getHsv(&H, &S, &V);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		pixel[0] = H; pixel[1] = S; pixel[2] = V;
		
		return;
	}
	
	if (colorModel == HSL) {
		int H, S, L;
		
		QColor color(R, G, B);
		color.getHsl(&H, &S, &L);
		
		H = (H*256)/360; // Rescale to 0-255
		if (H==256) H=255;
		
		pixel[0] = H; pixel[1] = S; pixel[2] = L;
		
		return;
	}
}

void ColorHistogram::setColorQuantization(int cq0, int cq1, int cq2, int cq3) 
{
	double dcq0 = int((log(double(cq0)) / log(2)) * 1000) / 1000.0;
	double dcq1 = int((log(double(cq1)) / log(2)) * 1000) / 1000.0;
	double dcq2 = int((log(double(cq2)) / log(2)) * 1000) / 1000.0;
	double dcq3 = int((log(double(cq3)) / log(2)) * 1000) / 1000.0;
	
	if (dcq0==int(dcq0) && dcq1==int(dcq1) && dcq2==int(dcq2) && dcq3==int(dcq3)) {
		cqScheme=BINARY;
		colorQuantization[0] = dcq0; colorQuantization[1] = dcq1; colorQuantization[2] = dcq2; colorQuantization[3] = dcq3;
	} else {
		cqScheme=NORMAL;
		colorQuantization[0] = cq0; colorQuantization[1] = cq1; colorQuantization[2] = cq2; colorQuantization[3] = cq3;
	}
}

void ColorHistogram::colorQuantize(int* pixel) 
{
	for (int i(0); i<componentCount; i++)
		if (colorQuantization[i]==0) pixel[i]=0;
		else {
			if (cqScheme==BINARY)
				pixel[i] = pixel[i] >> (8 - colorQuantization[i]);
			else if (cqScheme == NORMAL) {
				pixel[i] = (pixel[i] * colorQuantization[i]) / 256;
				if (pixel[i] >= colorQuantization[i]) pixel[i]=colorQuantization[i]-1;
			}
		}
}

void ColorHistogram::incrementHistogram(int *pixel)
{
	if (histogramType == COMBINEDHISTOGRAM) {
		uint index=0;
		for (int i(0); i<componentCount; i++) {
			if (colorQuantization[i] == 0) continue;
			
			index += pixel[i];
			
			if (i < componentCount-1) {
				if (cqScheme == BINARY)
					index = index << colorQuantization[i+1];
				else if (cqScheme == NORMAL)
					index = index * colorQuantization[i+1];
			}
		}
		result.features[index]++;
		
	} else if (histogramType == SPLITHISTOGRAM) {
		uint offset=0;
		for (int i(0); i<componentCount; i++) {
			if (colorQuantization[i] == 0) continue;
			
			result.features[offset + pixel[i]]++;
			
			if (i < componentCount-1) {
				if (cqScheme == BINARY)
					offset += pow(2, colorQuantization[i]);
				else if (cqScheme == NORMAL)
					offset += colorQuantization[i];
			}
		}
	}
}


void ColorHistogram::histogramNormalizeQuantize(int imageSize)
{
	// Quantization factor
	int factor = pow(2, histogramQuantization);
	
	if (histogramNormalization == NO_NORMALIZATION) {
		// Cumulative histogram
		if (histogramCumulative)
			for (int i = 1; i < result.features.size(); i++)
				result.features[i] += result.features[i-1];
		
		// Just quantize, no normalization
		for (int i = 0; i < result.features.size(); i++)
			result.features[i] = int ( (result.features[i] / imageSize ) * factor );
	}
	
	else if (histogramNormalization == MAX_NORMALIZATION) {
		// Cumulative histogram and find maximum
		qreal max(0);
		for (int i(0); i<result.features.size(); i++) {
			if (histogramCumulative && i>0)
				result.features[i] += result.features[i-1];
			if (result.features[i] > max)
				max = result.features[i];
		}
		
		// Normalize and quantize
		for (int i(0); i<result.features.size(); i++) {
			qreal x = qreal(result.features[i]) / max;
			result.features[i] = x*factor;
		}
	}
	else if (histogramNormalization == BOTH_NORMALIZATION) {
		// Cumulative histogram, find maximum and minimum
		qreal max(0), min(result.features[0]);
		for (int i(0); i<result.features.size(); i++) {
			if (histogramCumulative && i>0)
				result.features[i] += result.features[i-1];
			if (result.features[i] > max)
				max = result.features[i];
			if (result.features[i] < min)
				min = result.features[i];
		}
		
		// Normalize and quantize
		for (int i(0); i<result.features.size(); i++) {
			qreal x = qreal(result.features[i] - min) / (max - min);
			result.features[i] = x*factor;
		}
	}
}

void ColorHistogram::resizeFeatureVector()
{
	int bins(0);
	if (histogramType == COMBINEDHISTOGRAM) {
		bins=1;
		for (int i(0); i<componentCount; i++) {
			if (colorQuantization[i] == 0) continue;
			if (cqScheme == BINARY)
				bins *= pow(2, colorQuantization[i]);
			else
				bins *= colorQuantization[i];
		}
		
	} else if (histogramType == SPLITHISTOGRAM) {
		for (int i(0); i<componentCount; i++)
			if (cqScheme == BINARY)
				bins += pow(2, colorQuantization[i]);
			else
				bins += colorQuantization[i];
	}
	
	result.features.resize( bins );
}

FeatureVector ColorHistogram::extractFeatures(const uchar *imageData, int size)
{
	resizeFeatureVector();
	
	int pixel[4]; // Storage for one pixel
	
	// Calculate histogram
	for (int i(0); i<size; i+=4) {
		pixel[0] = imageData[i+2]; // RED
		pixel[1] = imageData[i+1]; // GREEN
		pixel[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		convertColorModel(pixel);
		colorQuantize(pixel);
		
		incrementHistogram(pixel);
	}
	
	histogramNormalizeQuantize(size/4);
	
	return result;
}



double ColorHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	switch(distanceMetric) {

	case EUCLIDEAN:
	{
		int sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum += (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
		}
		return sqrt(qreal(sum));
	}
	
	case MATSUSHITA:
	{
		double sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			double k = sqrt(f2.features[i]) - sqrt(f1.features[i]);
			sum += k * k;
		}
		return sqrt(sum);
	}
	
	case BRAY_CURTIS:
	{
		// Bray-Curtis distance
		double sum1(0), sum2(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum1 += double(abs(f2.features[i] - f1.features[i]));
			sum2 += (f2.features[i] + f1.features[i]);
		}
		return sum1 / sum2;
	}
	
	case MANHATTAN:
	{
		// Manhattan distance
		int sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum += abs(f2.features[i] - f1.features[i]);
		}
		return double(sum) / f1.features.size();
	}
	
	case SOERGEL:
	{
		// Soergel distance
		double sum1(0), sum2(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum1 += double(abs(f2.features[i] - f1.features[i]));
			sum2 += qMax(f2.features[i], f1.features[i]);
		}
		return sum1 / sum2;
	}
	
	case BHATTACHARYA:
	{
		double sum(0), sum1(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum += sqrt(fabs(f2.features[i] - f1.features[i]));
			sum1 += f1.features[i];
		}
		return log10(sum/sum1);
	}
	
	case CHI_SQUARE:
	{
		double sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum += ((f2.features[i] - f1.features[i])*(f2.features[i] - f1.features[i])) / (f2.features[i] + f1.features[i]);
		}
		return sum;
	}
	
	case CANBERRA:
	{
		double sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum += double(fabs(f2.features[i] - f1.features[i])) / (f2.features[i] + f1.features[i]);
		}
		return sum;
	}
	
	case HIST_INT:
	{
		int sum(0), sum1(0), sum2(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum  += qMin(f2.features[i], f1.features[i]);
			sum1 += f1.features[i];
			sum2 += f2.features[i];
		}
		return 1 - double(sum) / qMin (sum1, sum2);
	}
	
	case JSD:
	{
		double sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			double mi((f2.features[i] + f1.features[i]) / 2);
			if (f1.features[i] > 0)
				sum += f1.features[i] * log(double(f1.features[i]) / mi);
			if (f2.features[i] > 0)
				sum += f2.features[i] * log(double(f2.features[i]) / mi);
		}
		return sum;
	}
		
	case ANGULAR:
	{
		double sum1(0), sum2(0), sum3(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum1 += f2.features[i] * f1.features[i];
			sum2 += f2.features[i] * f2.features[i];
			sum3 += f1.features[i] * f1.features[i];
		}
		return 1 - (sum1 / sqrt(sum2 * sum3) );
	}
		
	case CHORD:
	{
		double sum1(0), sum2(0), sum3(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum1 += f2.features[i] * f1.features[i];
			sum2 += f2.features[i] * f2.features[i];
			sum3 += f1.features[i] * f1.features[i];
		}
		return sqrt(2 - 2*(sum1 / sqrt(sum2 * sum3)));
	}
		
	case WAVE_HEDGES:
	{
		double sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum += (1 - double(qMin(f2.features[i], f1.features[i])) / qMax(f2.features[i], f1.features[i]));
		}
		return sum;
	}
		
	case WED:
	{
		int sum(0);
		for (int i(0); i<f1.features.size(); i++) {
			double k(f1.features[i]);
			if (k==0) k=1;
			sum += k * (f2.features[i] - f1.features[i]) * (f2.features[i] - f1.features[i]);
		}
		return sum;
	}
		
	case K_S: // Kolmogorov-Smirnov
	{
		int max(0);
		for (int i(0); i<f1.features.size(); i++) {
			int d(abs(f2.features[i] - f1.features[i]));
			if (d>max) max=d;
		}
		return max;
	}
		
	case KUIPER:
	{
		double max1(0), max2(0);
		for (int i(0); i<f1.features.size(); i++) {
			double d1(f2.features[i] - f1.features[i]);
			double d2(f1.features[i] - f2.features[i]);
			if (d1>max1) max1=d1;
			if (d2>max2) max2=d2;
		}
		return max1+max2;
	}
	
	case MEAN:
	{
		double sum1(0), sum2(0);
		for (int i(0); i<f1.features.size(); i++) {
			sum1 += f1.features[i];
			sum2 += f2.features[i];
		}
		return fabs(sum1 / f1.features.size() - sum2 / f2.features.size());
	}
	}
	
	return 0; // No distance metric found
}

