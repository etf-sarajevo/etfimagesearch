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
	double X,Y,Z; // used in several conversions
	
	if (colorModel == YUV) {
		// YUV formula
		/*double Y, U, V;
		Y = 0.299*R + 0.587*G + 0.114*B;
		U = 0.492*(B-Y);
		V = 0.877*(R-Y);
		
		// Ranges are now Ye[0,1], Ue[-0.436,0.436], Ve[-0.615,0.615]
		
		// Convert to byte:
		Y = toByte123(Y*256);
		U = toByte123((U+0.436)*256 / (0.436*2));
		V = toByte123((V+0.615)*256 / (0.615*2));*/
		
		// The following is equivalent and comes from JPEG standard
		int Y = toByte123 ( 0.299 * R + 0.587 * G + 0.114 * B );
		int U = toByte123 ( 128 - 0.168736 * R - 0.331264 * G + 0.5 * B );
		int V = toByte123 ( 128 + 0.5 * R - 0.418688 * G - 0.081312 * B );
		
		pixel[0] = Y; pixel[1] = U; pixel[2] = V;
		return;
	}
	
	if (colorModel == YIQ) {
		int Y = toByte123 ( 0.299 * R + 0.587 * G + 0.114 * B );
		int I = toByte123 ( 0.596 * R - 0.275 * G - 0.321 * B );
		int Q = toByte123 ( 0.212 * R - 0.523 * G + 0.311 * B );
		
		pixel[0] = Y; pixel[1] = I; pixel[2] = Q;
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

	if (colorModel == XYZ || colorModel == LAB || colorModel == LUV) {
		// Convert RGB to range [0,1]
		double r(double(R)/255);
		double g(double(G)/255);
		double b(double(B)/255);
		
		// We assume that color is sRGB with decoding gamma of 2.2, as customary
		
		// Linearize sRGB
		if (r<0.04045) r=r/12.92; else r=pow((r+0.055)/1.055, 2.4);
		if (g<0.04045) g=g/12.92; else g=pow((g+0.055)/1.055, 2.4);
		if (b<0.04045) b=b/12.92; else b=pow((b+0.055)/1.055, 2.4);
		
		// Convert sRGB to XYZ using transformation matrix
		X = 0.4124564 * R + 0.3575761 * G + 0.1804375 * B;
		Y = 0.2126729 * R + 0.7151522 * G + 0.0721750 * B;
		Z = 0.0193339 * R + 0.1191920 * G + 0.9503041 * B;
	}

	if (colorModel == XYZ) {
		pixel[0] = toByte123(X*255);
		pixel[1] = toByte123(Y*255);
		pixel[2] = toByte123(Z*255);
		return;
	}
	
	if (colorModel == LAB) {
		// Reference illuminant D65 white point
		double Xn = 0.95047;
		double Yn = 1;
		double Zn = 1.08883;
		
		double D = pow( 6.0 / 29.0, 3 );
		
		double Xfn( X / Xn ), Yfn( Y / Yn ), Zfn( Z / Zn );
		if ( Xfn > D ) Xfn = pow(Xfn,3); else Xfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Xfn + ( 4.0 / 29.0 );
		if ( Yfn > D ) Yfn = pow(Yfn,3); else Yfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Yfn + ( 4.0 / 29.0 );
		if ( Zfn > D ) Zfn = pow(Zfn,3); else Zfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Zfn + ( 4.0 / 29.0 );
		
		double L = 116 * Yfn - 16;
		double a = 500 * (Xfn - Yfn);
		double b = 200 * (Yfn - Zfn);
		
		// Convert to byte
		pixel[0] = toByte123( L*255 / 100 );
		pixel[1] = toByte123( a + 127 );
		pixel[2] = toByte123( b + 127 );
		return;
	}
	
	if (colorModel == LUV) {
		// Reference illuminant D65 white point
		double Yn = 1;
		// Reference illuminant chromaticity coordinates (from Wikipedia)
		double un = 0.2009; double vn = 0.4610;
		
		double D = pow( 6.0 / 29.0, 3 );
		
		double Yfn( Y / Yn );
		if ( Yfn > D ) Yfn = pow(Yfn,3); else Yfn = ( 1.0 / 3.0 ) * pow( (29.0 / 6.0), 2) * Yfn + ( 4.0 / 29.0 );

		double L = 116 * Yfn - 16;
		double u = 13 * L * ( (4*X) / (X + 15*Y + 3*Z) - un);
		double v = 13 * L * ( (9*Y) / (X + 15*Y + 3*Z) - vn);
		
		// Convert to byte
		pixel[0] = toByte123( L*255 / 100 );
		pixel[1] = toByte123( u + 127 );
		pixel[2] = toByte123( v + 127 );
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
		if (histogramCumulative) {
			for (int i = 1; i < result.features.size(); i++) {
				result.features[i] += result.features[i-1];
			}
		}
		
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
		double sum(0);
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
		double sum(0);
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

