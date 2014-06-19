#include "fuzzyspatialhistogram.h"

#include <QDebug>
#include <cmath>

FuzzySpatialHistogram::FuzzySpatialHistogram() : type(ANNULAR), fuzzy(true)
{
	setHistogramNormalization(ColorHistogram::MAX_NORMALIZATION); // Avoid precision loss
	// GRID type
	addVariable(Variable("rows", 3, 1, 10, 1)); // variableValues[6]
	addVariable(Variable("cols", 3, 1, 10, 1)); // variableValues[7]
	addVariable(Variable("k1", 0.05, 0, 1, 0.01)); // variableValues[8]
	addVariable(Variable("k2", 0.05, 0, 1, 0.01)); // variableValues[9]
	// ANNULAR type
	addVariable(Variable("circles", 6, 1, 10, 1)); // variableValues[10]
	// ANGULAR type
	addVariable(Variable("angles", 6, 1, 10, 1)); // variableValues[11]
	// BULLS_EYE type
	addVariable(Variable("centralRegion", 0.2, 0, 1, 0.1)); // variableValues[12]
}



int FuzzySpatialHistogram::numHistograms() {
	if (type==GRID) return variableValues[6] * variableValues[7];
	if (type==ANNULAR) return variableValues[10];
	if (type==ANGULAR) return variableValues[11];
	if (type==ANNO_ANGULAR) return variableValues[10]*variableValues[11];
	if (type==BULLS_EYE) return 1+variableValues[11];
}


void FuzzySpatialHistogram::incrementFSH(const Pixel& p, int width, int height, int x, int y) 
{
	// FuzzyHistogram::incrementHistogram() will increment the first 
	// FuzzyHistogram::size() places in the feature vector
	incrementHistogram(p);
	
	// Now we calculate participation in each of the numHistograms()
	if (type == GRID) {
		int prow = (double(x) / width)  * variableValues[6];
		int pcol = (double(y) / height)  * variableValues[7];
		int offset = pcol*variableValues[7] + prow;
		offset = (offset+1) * FuzzyHistogram::size();
		for (int i(0); i<FuzzyHistogram::size(); i++) {
			result [i+offset] += result[i];
			result [i] = 0;
		}
	}
	else 
	if (type == ANNULAR) {
		double x1 = (x - width/2)  / double(width/2);
		double y1 = (y - height/2) / double(height/2);
		double d  = sqrt(x1*x1+y1*y1) / sqrt(2);
		if (!fuzzy) {
			int offset = d*variableValues[10];
			if (offset >= variableValues[10]) 
				offset = variableValues[10] - 1;
			offset = (offset+1) * FuzzyHistogram::size();
			for (int i(0); i<FuzzyHistogram::size(); i++) {
				result [i+offset] += result[i];
				result [i] = 0;
			}
		} else { // Fuzzy annular histogram
			double segLength = 1.0 / variableValues[10];
			double k1(variableValues[8]), k2(variableValues[9]);
			double lowFuzzy(-k1), lowFixed(k2), highFixed(segLength-k1), highFuzzy(segLength+k2);
			for (int hist(0); hist<variableValues[10]; hist++) {
				double amount;
				if (d<lowFuzzy) break;
				if (d<lowFixed) {
					amount = (d-lowFuzzy) / (lowFixed-lowFuzzy);
				}
				else if (d<=highFixed) {
					amount = 1;
				} else if (d<=highFuzzy) {
					amount = 1 - (d-highFixed) / (highFuzzy - highFixed);
				}
				if (d <= highFuzzy) {
					int offset = (hist+1) * FuzzyHistogram::size();
					for (int i(0); i<FuzzyHistogram::size(); i++) {
						result [i+offset] += result[i]*amount;
//						result [i] = 0;
					}
				}
				lowFuzzy  += segLength;
				lowFixed  += segLength;
				highFixed += segLength;
				highFuzzy += segLength;
			}
			for (int i(0); i<FuzzyHistogram::size(); i++) 
				result[i] = 0;
		}
	}
	else if (type == ANGULAR) {
		double x1 = (x - width/2)  / double(width/2);
		double y1 = (y - height/2) / double(height/2);
		double d=sqrt(x1*x1+y1*y1);
		double angle = asin(x1/d) / (4*atan(1)); // Range is [-0.5,0.5]
		if (y1<0) angle = (angle + 0.5) / 2;
		else angle = (angle + 1.5) / 2;
		if (d==0) angle=0;
		if (!fuzzy) {
			int offset = angle*variableValues[11];
			if (offset >= variableValues[11]) 
				offset = variableValues[11] - 1;
			offset = (offset+1) * FuzzyHistogram::size();
			for (int i(0); i<FuzzyHistogram::size(); i++) {
				result [i+offset] += result[i];
				result [i] = 0;
			}
		} else {
			double segLength = 1.0 / variableValues[11];
			double k1(variableValues[8]), k2(variableValues[9]);
			double lowFuzzy(-k1), lowFixed(k2), highFixed(segLength-k1), highFuzzy(segLength+k2);
			for (int hist(0); hist<variableValues[11]; hist++) {
				double amount;
				if (angle<lowFuzzy) break;
				if (angle<lowFixed) {
					amount = (angle-lowFuzzy) / (lowFixed-lowFuzzy);
				}
				else if (angle<=highFixed) {
					amount = 1;
				} else if (angle<=highFuzzy) {
					amount = 1 - (angle-highFixed) / (highFuzzy - highFixed);
				}
				if (angle <= highFuzzy) {
					int offset = (hist+1) * FuzzyHistogram::size();
					for (int i(0); i<FuzzyHistogram::size(); i++) {
						result [i+offset] += result[i]*amount;
						result [i] = 0;
					}
				}
				lowFuzzy  += segLength;
				lowFixed  += segLength;
				highFixed += segLength;
				highFuzzy += segLength;
			}
		}
	}
	else if (type == ANNO_ANGULAR) {
		double x1 = (x - width/2)  / double(width/2);
		double y1 = (y - height/2) / double(height/2);
		double d=sqrt(x1*x1+y1*y1);
		double angle = asin(x1/d) / (4*atan(1)); // Range is [-0.5,0.5]
		if (y1<0) angle = (angle + 0.5) / 2;
		else angle = (angle + 1.5) / 2;
		d = d / sqrt(2);
		if (d==0) angle=0;
		if (!fuzzy) {
			int offset1 = d*variableValues[10];
			if (offset1 >= variableValues[10]) 
				offset1 = variableValues[10] - 1;
			int offset2 = angle*variableValues[11];
			if (offset2 >= variableValues[11]) 
				offset2 = variableValues[11] - 1;
			//qDebug() << d << angle << offset1 << offset2 << (offset2*variableValues[10] + offset1 + 1);
			int offset = (offset2*variableValues[10] + offset1 + 1) * FuzzyHistogram::size();
			for (int i(0); i<FuzzyHistogram::size(); i++) {
				result [i+offset] += result[i];
				result [i] = 0;
			}
		} else {
			
		}
	}
	else if (type == BULLS_EYE) {
		double x1 = (x - width/2)  / double(width/2);
		double y1 = (y - height/2) / double(height/2);
		double d=sqrt(x1*x1+y1*y1);
		double angle = asin(x1/d) / (4*atan(1)); // Range is [-0.5,0.5]
		if (y1<0) angle = (angle + 0.5) / 2;
		else angle = (angle + 1.5) / 2;
		d = d / sqrt(2);
		if (d==0) angle=0;
		
		if (!fuzzy) {
			int offset;
			if (d <= variableValues[12])
				offset = 0;
			else
				offset = 1+angle*variableValues[11];
			if (offset > variableValues[11]) offset = variableValues[11];
//			if (y1>0)
//			qDebug() << x1 << y1 << (d*sqrt(2)) << angle << offset << asin(y1/(d*sqrt(2)));
			offset = (offset+1) * FuzzyHistogram::size();
			for (int i(0); i<FuzzyHistogram::size(); i++) {
				result [i+offset] += result[i];
				result [i] = 0;
			}
		} else {
			// Determine membership to central region
			double centralRegion = variableValues[12];
			double k1(variableValues[8]), k2(variableValues[9]);
			double amount;
			if (d<k2) {
				amount = (d+k1) / (k2+k1);
			}
			else if (d<=centralRegion-k1) {
				amount = 1;
			} else if (d<=centralRegion+k1) {
				amount = 1 - (d-centralRegion+k1) / (2*k1);
			} else
				amount = 0;
			
			int offset = FuzzyHistogram::size();
			if (amount > 0) {
				for (int i(0); i<FuzzyHistogram::size(); i++) {
					result [i+offset] += result[i]*amount;
					result [i] = 0;
				}
			}
			
			// Determine membership to angular regions
			amount = 1-amount;
			
			double segLength = 1.0 / variableValues[11];
			double lowFuzzy(-k1), lowFixed(k2), highFixed(segLength-k1), highFuzzy(segLength+k2);
			for (int hist(0); hist<variableValues[11]; hist++) {
				double angleAmount;
				if (angle<lowFuzzy) break;
				if (angle<lowFixed) {
					angleAmount = amount * (angle-lowFuzzy) / (lowFixed-lowFuzzy);
				}
				else if (angle<=highFixed) {
					angleAmount = amount;
				} else if (angle<=highFuzzy) {
					angleAmount = (1 - (angle-highFixed) / (highFuzzy - highFixed)) * amount;
				}
				if (angle <= highFuzzy) {
					int offset = (hist+2) * FuzzyHistogram::size();
					for (int i(0); i<FuzzyHistogram::size(); i++) {
						result [i+offset] += result[i]*amount;
						result [i] = 0;
					}
				}
				lowFuzzy  += segLength;
				lowFixed  += segLength;
				highFixed += segLength;
				highFuzzy += segLength;
			}
		}
	}
}


FeatureVector FuzzySpatialHistogram::extractFeatures(const uchar* imageData, int width, int height)
{
	// We will resize the feature vector larger then needed
	// First block is used to call incrementHistogram, then results are copied
	// to the rest of feature vector in incrementFSH
	int resultSize = FuzzyHistogram::size() * ( numHistograms() + 1);
	result.resize( resultSize );
	result.fill( 0 , resultSize );;
	
	Pixel p; // Storage for one pixel
	
	// Calculate histogram
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
		
		incrementFSH(p, width, height, x, y);
	}
		
	histogramNormalizeQuantize(width*height);
	
	// Remove first block since we dont need it anymore
	for (int i(0); i<size(); i++)
		result[i] = result[i+FuzzyHistogram::size()];
	result.resize(size());
	
	return result;
}

