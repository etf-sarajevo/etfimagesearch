#include "cedd.h"

#include <cmath>
#include <vector>
#include <QColor>
#include <QDebug>

CEDD::CEDD() : compact(false)
{
}


// Convert RGB to HSV color model
void RGBtoHSV(int& R, int& G, int& B)
{
	// Use Qt color conversion
	/*QColor color(R, G, B);
	color.getHsv(&R, &G, &B);*/
	
	// We must use the following (wrong) color conversion algorithm since all the fuzzy coefficients are tuned for it :(
	int HSV_H = 0;
	int HSV_S = 0;
	int HSV_V = 0;

	double MaxHSV = qMax(R, qMax(G, B));
	double MinHSV = qMin(R, qMin(G, B));

	HSV_V = MaxHSV;

	HSV_S = 0;
	if (MaxHSV != 0) HSV_S = 255 - 255 * (MinHSV / MaxHSV);

	if (MaxHSV != MinHSV) {

		int IntegerMaxHSV = MaxHSV;

		if (IntegerMaxHSV == R && G >= B) {
			HSV_H = 60 * (G - B) / (MaxHSV - MinHSV);
		} else if (IntegerMaxHSV == R && G < B) {
			HSV_H = 359 + 60 * (G - B) / (MaxHSV - MinHSV);
		} else if (IntegerMaxHSV == G) {
			HSV_H = 119 + 60 * (B - R) / (MaxHSV - MinHSV);
		} else if (IntegerMaxHSV == B) {
			HSV_H = 239 + 60 * (R - G) / (MaxHSV - MinHSV);
		}


	} else HSV_H = 0;
	
	R = HSV_H;
	G = HSV_S;
	B = HSV_V;
}


// ---- FUZZY 10/24 BIN
// Calculate 10bin or 24bin fuzzy histogram using multiparticipate defazzificator


// Membership functions
static double HueMembershipValues[] = {0, 0, 5, 10,
		5, 10, 35, 50,
		35, 50, 70, 85,
		70, 85, 150, 165,
		150, 165, 195, 205,
		195, 205, 265, 280,
		265, 280, 315, 330,
		315, 330, 360, 360}; // Table Dimensions= Number Of Triangles X 4 (Start - Stop)

static double SaturationMembershipValues[] = {0, 0, 10, 75,
		10, 75, 255, 255};

static double ValueMembershipValues[] = {0, 0, 10, 75,
		10, 75, 180, 220,
		180, 220, 255, 255};

static double Saturation24MembershipValues[] = {0, 0, 68, 188,
		68, 188, 255, 255};

static double Value24MembershipValues[] = {0, 0, 68, 188,
		68, 188, 255, 255};

static int Fuzzy10BinRulesDefinition[][4] = {
		{0, 0, 0, 2},
		{0, 1, 0, 2},
		{0, 0, 2, 0},
		{0, 0, 1, 1},
		{1, 0, 0, 2},
		{1, 1, 0, 2},
		{1, 0, 2, 0},
		{1, 0, 1, 1},
		{2, 0, 0, 2},
		{2, 1, 0, 2},
		{2, 0, 2, 0},
		{2, 0, 1, 1},
		{3, 0, 0, 2},
		{3, 1, 0, 2},
		{3, 0, 2, 0},
		{3, 0, 1, 1},
		{4, 0, 0, 2},
		{4, 1, 0, 2},
		{4, 0, 2, 0},
		{4, 0, 1, 1},
		{5, 0, 0, 2},
		{5, 1, 0, 2},
		{5, 0, 2, 0},
		{5, 0, 1, 1},
		{6, 0, 0, 2},
		{6, 1, 0, 2},
		{6, 0, 2, 0},
		{6, 0, 1, 1},
		{7, 0, 0, 2},
		{7, 1, 0, 2},
		{7, 0, 2, 0},
		{7, 0, 1, 1},
		{0, 1, 1, 3},
		{0, 1, 2, 3},
		{1, 1, 1, 4},
		{1, 1, 2, 4},
		{2, 1, 1, 5},
		{2, 1, 2, 5},
		{3, 1, 1, 6},
		{3, 1, 2, 6},
		{4, 1, 1, 7},
		{4, 1, 2, 7},
		{5, 1, 1, 8},
		{5, 1, 2, 8},
		{6, 1, 1, 9},
		{6, 1, 2, 9},
		{7, 1, 1, 3},
		{7, 1, 2, 3}
};  // 48 0 


static int Fuzzy24BinRulesDefinition[][3] = {
		{1, 1, 1},
		{0, 0, 2},
		{0, 1, 0},
		{1, 0, 2}
};


void fuzzyFindMembership(double Input, double Triangles[], std::vector<double>& Membership)
{
	int Temp = 0;

	for (int i(0); i < Membership.size() * 4; i += 4) {
		Membership[Temp] = 0;
	
		// Current triangle
		if (Input >= Triangles[i + 1] && Input <= Triangles[i + 2]) {
			Membership[Temp] = 1;
		}
	
		// Left slope
		if (Input >= Triangles[i] && Input < Triangles[i + 1]) {
			Membership[Temp] = (Input - Triangles[i]) / (Triangles[i + 1] - Triangles[i]);
		}
	
		// Right slope
		if (Input > Triangles[i + 2] && Input <= Triangles[i + 3]) {
			Membership[Temp] = (Input - Triangles[i + 2]) / (Triangles[i + 2] - Triangles[i + 3]) + 1;
		}
	
		Temp += 1;
	}
}

void multiParticipateDefazzificator( std::vector<std::vector<int> > rules, std::vector<std::vector<double> > inputs, double result[] )
{
	int ruleActivation = -1;
	for (int i(0); i < rules.size(); i++) {
		// Does the rule match?
		bool match(true);
		double min( inputs[ 0 ][ rules[i][0] ] );
		for (int j(0); j<inputs.size(); j++) {
			if (inputs[ j ][ rules[i][j] ] <= 0)
				match=false;
			if (inputs[ j ][ rules[i][j] ] < min) 
				min = inputs[ j ][ rules[i][j] ];
		}
		
		if ( match ) {
			ruleActivation = rules[i][ inputs.size() ]; // Output
			result[ruleActivation] += min;
		}
	}
}


void fuzzy10bin(int Hue, int Saturation, int Value, double result[]) {
	std::vector<double> HueActivation(8), SaturationActivation(2), ValueActivation(3);
	
	fuzzyFindMembership( Hue, HueMembershipValues, HueActivation );
	fuzzyFindMembership( Saturation, SaturationMembershipValues, SaturationActivation );
	fuzzyFindMembership( Value, ValueMembershipValues, ValueActivation );
	
	// Prepare inputs for defazzificator
	std::vector<std::vector<int> > Fuzzy10BinRules;
	for (int i(0); i<48; i++)
		Fuzzy10BinRules.push_back( std::vector<int>(  Fuzzy10BinRulesDefinition[i], Fuzzy10BinRulesDefinition[i]+4 ));
	std::vector<std::vector<double> > inputs;
	inputs.push_back(HueActivation);
	inputs.push_back(SaturationActivation);
	inputs.push_back(ValueActivation);
	
	multiParticipateDefazzificator( Fuzzy10BinRules, inputs, result );
}


void fuzzy24bin(int Hue, int Saturation, int Value, double fuzzy10bin[], double result[]) {
	std::vector<double> SaturationActivation(2), ValueActivation(2);
	
	fuzzyFindMembership( Saturation, Saturation24MembershipValues, SaturationActivation );
	fuzzyFindMembership( Value, Value24MembershipValues, ValueActivation );
	
	double tmpresult[3] = {0};

	// Skip defazzificator if there is no color
	double Temp(0);
	for (int i(3); i<10; i++)
		Temp += fuzzy10bin[i];
	
	if (Temp > 0) {
		// Prepare inputs for defazzificator
		std::vector<std::vector<int> > Fuzzy24BinRules;
		for (int i(0); i<4; i++)
			Fuzzy24BinRules.push_back( std::vector<int>(  Fuzzy24BinRulesDefinition[i], Fuzzy24BinRulesDefinition[i]+3 ));
		std::vector<std::vector<double> > inputs;
		inputs.push_back(SaturationActivation);
		inputs.push_back(ValueActivation);
		
		multiParticipateDefazzificator( Fuzzy24BinRules, inputs, tmpresult );
	}
	
	for (int i(0); i<3; i++)
		result[i] += fuzzy10bin[i];
	for (int i(3); i<10; i++) {
		result[ (i - 2) * 3     ] += fuzzy10bin[i] * tmpresult[0];
		result[ (i - 2) * 3 + 1 ] += fuzzy10bin[i] * tmpresult[1];
		result[ (i - 2) * 3 + 2 ] += fuzzy10bin[i] * tmpresult[2];
	}
	
}



// ---- QUANTIZE
// Quantization to 3 bits per bin using a distribution of values calculated using GA optimizer

static double QuantTable[][8] = {
	{180.19686541079636, 23730.024499150866, 61457.152912541605, 113918.55437576842, 179122.46400035513, 260980.3325940354, 341795.93301552488, 554729.98648386425},
	{209.25176965926232, 22490.5872862417345, 60250.8935141849988, 120705.788057580583, 181128.08709063051, 234132.081356900555, 325660.617733105708, 520702.175858657472},
	{405.4642173212585, 4877.9763319071481, 10882.170090625908, 18167.239081219657, 27043.385568785292, 38129.413201299016, 52675.221316293857, 79555.402607004813},
	{405.4642173212585, 4877.9763319071481, 10882.170090625908, 18167.239081219657, 27043.385568785292, 38129.413201299016, 52675.221316293857, 79555.402607004813},
	{968.88475977695578, 10725.159033657819, 24161.205360376698, 41555.917344385321, 62895.628446402261, 93066.271379694881, 136976.13317822068, 262897.86056221306},
	{968.88475977695578, 10725.159033657819, 24161.205360376698, 41555.917344385321, 62895.628446402261, 93066.271379694881, 136976.13317822068, 262897.86056221306}
};


FeatureVector quantizeCEDD( std::vector<double> histogram, int histogramSize )
{
	FeatureVector quantizedHistogram;
	quantizedHistogram.resize( histogram.size() );
	double distance;
	double min;
	
	/*qDebug() << "Histogram:";
	ImageFeatures::vectorDump(histogram);*/
	
	for (int k(0); k < 6; k++) {
		for (int i(0); i < histogramSize; i++) {
			int idx = k * histogramSize + i;
			quantizedHistogram[idx] = 0;
			min = 1;
			for (int j(0); j < 8; j++) {
				distance = fabs(histogram[idx] - QuantTable[k][j] / 1000000);
				if (distance < min) {
					min = distance;
					quantizedHistogram[idx] = j;
				}
			}
		}
	}
	return quantizedHistogram;
}




FeatureVector CEDD::extractFeatures(const uchar* imageData, int width, int height)
{
	// Thresholds
	double T0(14), T1(0.68), T2(0.98), T3(0.98);
	
	QString output;
	
	// Some variables
	int MeanRed, MeanGreen, MeanBlue;
	double NeighborhoodArea[4];
	double MaskResults[5];
	int Edges[6] = {0};
	std::vector<double> CEDD(144, 0);
	double PixelCount[2][2];
	
	// Calculate step sizes for edge detection
	// If image is very large (over 10MP) we will use larger steps for better results
	int NumberOfBlocks = 1600;
	int Step_X =  width / sqrt(NumberOfBlocks);
	int Step_Y = height / sqrt(NumberOfBlocks);
	
	if (Step_X % 2 != 0) Step_X--;
	if (Step_Y % 2 != 0) Step_Y--;
	
	if (Step_X < 2) Step_X = 2;
	if (Step_Y < 2) Step_Y = 2;
	
	// Loop through image using Step_X & Step_Y
	for (int y(0); y < height - Step_Y; y += Step_Y) {
		for (int x(0); x < width - Step_X; x += Step_X) {
			
			// Initialize everything
			MeanRed = MeanGreen = MeanBlue = 0;
			for (int i(0); i<4; i++) NeighborhoodArea[i] = 0;
			for (int i(0); i<5; i++) MaskResults[i] = 0;
			for (int i(0); i<6; i++) Edges[i] = -1;
			for (int i(0); i<2; i++) for (int j(0); j<2; j++) PixelCount[i][j] = 0;
			
			// Process one block
			for (int i(y); i < y + Step_Y; i++) {
				for (int j(x); j < x + Step_X; j++) {
					// Get colors
					int idx = (i * width + j) * 4;
					int R = imageData[idx+2];
					int G = imageData[idx+1];
					int B = imageData[idx];
					
					// Mean colors
					MeanRed   += R;
					MeanGreen += G;
					MeanBlue  += B;
					
					// Increment gray levels for pixel neighborhood (for edge detection)
					int gray = 0.114 * B + 0.587 * G + 0.299 * R;
					double areaValue = double(4 * gray) / (Step_X * Step_Y);
					if (i < (y + Step_Y / 2))
						if (j < (x + Step_X / 2))
							NeighborhoodArea[0] += areaValue;
						else
							NeighborhoodArea[1] += areaValue;
					else
						if (j < (x + Step_X / 2))
							NeighborhoodArea[2] += areaValue;
						else
							NeighborhoodArea[3] += areaValue;
				}
			}
			
			// Calculate masks
			double sr2 = sqrt(2);
			MaskResults[0] = fabs( NeighborhoodArea[0] * 2   + NeighborhoodArea[1] * (-2) + NeighborhoodArea[2] * (-2) + NeighborhoodArea[3] * 2    );
			MaskResults[1] = fabs( NeighborhoodArea[0] * 1   + NeighborhoodArea[1] * 1    + NeighborhoodArea[2] * (-1) + NeighborhoodArea[3] * (-1) );
			MaskResults[2] = fabs( NeighborhoodArea[0] * 1   + NeighborhoodArea[1] * (-1) + NeighborhoodArea[2] * 1    + NeighborhoodArea[3] * (-1) );
			MaskResults[3] = fabs( NeighborhoodArea[0] * sr2 + NeighborhoodArea[1] * 0    + NeighborhoodArea[2] * 0    + NeighborhoodArea[3] * (-sr2) );
			MaskResults[4] = fabs( NeighborhoodArea[0] * 0   + NeighborhoodArea[1] * sr2  + NeighborhoodArea[2] * (-sr2) + NeighborhoodArea[3] * 0  );
			
			// Normalize masks
			double Max = MaskResults[0];
			for (int i(1); i<5; i++)
				if (MaskResults[i] > Max) Max = MaskResults[i];
			
			/*output="";
			for (int i(0); i<4; i++)
				output += QString("%1,").arg(NeighborhoodArea[i]);
			qDebug() << "Areas: "<<output;

			output="";
			for (int i(0); i<5; i++)
				output += QString("%1,").arg(MaskResults[i]);
			qDebug() << "Masks: "<<output<<" Max: "<<Max;*/
			
			for (int i(0); i<5; i++)
				MaskResults[i] = MaskResults[i] / Max;
			
			// Populate edges array
			int T = -1;
			if (Max < T0) {
				Edges[0] = 0;
				T = 0;
			} else {
				if (MaskResults[0] > T1)
					Edges[++T] = 1;
				if (MaskResults[1] > T2)
					Edges[++T] = 2;
				if (MaskResults[2] > T2)
					Edges[++T] = 3;
				if (MaskResults[3] > T3)
					Edges[++T] = 4;
				if (MaskResults[4] > T3)
					Edges[++T] = 5;
			}
			
			/*output="";
			for (int i(0); i<5; i++)
				output += QString("%1,").arg(MaskResults[i]);
			qDebug() << "Masks: "<<output<<" Max: "<<Max;
			
			output="";
			for (int i = 0; i <= T; i++) {
				output += QString("T=%1 Edge=%2,").arg(i).arg(Edges[i]);
			}
			qDebug()<<"Etches: "<<output;*/
			
			// Finish means calculation
			MeanRed   = double(MeanRed)   / (Step_X * Step_Y);
			MeanGreen = double(MeanGreen) / (Step_X * Step_Y);
			MeanBlue  = double(MeanBlue)  / (Step_X * Step_Y);
			
			//qDebug() << "RGB" << MeanRed << MeanGreen << MeanBlue;
			RGBtoHSV(MeanRed, MeanGreen, MeanBlue);
			
			//qDebug() << "HSV" << MeanRed << MeanGreen << MeanBlue;
			
			if (!compact) {
				double fuzzyHisto10[10] = {0}, fuzzyHisto24[24] = {0};
				fuzzy10bin( MeanRed, MeanGreen, MeanBlue, fuzzyHisto10 );
				
				/*output = "";
				for (int i(0); i<10; i++)
					output += QString("%1,").arg(fuzzyHisto10[i]);
				qDebug() << "FH10" << output;*/

				fuzzy24bin( MeanRed, MeanGreen, MeanBlue, fuzzyHisto10, fuzzyHisto24 );
				
				/*output = "";
				for (int i(0); i<24; i++)
					output += QString("%1,").arg(fuzzyHisto24[i]);
				qDebug() << "FH24" << output;*/
				
				for (int i(0); i <= T; i++)
					for (int j(0); j < 24; j++)
						if (fuzzyHisto24[j] > 0)
							CEDD[24 * Edges[i] + j] += fuzzyHisto24[j];
				
			} else {
				double fuzzyHisto[10] = {0};
				fuzzy10bin( MeanRed, MeanGreen, MeanBlue, fuzzyHisto );
				for (int i(0); i <= T; i++)
					for (int j(0); j < 10; j++)
						if (fuzzyHisto[j] > 0)
							CEDD[10 * Edges[i] + j] += fuzzyHisto[j];
			}
		}
	}
	
	
	/*qDebug() << "Pre norm:";
	ImageFeatures::vectorDump(CEDD);*/
	
	// Normalize histogram
	double Sum(0);
	for (int i(0); i < 144; i++)
		Sum += CEDD[i];
	for (int i(0); i < 144; i++)
		CEDD[i] = CEDD[i] / Sum;
	
	// Quantize
	FeatureVector fv;
	fv.resize(size());
	if (compact)
		fv = quantizeCEDD( CEDD, 10 );
	else
		fv = quantizeCEDD( CEDD, 24 );
	
	/*qDebug() << "Quantized:";
	ImageFeatures::vectorDump(fv);*/
	return fv;
}


double CEDD::distance(FeatureVector f1, FeatureVector f2)
{
	return dmObject.distance(DistanceMetric::TANIMOTO, f1, f2);
	//return DistanceMetric::distance(DistanceMetric::EUCLIDEAN, f1, f2);
}
