#include "fuzzyhistogram.h"

FuzzyHistogram::FuzzyHistogram() : ColorHistogram(), blackModel(Pixel::HSL), whiteModel(Pixel::HSL), grayModel(Pixel::HSL), hueModel(Pixel::HSL), numberOfBins(10)
{
	setColorModel(Pixel::RGB);
	setColorQuantization(3,3,3);
	setHistogramType(ColorHistogram::COMBINEDHISTOGRAM);
	setHistogramNormalization(ColorHistogram::NO_NORMALIZATION);
	setHistogramQuantization(8);
	setHistogramCumulative(false);
	setDistanceMetric(DistanceMetric::MATSUSHITA);
	
	// Variables
	addVariable(Variable("blackLower", 0.06, 0, 1, 0.01)); // variableValues[0] = 0.06;
	addVariable(Variable("blackUpper", 0.17, 0, 1, 0.01)); // variableValues[1] = 0.17;
	addVariable(Variable("whiteLower", 0.60, 0.5, 1, 0.01)); // variableValues[2] = 0.60;
	addVariable(Variable("whiteUpper", 0.96, 0.5, 1, 0.01)); // variableValues[3] = 0.96;
	addVariable(Variable("grayLower",  0.10, 0, 0.5, 0.01)); // variableValues[4] = 0.10;
	addVariable(Variable("grayUpper",  0.21, 0, 0.5, 0.01)); // variableValues[5] = 0.21;
}

void FuzzyHistogram::setParams(QString params)
{
	QStringList paramsList = params.split(';');
	blackModel == Pixel::fromString(paramsList[0]);
	whiteModel == Pixel::fromString(paramsList[1]);
	grayModel  == Pixel::fromString(paramsList[2]);
	bool ok;
	numberOfBins == paramsList[2].toInt(&ok);
	
	// Optimize hue model to avoid unneccessary conversions
	if (blackModel == Pixel::HSV || blackModel == Pixel::HSL || blackModel == Pixel::IHLS || blackModel == Pixel::HMMD)
		hueModel = blackModel;
	else if (whiteModel == Pixel::HSV || whiteModel == Pixel::HSL || whiteModel == Pixel::IHLS || whiteModel == Pixel::HMMD)
		hueModel = whiteModel;
	else if (grayModel == Pixel::HSV || grayModel == Pixel::HSL || grayModel == Pixel::IHLS || grayModel == Pixel::HMMD)
		hueModel = grayModel;
	else
		hueModel = Pixel::HSV;
}

QString FuzzyHistogram::getParams()
{
	QStringList paramsList;
	paramsList.append(Pixel::toString(blackModel));
	paramsList.append(Pixel::toString(whiteModel));
	paramsList.append(Pixel::toString(grayModel));
	paramsList.append(QString("%1").arg(numberOfBins));
	return paramsList.join(QString(';'));
}


void FuzzyHistogram::colorQuantize(Pixel& p)
{
	return; // Don't quantize color
}

// Some smart shit to avoid unneccessary color model conversions
void FuzzyHistogram::convertModels(double& hue, double& black, double& white, double& gray, const Pixel& p)
{
	Pixel huePix(p);
	huePix.convertColorModel(hueModel);
	
	Pixel blackPix(p), whitePix(p), grayPix(p);
	if (blackModel == hueModel)
		blackPix = huePix;
	else
		blackPix.convertColorModel(blackModel);
	
	if (whiteModel == hueModel)
		whitePix = huePix;
	else if (whiteModel == blackModel)
		whitePix = blackPix;
	else
		whitePix.convertColorModel(whiteModel);
	
	if (grayModel == hueModel)
		grayPix = huePix;
	else if (grayModel  == blackModel)
		grayPix = blackPix;
	else if (grayModel  == whiteModel)
		grayPix = whitePix;
	else
		grayPix.convertColorModel(grayModel);
	
	hue = double(huePix.c[0])/255; // Not strictly correct but...
	if (blackModel == Pixel::YUV || blackModel == Pixel::LAB)
		black = double(blackPix.c[0])/255;
	else
		black = double(blackPix.c[2])/255;

	if (whiteModel == Pixel::YUV || whiteModel == Pixel::LAB)
		white = double(whitePix.c[0])/255;
	else
		white = double(whitePix.c[2])/255;
	
	gray = double(grayPix.c[1])/255; // results for YUV, LAB, RGB etc. will be meaningless
}

void FuzzyHistogram::incrementHistogram(const Pixel &p)
{
	
	double blackLower=variableValues[0], blackUpper=variableValues[1];
	double  grayLower=variableValues[4],  grayUpper=variableValues[5];
	double whiteLower=variableValues[2], whiteUpper=variableValues[3];
	
	double fuzzyHueBounds[14] = {0, 10, 30, 60, 80, 130, 150, 200, 220, 230, 250, 280, 300, 360};
	int fuzzyHueBoundsCount(14);
	
	double hue, black, white, gray;
	convertModels(hue,black,white,gray,p);
		
	int color;
	
	double colorPart = 1.0;
	if (black<=blackLower) {
		result[0]++; // black = 0
		return;
	} else if (black<=blackUpper) {
		double blackPart = (black-blackLower)/(blackUpper-blackLower);
		result[0] += blackPart;
		colorPart = 1-blackPart;
	}
	
	if (white>=whiteUpper) {
		result[1]++; // white = 1
		return;
	} else if (white>=whiteLower) {
		double whitePart = (white-whiteLower)/(whiteUpper-whiteLower);
		result[1] += whitePart;
		colorPart = 1-whitePart;
	}
	
	if (gray<=grayLower) {
		result[2] += colorPart; // gray = 2
		return;
	} else if (gray<=grayUpper) {
		double grayPart = (gray-grayLower)/(grayUpper-grayLower);
		grayPart *= colorPart;
		result[2] += grayPart;
		colorPart -= grayPart;
	}

	hue*=360;
	for (int i(0); i<fuzzyHueBoundsCount-1; i++) {
		if (hue>=fuzzyHueBounds[i] && hue<=fuzzyHueBounds[i+1]) {
			if (i%2 == 0) {
				int color = 3 + i/2;
				result[color] += colorPart;
			} else {
				int leftColor = 3 + i/2;
				int rightColor = leftColor+1;
				if (rightColor == 10) rightColor=3;
				double rightAmount = (hue-fuzzyHueBounds[i])/(fuzzyHueBounds[i+1]-fuzzyHueBounds[i]);
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

void FuzzyHistogram::setVariable(QString name, double value)
{
	ImageFeatures::setVariable(name, value);
	if (getVariable("blackLower").value > getVariable("blackUpper").value)
		throw "blackLower > blackUpper";
	if (getVariable("whiteLower").value > getVariable("whiteUpper").value)
		throw "whiteLower > whiteUpper";
	if (getVariable("grayLower").value  > getVariable("grayUpper").value)
		throw "grayLower  > grayUpper";
}

