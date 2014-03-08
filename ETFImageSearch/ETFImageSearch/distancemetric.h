#ifndef DISTANCEMETRIC_H
#define DISTANCEMETRIC_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "featurevector.h"

/**
 * Common algorithms for calculating distance between two points.
 */

class DistanceMetric
{
public:
	enum Type { 
		MATSUSHITA, 
		BRAY_CURTIS, 
		MANHATTAN, 
		EUCLIDEAN, 
		SOERGEL, 
		BHATTACHARYA, 
		CHI_SQUARE, 
		CANBERRA, 
		HIST_INT, 
		JSD, 
		ANGULAR, 
		CHORD, 
		WAVE_HEDGES, 
		WED, 
		K_S, 
		KUIPER, 
		MEAN,
		TANIMOTO,
		NED,
		QUADRATIC,
		MAHALANOBIS,
		PCC
	};
	
	/**
	 * Get distance between two feature vectors using given metric.
	 */
	double distance(Type distanceMetric, const FeatureVector &f1, const FeatureVector &f2);
	
	/**
	 * Get type from string name.
	 */
	static Type fromString(QString string);
	
	/**
	 * Get string name from type.
	 */
	static QString toString(Type type);
	
	/**
	 * List all available types as strings.
	 */
	static QStringList allMetrics();
	
	
	/**
	 * Vector of parameters needed for calculation of distance.
	 *  NED: Standard deviation of i-th feature (size = feature size)
	 *  QUADRATIC: Perceptual distance between i-th and j-th feature (size = feature size ^ 2)
	 *  MAHALANOBIS: Covariance matrix (size = feature size ^ 2)
	 */
	QVector<double> parameterVector;
	
	DistanceMetric() {}
};

#endif // DISTANCEMETRIC_H
