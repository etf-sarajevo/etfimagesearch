#include "hsl10bin.h"

HSL10bin::HSL10bin() : ColorHistogram(), blackThreshold(0.15), whiteThreshold(0.80), grayThreshold(0.10), hueQuant(7)
{
	setColorModel(Pixel::HSL);
	setColorQuantization(3,3,3);
	setHistogramType(ColorHistogram::COMBINEDHISTOGRAM);
	setHistogramNormalization(ColorHistogram::NO_NORMALIZATION);
	setHistogramQuantization(8);
	setHistogramCumulative(false);
	setDistanceMetric(DistanceMetric::MATSUSHITA);
	
	// Variables
	variables.append(Variable("blackLower", 0, 1, 0.01, 0.06)); variableValues[0] = 0.06;
	variables.append(Variable("blackUpper", 0, 1, 0.01, 0.17)); variableValues[1] = 0.17;
	variables.append(Variable("whiteLower", 0.5, 1, 0.01, 0.60)); variableValues[2] = 0.60;
	variables.append(Variable("whiteUpper", 0.5, 1, 0.01, 0.96)); variableValues[3] = 0.96;
	variables.append(Variable("grayLower",  0, 0.5, 0.01, 0.10)); variableValues[4] = 0.10;
	variables.append(Variable("grayUpper",  0, 0.5, 0.01, 0.21)); variableValues[5] = 0.21;
}

void HSL10bin::setParams(double blackThreshold, double whiteThreshold, double grayThreshold, int hueQuant)
{
	this->blackThreshold = blackThreshold;
	this->whiteThreshold = whiteThreshold;
	this->grayThreshold  = grayThreshold;
	this->hueQuant       = hueQuant;
}

void HSL10bin::colorQuantize(Pixel& p)
{
	return; // Don't quantize color
}

void HSL10bin::incrementHistogram(const Pixel &p)
{
	
	double blackLower=variableValues[0], blackUpper=variableValues[1];
	double  grayLower=variableValues[4],  grayUpper=variableValues[5];
	double whiteLower=variableValues[2], whiteUpper=variableValues[3];
	
	double fuzzyHueBounds[14] = {0, 10, 30, 60, 80, 130, 150, 200, 220, 230, 250, 280, 300, 360};
	int fuzzyHueBoundsCount(14);
		
	double H(double(p.c[0])/255), S(double(p.c[1])/255), L(double(p.c[2])/255);
	int color;
	
	double colorPart = 1.0;
	if (L<=blackLower) {
		result[0]++; // black = 0
		return;
	} else if (L<=blackUpper) {
		double black = (L-blackLower)/(blackUpper-blackLower);
		result[0] += black;
		colorPart = 1-black;
	}
	
	if (L>=whiteUpper) {
		result[1]++; // white = 1
		return;
	} else if (L>=whiteLower) {
		double white = (L-whiteLower)/(whiteUpper-whiteLower);
		result[1] += white;
		colorPart = 1-white;
	}
	
	if (S<=grayLower) {
		result[2] += colorPart; // gray = 2
		return;
	} else if (S<=grayUpper) {
		double gray = (S-grayLower)/(grayUpper-grayLower);
		gray *= colorPart;
		result[2] += gray;
		colorPart -= gray;
	}

	H*=360;
	for (int i(0); i<fuzzyHueBoundsCount-1; i++) {
		if (H>=fuzzyHueBounds[i] && H<=fuzzyHueBounds[i+1]) {
			if (i%2 == 0) {
				int color = 3 + i/2;
				result[color] += colorPart;
			} else {
				int leftColor = 3 + i/2;
				int rightColor = leftColor+1;
				if (rightColor == 10) rightColor=3;
				double rightAmount = (H-fuzzyHueBounds[i])/(fuzzyHueBounds[i+1]-fuzzyHueBounds[i]);
				result[rightColor] += rightAmount*colorPart;
				result[leftColor] += (1-rightAmount)*colorPart;
			}
			break;
		}
	}
	
	/*if (H>=20 && H<70)
		result[4] += colorPart; // yellow
	if (H>=70 && H<140)
		result[5] += colorPart; // green
	if (H>=140 && H<210)
		result[6] += colorPart; // light blue
	if (H>=210 && H<240)
		result[7] += colorPart; // dark blue
	if (H>=240 && H<290)
		result[8] += colorPart; // purple
	if (H>=290 && H<360)
		result[9] += colorPart; // pink
	if (H>=360 || H<20)
		result[3] += colorPart; // red*/
	
	/*if (L<=blackThreshold)
		color=0; // black
	else if (L>=whiteThreshold)
		color=1; // white
	else if (S<=grayThreshold)
		color=2; // gray
	
	*//*// Parts of colorspace where perceived color is white but relationship between S and L is nonlinear
	else if (L>=0.9 && S<=5*L-4.05)
		color=1; // white
	else if (L>=0.8 && S<=3*L-2.25)
		color=1; // white*//*
		
	else {
		
		H*=360;
		if (H>=20 && H<70)
			color=3; // yellow
		if (H>=70 && H<140)
			color=4; // green
		if (H>=140 && H<210)
			color=5; // light blue
		if (H>=210 && H<240)
			color=6; // dark blue
		if (H>=240 && H<290)
			color=7; // purple
		if (H>=290 && H<360)
			color=8; // pink
		if (H>=360 || H<20)
			color=9; // red
		//if (S >= 0.7) color+=7;
		
		/*H*=360;
		if (H>=20 && H<45)
			color=3; // orange
		if (H>=45 && H<70)
			color=4; // yellow
		if (H>=70 && H<140)
			color=5; // green
		if (H>=140 && H<210)
			color=6; // light blue
		if (H>=210 && H<240)
			color=7; // dark blue
		if (H>=240 && H<280)
			color=8; // purple
		if (H>=280 && H<330)
			color=9; // pink
		if (H>=330 || H<20)
			color=10; // red*/
		
		/*H*=hueQuant;
		if (S >= 0.7) H+=hueQuant;
		color = 2+H;
		if (color==2+hueQuant*2) color--;*//*
	}
	result[color]++;*/
}

