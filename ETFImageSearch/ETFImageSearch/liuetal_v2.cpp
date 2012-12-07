#include "liuetal_v2.h"

#include <cmath>
//#include <iostream>

#define DEBUGLEVEL 0

LiuEtAl_v2::LiuEtAl_v2() : DCTSearchAlgorithm()
{
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
	}
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
	
	/*    if (currentComponent == 0 && colorFeaturesLast1000[currentComponent][0]==0) {
		for (i=0; i<64; i++)
			fprintf(stderr, "%d ",row[i]);
		fprintf(stderr, "\n");
	}*/
	
	colorFeaturesLast1000[component][0] += M11;
	colorFeaturesLast1000[component][1] += M12;
	colorFeaturesLast1000[component][2] += M21;
	colorFeaturesLast1000[component][3] += M22;
	
	colorFeaturesCounters[component]++;
	
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

	// Push data into FeatureVector
	FeatureVector result;
	// ME features
	for (j=0; j<3; j++) {
		for (i=0; i<4; i++) {
			double last1000Average = ((double)colorFeaturesLast1000[j][i]) / 1024;
			colorFeatures[j][i] *= colorFeaturesDoubleCounters[j];
			colorFeatures[j][i] += last1000Average;
			double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
			colorFeatures[j][i] /= divisor;
			
			result.features.push_back( colorFeatures[j][i] );
		}
	}
	
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


qreal LiuEtAl_v2::distance(FeatureVector f1, FeatureVector f2)
{
	qreal ME=0;
	for (int k=0; k<3; k++) {
		qreal sum=0;
		for (int j=0; j<4; j++)
			sum += pow(f1.features[k*4+j] - f2.features[k*4+j], 2);
		ME += sqrt(sum);
	}
	//std::cout << "ME = "<<ME<<std::endl;

	qreal D=0;
	for (int k=0; k<3; k++) {
		qreal sum=0;
		for (int j=0; j<12; j++) {
			sum += pow(f1.features[12 + k*12 + j] - f2.features[12 + k*12 + j], 2);
		}
		D += sqrt(sum);
	}
	//std::cout << "D = "<<D<<std::endl;

	// Weighted formula in paper is absurd
	// double W = 0.99999995*ME + 0.00000005*D;
	// double W = pow(D,3)*ME;
	qreal W = 0.9995*D + 0.005*ME;
	//std::cout << "W = "<<W<<std::endl;
	
	return W;
}
