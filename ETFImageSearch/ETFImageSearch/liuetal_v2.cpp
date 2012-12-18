#include "liuetal_v2.h"

#include <cmath>
#include <iostream>

#define DEBUGLEVEL 0

LiuEtAl_v2::LiuEtAl_v2() : DCTSearchAlgorithm()
{
	totalMaxM = -100;
	totalMinM = 100;
	kME = 0.06;
	kD = 0.94;
	debug = false;
}

void LiuEtAl_v2::init()
{
	int i,j;
	for (i=0; i<MAX_COMPONENTS; i++) {
		colorFeaturesCounters[i] = colorFeaturesDoubleCounters[i] = 0;
	
		for (j=0; j<4; j++) {
			colorFeatures[i][j] = colorFeaturesLast1000[i][j] = 0;
		}
		for (j=0; j<6; j++) {
			specificBlocks[i][j] = specificBlocksLast1000[i][j] = 0;
			specificBlocksSquares[i][j] = specificBlocksSquaresLast1000[i][j] = 0;
		}
		
		for (j=0; j<64; j++) colorHistogram[i][j] = 0;
	}
	
	maxM=0; minM=1024;
	colorHistogramCounter=0;
	
	for (i=0; i<512; i++)
		bigHistogram[i] = 0;
	for (i=0; i<MAX_COMPONENTS; i++)
		for (j=0; j<4; j++)
			previousComponent[i][j]=0;
}

void LiuEtAl_v2::processBlock(short int* row, int component)
{
	int i;

	// Calculate coefficients M11 M12 M21 M22 from paper
	// libjpeg is not zigzag!!!
	int M11 = (row[0] + row[1] + row[8] + row[9]) >> 4;
	int M12 = (row[0] + row[1] - row[8] - row[9]) >> 4;
	int M21 = (row[0] - row[1] + row[8] - row[9]) >> 4;
	int M22 = (row[0] - row[1] - row[8] + row[9]) >> 4;
	
	// Max/min
	if (M11>maxM) maxM=M11; if (M11<minM) minM=M11;
	if (M12>maxM) maxM=M12; if (M12<minM) minM=M12;
	if (M21>maxM) maxM=M21; if (M21<minM) minM=M21;
	if (M22>maxM) maxM=M22; if (M22<minM) minM=M22;
	
	/*    if (currentComponent == 0 && colorFeaturesLast1000[currentComponent][0]==0) {
		for (i=0; i<64; i++)
			fprintf(stderr, "%d ",row[i]);
		fprintf(stderr, "\n");
	}*/	
//	if (row[0] > maxM) maxM=row[0];
//	if (row[0] < minM) minM = row[0];
	
	// Calculating Mxy means
	colorFeaturesLast1000[component][0] += M11;
	colorFeaturesLast1000[component][1] += M12;
	colorFeaturesLast1000[component][2] += M21;
	colorFeaturesLast1000[component][3] += M22;
	
	colorFeaturesCounters[component]++;
	
	// Calculating component histograms
	colorHistogram[component][M11+32]++;
	colorHistogram[component][M12+32]++;
	colorHistogram[component][M21+32]++;
	colorHistogram[component][M22+32]++;
	colorHistogramCounter++;
	
	// Calculating total histogram
/*	if (component==0 && colorHistogramCounter>1) {
		int idx = (previousComponent[0][0] << 6) + (previousComponent[1][0] << 3) + previousComponent[2][0];
		bigHistogram[idx]++;
		idx = (previousComponent[0][1] << 6) + (previousComponent[1][1] << 3) + previousComponent[2][1];
		bigHistogram[idx]++;
		idx = (previousComponent[0][2] << 6) + (previousComponent[1][2] << 3) + previousComponent[2][2];
		bigHistogram[idx]++;
		idx = (previousComponent[0][3] << 6) + (previousComponent[1][3] << 3) + previousComponent[2][3];
		bigHistogram[idx]++;
	}*/
	previousComponent[component][0] = (M11+32) >> 3;
//	previousComponent[component][0] = row[0] >> 7;
	previousComponent[component][1] = (M12+32) >> 3;
	previousComponent[component][2] = (M21+32) >> 3;
	previousComponent[component][3] = (M22+32) >> 3;
	
#if DEBUGLEVEL>1
	printf ("row[0]=%d row[1]=%d row[2]=%d row[3]=%d\n", row[0], row[1], row[2], row[3]);
	printf ("M11=%d M12=%d M21=%d M22=%d\n", M11, M12, M21, M22);
	printf ("DEBUG (%d,%d): %d %d %d %d\n", component, colorFeaturesCounters[component], colorFeaturesLast1000[component][0], colorFeaturesLast1000[component][1], colorFeaturesLast1000[component][2], colorFeaturesLast1000[component][3]);
#endif
	
	// Coefficients displayed on Figure 3 from paper
	int texture1 = row[0];
	int texture2 = row[1] + row[2] + row[8] + row[9] + row[16];
	int texture3 = row[3] + row[4] + row[10] + row[11] + row[17] + row[18] + row[24] + row[25] + row[32];
	
	int texture4 = /*row[0] +*/ row[8] + row[16] + row[24] + row[32] + row[40] + row[48] + row[56]; /* Error in paper */
	int texture5 = row[1] + row[2] + row[3] + row[4] + row[5] + row[6] + row[7];
	int texture6 = row[9] + row[18] + row[27] + row[36] + row[45] + row[54];
	
	specificBlocksLast1000[component][0] += texture1;
	specificBlocksLast1000[component][1] += texture2;
	specificBlocksLast1000[component][2] += texture3;
	specificBlocksLast1000[component][3] += texture4;
	specificBlocksLast1000[component][4] += texture5;
	specificBlocksLast1000[component][5] += texture6;
	
	specificBlocksSquaresLast1000[component][0] += texture1*texture1;
	specificBlocksSquaresLast1000[component][1] += texture2*texture2;
	specificBlocksSquaresLast1000[component][2] += texture3*texture3;
	specificBlocksSquaresLast1000[component][3] += texture4*texture4;
	specificBlocksSquaresLast1000[component][4] += texture5*texture5;
	specificBlocksSquaresLast1000[component][5] += texture6*texture6;
	
	// Recalculating average every 1024 blocks so that sums wouldn't overflow (especially the squares sum)
	if (colorFeaturesCounters[component] == 1024) {
		for (i=0; i<4; i++) {
			double last1000Average = ((double)colorFeaturesLast1000[component][i]) / 1024;
			colorFeatures[component][i] *= colorFeaturesDoubleCounters[component];
			colorFeatures[component][i] += last1000Average;
			colorFeatures[component][i] /= (colorFeaturesDoubleCounters[component] + 1);
			colorFeaturesLast1000[component][i] = 0;
#if DEBUGLEVEL>1
			printf ("AVERAGE %lf %lf\n", last1000Average, colorFeatures[component][i]);
#endif
		}
		colorFeaturesDoubleCounters[component]++;
		colorFeaturesCounters[component] = 0;
	
		for (i=0; i<6; i++) {
			double last1000Average = ((double)specificBlocksLast1000[component][i]) / 1024;
			specificBlocks[component][i] *= colorFeaturesDoubleCounters[component];
			specificBlocks[component][i] += last1000Average;
			specificBlocks[component][i] /= (colorFeaturesDoubleCounters[component] + 1);
			specificBlocksLast1000[component][i] = 0;
		}
		for (i=0; i<6; i++) {
			double last1000Average = ((double)specificBlocksSquaresLast1000[component][i]) / 1024;
			specificBlocksSquares[component][i] *= colorFeaturesDoubleCounters[component];
			specificBlocksSquares[component][i] += last1000Average;
			specificBlocksSquares[component][i] /= (colorFeaturesDoubleCounters[component] + 1);
			specificBlocksSquaresLast1000[component][i] = 0;
		}
	}
}


// Calculate the final average
FeatureVector LiuEtAl_v2::calculateVector()
{
	int i,j;
	
//	std::cout << "maxM "<<maxM<<" minM "<<minM<<std::endl;
	if (maxM > totalMaxM) totalMaxM = maxM;
	if (minM < totalMinM) totalMinM = minM;
//g	std::cout << "maxM "<<totalMaxM<<" minM "<<totalMinM<<std::endl;

	// Push data into FeatureVector
	FeatureVector result;
	// ME features
	/*for (j=0; j<3; j++) {
		for (i=0; i<4; i++) {
			double last1000Average = ((double)colorFeaturesLast1000[j][i]) / 1024;
			colorFeatures[j][i] *= colorFeaturesDoubleCounters[j];
			colorFeatures[j][i] += last1000Average;
			double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
			colorFeatures[j][i] /= divisor;
			
			result.features.push_back( colorFeatures[j][i] );
		}
	}*/
	
	// New features, based on histogram
	for (j=0; j<3; j++) {
		// Convert histogram to cumulative histogram and find maximum
		int max=0;
		for (int i=1; i<64; i++) {
			colorHistogram[j][i] += colorHistogram[j][i-1];
			if (colorHistogram[j][i] > max)
				max = colorHistogram[j][i];
		}
		// Feature is (histogram / max) * 256
		for (int i=0; i<64; i++)
			result.features.push_back( 256 * qreal(colorHistogram[j][i]) / max );
	}
	
	// One big histogram
/*	int max=0;
	for (j=0; j<512; j++) {
		if (bigHistogram[j] > max) max=bigHistogram[j];
	}
	for (j=0; j<512; j++) {
		result.features.push_back( 256 * qreal(bigHistogram[j]) / max);
	}*/
	
	// D features
	for (j=0; j<3; j++) {
		for (i=0; i<6; i++) {
			double last1000Average = ((double)specificBlocksLast1000[j][i]) / 1024;
			specificBlocks[j][i] *= colorFeaturesDoubleCounters[j];
			specificBlocks[j][i] += last1000Average;
			double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
			specificBlocks[j][i] /= divisor;
			result.features.push_back( specificBlocks[j][i] );
		}
		for (i=0; i<6; i++) {
			double last1000Average = ((double)specificBlocksSquaresLast1000[j][i]) / 1024;
			specificBlocksSquares[j][i] *= colorFeaturesDoubleCounters[j];
			specificBlocksSquares[j][i] += last1000Average;
			double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
			specificBlocksSquares[j][i] /= divisor;
	
			// Std dev = square root of (mean of squares minus square of mean)
			double stddev = specificBlocksSquares[j][i] - specificBlocks[j][i]*specificBlocks[j][i];
			if (stddev<0) stddev = -stddev;
			stddev = sqrt(stddev);
			result.features.push_back( stddev );
		}
	}
	
	return result;
}


double LiuEtAl_v2::distance(FeatureVector f1, FeatureVector f2)
{
	qreal ME=0;
	for (int k=0; k<3; k++) {
		qreal sum=0;
//		for (int j=0; j<4; j++)
		for (int j=0; j<64; j++)
			sum += pow(f1.features[k*64+j] - f2.features[k*64+j], 2);
		ME += sqrt(sum);
	}
	
	if (debug) std::cout << "ME = "<<ME<<std::endl;
/*	qreal sum=0;
	for (int k=0; k<512; k++) {
		sum += pow(f1.features[k] - f2.features[k], 2);
	}
	ME = sqrt(sum);*/

	qreal D=0;
	for (int k=0; k<3; k++) {
		qreal sum=0;
		for (int j=0; j<12; j++) {
			sum += pow(f1.features[192 + k*12 + j] - f2.features[192 + k*12 + j], 2);
		}
		D += sqrt(sum);
	}
	if (debug) std::cout << "D = "<<D<<std::endl;

	// Weighted formula in paper is absurd
	// double W = 0.99999995*ME + 0.00000005*D;
	// double W = pow(D,3)*ME;
	// qreal W = 0.995*D + 0.005*ME;
	qreal W = kD*D + kME*ME;
	//std::cout << "W = "<<W<<std::endl;
	
	return W;
}
