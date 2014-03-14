#include "colorhistogram.h"

#include <QDebug>
#include <QColor>
#include <cmath>

ColorHistogram::ColorHistogram() : ImageFeatures(),
	colorModel(Pixel::RGB), componentCount(3), cqScheme(BINARY), histogramType(COMBINEDHISTOGRAM), histogramQuantization(8), distanceMetric(DistanceMetric::MANHATTAN)
{
	colorQuantization[0]=colorQuantization[1]=colorQuantization[2]=3;
	colorQuantization[3]=0;
}

int ColorHistogram::size()
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
	return bins;
}

ImageFeatures::DataType ColorHistogram::dataType()
{
	if (histogramQuantization<=8)
		return ImageFeatures::UINT8;
	if (histogramQuantization<=16)
		return ImageFeatures::UINT16;
	if (histogramQuantization<32)
		return ImageFeatures::UINT32;
	if (histogramQuantization==32)
		return ImageFeatures::FLOAT32;
	return ImageFeatures::DOUBLE64;
}


void ColorHistogram::setParams(QString params)
{
	QStringList paramsList = params.split(';');

	// paramsList[0] = color model
	setColorModel(Pixel::fromString(paramsList[0]));
	
	// paramsList[1] = color quantization
	QStringList quantizations = paramsList[1].split(',');
	if (quantizations.size() == 4)
		setColorQuantization(quantizations[0].toInt(), quantizations[1].toInt(), quantizations[2].toInt(), quantizations[3].toInt());
	else if (quantizations.size() == 3)
		setColorQuantization(quantizations[0].toInt(), quantizations[1].toInt(), quantizations[2].toInt());
	else
		throw "Invalid quantization";
	
	// paramsList[2] = histogram type
	if (paramsList[2] == "COMBINED")
		setHistogramType(COMBINEDHISTOGRAM);
	else if (paramsList[2] == "SPLIT")
		setHistogramType(SPLITHISTOGRAM);
	else
		throw "Invalid histogram type";
	
	// paramsList[3] = histogram normalization
	if (paramsList[3] == "NO_NORMALIZATION")
		setHistogramNormalization(NO_NORMALIZATION);
	else if (paramsList[3] == "MAX_NORMALIZATION")
		setHistogramNormalization(MAX_NORMALIZATION);
	else if (paramsList[3] == "BOTH_NORMALIZATION")
		setHistogramNormalization(BOTH_NORMALIZATION);
	else
		throw "Invalid histogram normalization";
	
	// paramsList[4] = histogram quantization
	bool ok;
	setHistogramQuantization(paramsList[4].toInt(&ok));
	if (!ok) throw "Invalid histogram quantization";
	
	// paramsList[5] = cumulative?
	if (paramsList[5] == "CUMULATIVE")
		setHistogramCumulative(true);
	else
		setHistogramCumulative(false);
	
	// paramsList[6] = distance metric
	setDistanceMetric(DistanceMetric::fromString(paramsList[6]));
}

QString ColorHistogram::getParams()
{
	QStringList paramsList;
	
	// paramsList[0] = color model
	paramsList.append(Pixel::toString(colorModel));
	
	// paramsList[1] = color quantization
	QString quantizations;
	if (colorQuantization[3] > 0)
		quantizations = QString("%1,%2,%3,%4").arg(colorQuantization[0]).arg(colorQuantization[1]).arg(colorQuantization[2]).arg(colorQuantization[3]);
	else
		quantizations = QString("%1,%2,%3").arg(colorQuantization[0]).arg(colorQuantization[1]).arg(colorQuantization[2]);
	paramsList.append(quantizations);
	
	// paramsList[2] = histogram type
	if (histogramType == COMBINEDHISTOGRAM)
		paramsList.append("COMBINED");
	else if (histogramType == SPLITHISTOGRAM)
		paramsList.append("SPLIT");
	
	// paramsList[3] = histogram normalization
	if (histogramNormalization == NO_NORMALIZATION)
		paramsList.append("NO_NORMALIZATION");
	else if (histogramNormalization == MAX_NORMALIZATION)
		paramsList.append("MAX_NORMALIZATION");
	else if (histogramNormalization == BOTH_NORMALIZATION)
		paramsList.append("BOTH_NORMALIZATION");
	
	// paramsList[4] = histogram quantization
	paramsList.append(QString("%1").arg(histogramQuantization));
	
	// paramsList[5] = cumulative?
	if (histogramCumulative)
		paramsList.append("CUMULATIVE");
	else 
		paramsList.append("NOT_CUMULATIVE");
	
	// paramsList[6] = distance metric
	paramsList.append(DistanceMetric::toString(distanceMetric));
	
	return paramsList.join(QString(';'));
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

void ColorHistogram::colorQuantize(Pixel &p) 
{
	for (int i(0); i<componentCount; i++)
		if (colorQuantization[i]==0) p.c[i]=0;
		else {
			if (cqScheme==BINARY)
				p.c[i] = p.c[i] >> (8 - colorQuantization[i]);
			else if (cqScheme == NORMAL) {
				p.c[i] = (p.c[i] * colorQuantization[i]) / 256;
				if (p.c[i] >= colorQuantization[i]) p.c[i]=colorQuantization[i]-1;
			}
		}
}

void ColorHistogram::incrementHistogram(const Pixel &p)
{
	if (histogramType == COMBINEDHISTOGRAM) {
		uint index=0;
		for (int i(0); i<componentCount; i++) {
			if (colorQuantization[i] == 0) continue;
			
			index += p.c[i];
			
			if (i < componentCount-1) {
				if (cqScheme == BINARY)
					index = index << colorQuantization[i+1];
				else if (cqScheme == NORMAL)
					index = index * colorQuantization[i+1];
			}
		}
		result[index]++;
		
	} else if (histogramType == SPLITHISTOGRAM) {
		uint offset=0;
		for (int i(0); i<componentCount; i++) {
			if (colorQuantization[i] == 0) continue;
			
			result[offset + p.c[i]]++;
			
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
			for (int i = 1; i < result.size(); i++) {
				result[i] += result[i-1];
			}
		}
		
		// Just quantize, no normalization
		for (int i = 0; i < result.size(); i++)
			if (histogramQuantization < 32)
				result[i] = uint ( (result[i] / imageSize ) * factor );
			else
				result[i] = result[i] / imageSize;
	}
	
	else if (histogramNormalization == MAX_NORMALIZATION) {
		// Cumulative histogram and find maximum
		qreal max(0);
		for (int i(0); i<result.size(); i++) {
			if (histogramCumulative && i>0)
				result[i] += result[i-1];
			if (result[i] > max)
				max = result[i];
		}
		
		// Normalize and quantize
		for (int i(0); i<result.size(); i++) {
			qreal x = qreal(result[i]) / max;
			if (histogramQuantization < 32)
				result[i] = uint ( x * factor );
			else
				result[i] = x;
		}
	}
	else if (histogramNormalization == BOTH_NORMALIZATION) {
		// Cumulative histogram, find maximum and minimum
		qreal max(0), min(result[0]);
		for (int i(0); i<result.size(); i++) {
			if (histogramCumulative && i>0)
				result[i] += result[i-1];
			if (result[i] > max)
				max = result[i];
			if (result[i] < min)
				min = result[i];
		}
		
		// Normalize and quantize
		for (int i(0); i<result.size(); i++) {
			qreal x = qreal(result[i] - min) / (max - min);
			if (histogramQuantization < 32)
				result[i] = uint ( x * factor );
			else
				result[i] = x;
		}
	}
}

void ColorHistogram::resizeFeatureVector()
{
	result.resize( size() );
	result.fill( 0 , size() );
}

FeatureVector ColorHistogram::extractFeatures(const uchar *imageData, int width, int height)
{
	resizeFeatureVector();
	
	Pixel p; // Storage for one pixel
	
	// Calculate histogram
	for (int i(0); i<width*height*4; i+=4) {
		p.model = Pixel::RGB;
		p.c[0] = imageData[i+2]; // RED
		p.c[1] = imageData[i+1]; // GREEN
		p.c[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		p.convertColorModel(colorModel);
		colorQuantize(p);
		
		incrementHistogram(p);
	}
	
	/*QString output;
	for (int i = 0; i < result.size(); i++) {
		output += QString("%1 ").arg(result[i]);
	}
	qDebug()<<"Image histogram:"<<output;*/
	
	histogramNormalizeQuantize(width*height);
	
	/*output = "";
	for (int i = 0; i < result.size(); i++) {
		output += QString("%1 ").arg(result[i]);
	}
	qDebug()<<"After normalization:"<<output;*/
	
	return result;
}


// Prepare distance calculation parameters for quadratic distance
// Currently works only for combined histogram in RGB space
// For other color spaces need to implement conversion to Lab
// TODO rename this method to perceptualDistance and cleanup
void ColorHistogram::setQuadratic()
{
	if (histogramType != COMBINEDHISTOGRAM) return;
	if (colorModel != Pixel::RGB) return;
	if (distanceMetric != DistanceMetric::QUADRATIC) return;
	
	QVector<double> result;
	Pixel pixel; // Storage for one pixel
	Pixel newpixel;
	int delta[3];
	QVector<Pixel> labPixels;
	
	// Convert quantizations to bin deltas and set pixel to center of first bin
	for (int i(0); i<3; i++) {
		if (cqScheme == BINARY)
			delta[i] = 256 / pow(2,colorQuantization[i]);
		else
			delta[i] = 256 / colorQuantization[i];
		pixel.c[i] = delta[i]/2;
	}
	
	// Convert center of each bin to CIELab and put into vector
	for (int i(0); i<colorQuantization[0]; i++) {
		for (int j(0); j<colorQuantization[1]; j++) {
			for (int k(0); k<colorQuantization[2]; k++) {
				// Copy pixel to newpixel so we dont lose old values
				for (int l(0); l<3; l++)
					newpixel.c[l] = pixel.c[l];
				
				newpixel.convertColorModel(Pixel::LAB);
				
				// Add newpixel to labPixels vector
				Pixel tmp;
				for (int l(0); l<3; l++)
					tmp.c[l] = newpixel.c[l];
				labPixels.append(tmp);
				
				pixel.c[2] += delta[2];
			}
			pixel.c[2] = delta[2]/2;
			pixel.c[1] += delta[1];
		}
		pixel.c[1] = delta[1]/2;
		pixel.c[0] += delta[0];
	}
	
	// Find L2 distance from each Lab pixel to each other Lab pixel
	double maxDist(0);
	for (int i(0); i<labPixels.size(); i++) {
		for (int j(0); j<labPixels.size(); j++) {
			double dist(0);
			for (int k(0); k<3; k++)
				dist += (labPixels[i].c[k] - labPixels[j].c[k])*(labPixels[i].c[k] - labPixels[j].c[k]);
			result.append(sqrt(dist));
			
			// Find maximum distance
			if (dist > maxDist)
				maxDist = dist;
		}
	}
	
	// Normalize result vector and convert to similarities
	for (int i(0); i<result.size(); i++) {
		result[i] = 1 - (result[i]/maxDist);
	}
	
	dmObject.parameterVector = result;
}

double ColorHistogram::distance(FeatureVector f1, FeatureVector f2)
{
	return dmObject.distance(distanceMetric, f1, f2);
}

