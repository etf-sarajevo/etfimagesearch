#include "colormoments.h"

#include <cmath>
#include <QDebug>

ColorMoments::ColorMoments()
{
}

/*float mean(FeatureVector array, int begin, int end)
{
	float result(0);
	for (int i(begin); i<end; i++)
		result += array[i];
	result /= end-begin;
	return result;
}

float stddev(FeatureVector array, int begin, int end, float mean)
{
	float result(0);
	for (int i(begin); i<end; i++)
		result += pow(array[i]-mean, 2);
	result /= end-begin;
	result = sqrt(result);
	return result;
}
float skewness(FeatureVector array, int begin, int end, float mean)
{
	float result(0);
	for (int i(begin); i<end; i++)
		result += fabs(pow(array[i]-mean, 3));
	result /= end-begin;
	result = pow(result, 1./3);
	return result;
}*/

FeatureVector ColorMoments::extractFeatures(const uchar* imageData, int width, int height)
{
	double mean[3] = {0,0,0}, stddev[3] = {0,0,0}, skewness[3] = {0,0,0};
	Pixel p; // Storage for one pixel
	
	// Calculate mean for all three channels
	int N(0);
	for (int i(0); i<width*height*4; i+=4) {
		p.model = Pixel::RGB;
		p.c[0] = imageData[i+2]; // RED
		p.c[1] = imageData[i+1]; // GREEN
		p.c[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		p.convertColorModel(colorModel);
		colorQuantize(p);
		
		for (int c(0); c<3; c++)
			mean[c] += p.c[c];
		N++;
	}
	for (int c(0); c<3; c++)
		mean[c] /= N;
	
	// Second pass to calculate std dev and skewness (requires mean)
	for (int i(0); i<width*height*4; i+=4) {
		p.model = Pixel::RGB;
		p.c[0] = imageData[i+2]; // RED
		p.c[1] = imageData[i+1]; // GREEN
		p.c[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		p.convertColorModel(colorModel);
		colorQuantize(p);
		
		for (int c(0); c<3; c++) {
			stddev[c]   += (p.c[c] - mean[c]) * (p.c[c] - mean[c]);
			skewness[c] += fabs( (p.c[c] - mean[c]) * (p.c[c] - mean[c]) * (p.c[c] - mean[c]) );
		}
	}
	for (int c(0); c<3; c++) {
		stddev[c]   = sqrt( stddev[c] / N );
		skewness[c] =  pow( stddev[c] / N, 1./3 );
	}
	
	// Put stats into feature vector
	FeatureVector result;
	result.resize( 3 * componentCount );
	result.fill( 0 , 3 * componentCount );
	for (int c(0); c<3; c++) {
		result[c*3]   = mean[c];
		result[c*3+1] = stddev[c];
		result[c*3+2] = skewness[c];
	}
	return result;
}

