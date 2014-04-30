#include "fuzzyspatialhistogram.h"

#include <QDebug>
#include <cmath>

FuzzySpatialHistogram::FuzzySpatialHistogram() : type(ANGULAR), fuzzy(false)
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
		double d=sqrt(x1*x1+y1*y1) / sqrt(2);
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
		int offset;
		if (d <= variableValues[12])
			offset = 0;
		else
			offset = 1+angle*variableValues[11];
		if (offset > variableValues[11]) offset = variableValues[11];
//		if (y1>0)
//		qDebug() << x1 << y1 << (d*sqrt(2)) << angle << offset << asin(y1/(d*sqrt(2)));
		offset = (offset+1) * FuzzyHistogram::size();
		for (int i(0); i<FuzzyHistogram::size(); i++) {
			result [i+offset] += result[i];
			result [i] = 0;
		}
	}
	/*else {
	for (int i(0); i<FuzzyHistogram::size(); i++) {
		for (int j(0); j<numHistograms(); j++) {
			if (participation[j][x*y] > 0) {
				result[i + j*FuzzyHistogram::size()] += result[i] * participation[j][x*y];
			}
		}
		result [i] = 0;
	}
	}*/
}

void FuzzySpatialHistogram::precalculateFuzzyBoundsTable(int width, int height)
{
	participation.resize( numHistograms() );
	
	for (int i(0); i<numHistograms(); i++) {
		participation[i].resize(width*height);
		participation[i].fill(0, width*height);
		
		if (type == GRID) {
			int rows = variableValues[6];
			int cols = variableValues[7];
			double gridk1 = variableValues[8];
			double gridk2 = variableValues[9];
			qDebug()<<"w"<<width<<"h"<<height;
			
			double fuzzyStartX = (double(width) / rows) * ( i % rows - gridk1);
			double fixedStartX = (double(width) / rows) * ( i % rows + gridk2);
			double fixedEndX = (double(width) / rows) * ( i % rows + 1 - gridk2);
			double fuzzyEndX = (double(width) / rows) * ( i % rows + 1 + gridk1);
			
			double fuzzyStartY = (double(height) / cols) * ( i / rows - gridk1);
			double fixedStartY = (double(height) / cols) * ( i / rows + gridk2);
			double fixedEndY = (double(height)   / cols) * ( i / rows + 1 - gridk2);
			double fuzzyEndY = (double(height)   / cols) * ( i / rows + 1 + gridk1);
			
			if (fuzzyStartX<0) fuzzyStartX=0;
			if (fuzzyEndX>width) fuzzyEndX=width;
			if (fuzzyStartY<0) fuzzyStartY=0;
			if (fuzzyEndY>height) fuzzyEndY=height;
			
			qDebug()<<"fsx"<<fuzzyStartX<<"ffx"<<fixedStartX<<"fex"<<fixedEndX<<"ffex"<<fuzzyEndX;
			qDebug()<<"fsy"<<fuzzyStartY<<"ffy"<<fixedStartY<<"fey"<<fixedEndY<<"ffey"<<fuzzyEndY;
			
			
			for (int y(fuzzyStartY); y<fuzzyEndY; y++) {
				QString s;
				for (int x(fuzzyStartX); x<fuzzyEndX; x++) {
					if (x>=fixedStartX && x<=fixedEndX) 
						participation[i][y*width+x] = 1;
					else if (x<fixedStartX)
						participation[i][y*width+x] = double(x-fuzzyStartX) / (fixedStartX-fuzzyStartX);
					else
						participation[i][y*width+x] = double(x-fixedEndX) / (fuzzyEndX-fixedEndX);
					
					if (y>=fixedStartY && y<=fixedEndY)
						participation[i][y*width+x] *= 1;
					else if (y<fixedStartY)
						participation[i][y*width+x] *= double(y-fuzzyStartY) / (fixedStartY-fuzzyStartY);
					else
						participation[i][y*width+x] *= double(y-fixedEndY) / (fuzzyEndY-fixedEndY);
					if (participation[i][y*width+x]<0) participation[i][y*width+x]=0;
					s+= QString("%1 ").arg(participation[i][y*width+x]);
				}
				//qDebug() << s;
			}
		} // if (type == GRID)
		
	}
}


FeatureVector FuzzySpatialHistogram::extractFeatures(const uchar* imageData, int width, int height)
{
	// We will resize the feature vector larger then needed
	int resultSize = FuzzyHistogram::size() * ( numHistograms() + 1);
	result.resize( resultSize );
	result.fill( 0 , resultSize );
	participation.resize( numHistograms() );
	
	precalculateFuzzyBoundsTable(width, height);
	
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
	
	for (int i(0); i<FuzzyHistogram::size() * ( numHistograms()); i++) {
		result[i] = result[i+FuzzyHistogram::size()];
	}
	result.resize( FuzzyHistogram::size() * numHistograms() );
	
	return result;
}

