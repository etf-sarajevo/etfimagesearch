#include "spatialhistogram.h"

#include <QDebug>

#include <cmath>

SpatialHistogram::SpatialHistogram() : spatialType(ANNULAR), appType(BIN_DISTRIBUTION),
	moments(false)
{
	// GRID type
	addVariable(Variable("rows", 3, 1, 10, 1)); // variableValues[0]
	addVariable(Variable("cols", 3, 1, 10, 1)); // variableValues[1]
	// ANNULAR type
	addVariable(Variable("circles", 16, 1, 10, 1)); // variableValues[2]
	// ANGULAR type
	addVariable(Variable("angles", 3, 1, 10, 1)); // variableValues[3]
	// BULLS_EYE type
	addVariable(Variable("centralRegion", 0.2, 0, 1, 0.1)); // variableValues[4]
}


int SpatialHistogram::numHistograms() {
	if (spatialType==GRID) return variableValues[0] * variableValues[1];
	if (spatialType==ANNULAR) return variableValues[2];
	if (spatialType==ANGULAR) return variableValues[3];
	if (spatialType==ANNO_ANGULAR) return variableValues[2]*variableValues[3];
	if (spatialType==BULLS_EYE) return 1+variableValues[3];
}

// Determine index of a color histogram bin that pixel would belong to
uint SpatialHistogram::getBinIndex(const Pixel &p)
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
		return index;
		
	} else if (histogramType == SPLITHISTOGRAM) {
		// wtfbbq
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


// Find centroid for each bin. Also updates binPixelCount
void SpatialHistogram::findCentroids(const uchar* imageData, int width, int height)
{
	centroid.resize(ColorHistogram::size());
	binPixelCount.resize(ColorHistogram::size());
	
	Pixel p;
	
	for (int i(0); i<width*height*4; i+=4) {
		p.model = Pixel::RGB;
		p.c[0] = imageData[i+2]; // RED
		p.c[1] = imageData[i+1]; // GREEN
		p.c[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		int x = (i/4) % width;
		int y = (i/4) / width;
		
		p.convertColorModel(colorModel);
		colorQuantize(p);
		
		int idx = getBinIndex(p);
		
		// Centroid is average of x and y coordinate
		centroid[idx].setX( centroid[idx].x() + x );
		centroid[idx].setY( centroid[idx].y() + y );
		binPixelCount[idx]++;
	}
	
	for (int i(0); i<ColorHistogram::size(); i++)
		if (binPixelCount[i] > 0) {
			centroid[i].setX( centroid[i].x() / binPixelCount[i] );
			centroid[i].setY( centroid[i].y() / binPixelCount[i] );
		}
			
}


// Find maximum distance for each bin (distance from centroid to most distant
// point in bin)
void SpatialHistogram::findMaxDistance(const uchar* imageData, int width, int height)
{
	maxDistance.resize(ColorHistogram::size());
	
	Pixel p;
	
	for (int i(0); i<width*height*4; i+=4) {
		p.model = Pixel::RGB;
		p.c[0] = imageData[i+2]; // RED
		p.c[1] = imageData[i+1]; // GREEN
		p.c[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		int x = (i/4) % width;
		int y = (i/4) / width;
		
		p.convertColorModel(colorModel);
		colorQuantize(p);
		
		int idx = getBinIndex(p);
		
		double d = ( x-centroid[idx].x() ) * ( x-centroid[idx].x() );
		d += ( y-centroid[idx].y() ) * ( y-centroid[idx].y() );
		d = sqrt(d);
		if (d > maxDistance[idx]) maxDistance[idx] = d;
	}
}


// Increment feature vector given relative coordinates relX and relY
void SpatialHistogram::spatialIncrement(const Pixel& p, double relX, double relY)
{
	int idx = getBinIndex(p);
	int offset(0);
	
	// Distance from coordinate axes scaled to range [-1,1]
	double d  = sqrt(relX*relX+relY*relY);
	
	// Angle from coordinate start scaled to range [0,1]
	double angle;
	if (spatialType == ANGULAR || spatialType == ANNO_ANGULAR) {
		double pi = 4*atan(1);
		angle = asin(relX/d) / pi; // Range is now [-0.5,0.5]
		
		if (relY<0) angle = (angle + 0.5) / 2;
		else angle = (angle + 1.5) / 2;
		if (d==0) angle=0;
	}
	
	d = d / sqrt(2);
	
	// Increment by histogram type
	if (spatialType == ANNULAR) {
		offset = d*variableValues[2];
		if (offset >= variableValues[2]) 
			offset = variableValues[2] - 1;
	}
	else if (spatialType == ANGULAR) {
		offset = angle*variableValues[3];
		if (offset >= variableValues[3]) 
			offset = variableValues[3] - 1;
	}
	else if (spatialType == ANNO_ANGULAR) {
		int offset1 = d*variableValues[2];
		if (offset1 >= variableValues[2]) 
			offset1 = variableValues[2] - 1;
		int offset2 = angle*variableValues[3];
		if (offset2 >= variableValues[3]) 
			offset2 = variableValues[3] - 1;

		offset = offset2*variableValues[2] + offset1;
	}
	else if (spatialType == BULLS_EYE) {
		if (d <= variableValues[12])
			offset = 0;
		else
			offset = 1+angle*variableValues[11];
		if (offset > variableValues[11]) offset = variableValues[11];
	}
	
	offset *= ColorHistogram::size();
	result[offset+idx]++;
}


void SpatialHistogram::spatialNormalizeQuantize()
{
	int factor = pow(2, histogramQuantization);
	
	// Just quantize, no normalization
	for (int i = 0; i < numHistograms(); i++) {
		double sum = 0;
		for (int j = 0; j < ColorHistogram::size(); j++)
			sum += result[i * ColorHistogram::size() + j];

		for (int j = 0; j < ColorHistogram::size(); j++) {
			int idx = i * ColorHistogram::size() + j;
			if (histogramQuantization < 32)
				result[idx] = uint ( (result[idx] / sum ) * factor );
			else
				result[idx] = result[idx] / sum;
		}
	}
}


FeatureVector SpatialHistogram::extractFeatures(const uchar *imageData, int width, int height)
{
	if (appType == BIN_DISTRIBUTION) {
		findCentroids  (imageData, width, height);
		findMaxDistance(imageData, width, height);
	}
	
	// Pass3: increment bins
	
	resizeFeatureVector();
	
	Pixel p;
	
	for (int i(0); i<width*height*4; i+=4) {
		p.model = Pixel::RGB;
		p.c[0] = imageData[i+2]; // RED
		p.c[1] = imageData[i+1]; // GREEN
		p.c[2] = imageData[i]; // BLUE
		// imageData[i+3] is Alpha or unused
		
		int x = (i/4) % width;
		int y = (i/4) / width;
		
		p.convertColorModel(colorModel);
		colorQuantize(p);
		
		// Relative coordinates scaled to range [-1,1]
		double relX, relY;
		if (appType == BIN_DISTRIBUTION) {
			// Relative to centroid
			int idx = getBinIndex(p);
			relX = (x - centroid[idx].x()) / maxDistance[idx];
			relY = (y - centroid[idx].y()) / maxDistance[idx];
		}
		else if (appType == IMAGE) {
			// Relative to image center
			relX = (x - width/2)  / double(width/2);
			relY = (y - height/2) / double(height/2);
		}
		
		spatialIncrement(p, relX, relY);
	}
	
	if (moments) {
		
	}
	else // if (moments)
		spatialNormalizeQuantize();
	
	
	return result;
}
