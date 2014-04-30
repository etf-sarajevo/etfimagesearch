#include "colormoments.h"

#include <cmath>
#include <QDebug>

ColorMoments::ColorMoments()
{
}

float mean(FeatureVector array, int begin, int end)
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
}

FeatureVector ColorMoments::extractFeatures(const uchar* imageData, int width, int height)
{
	qDebug() << "eF";
	ColorHistogram ch;
	ch.setParams(getParams());
	FeatureVector histogram = ch.extractFeatures(imageData, width, height);
QString debug;
for (int i(0); i<histogram.size(); i++) debug += QString("%1,").arg(histogram[i]);
qDebug() << "HISTO: "<<debug;
	FeatureVector result;

	if (histogramType == ColorHistogram::SPLITHISTOGRAM) {
		result.resize( 3 * componentCount );
		result.fill( 0 , 3 * componentCount );
		int binstart(0), binend(0);
		for (int i(0); i<componentCount; i++) {
			binstart = binend+1;
			if (cqScheme == BINARY)
				binend += pow(2, colorQuantization[i])-1;
			else
				binend += colorQuantization[i]-1;
			
			qDebug() << binstart << binend;

			result[i*3] = mean(histogram, binstart, binend);
			result[i*3 + 1] = stddev(histogram, binstart, binend, result[i*3]);
			result[i*3 + 2] = skewness(histogram, binstart, binend, result[i*3]);
		}
	} else {
		result.resize( 3 );
		result.fill( 0 , 3 );
		result[0] = mean(histogram, 0, histogram.size());
		result[1] = stddev(histogram, 0, histogram.size(), result[0]);
		result[2] = skewness(histogram, 0, histogram.size(), result[0]);
	}
	return result;
}

