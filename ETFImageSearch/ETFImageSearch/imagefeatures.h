#ifndef IMAGEFEATURES_H
#define IMAGEFEATURES_H

#include <QVector>
#include <QString>
#include <QMap>
#include <QVariant>
#include <vector>

#include "featurevector.h"

#define REGISTER_CLASS


/**
 * Base class for algorithms for extracting features from image
 */

class ImageFeatures
{
public:
	ImageFeatures();
	
	/**
	 * Name of this feature extraction method (used as ID in index files, should be unique)
	 */
	virtual QString name()=0;
	
	/**
	 * Number of elements in feature vector 
	 */
	virtual int size()=0;
	
	enum DataType {
		UINT8, INT8, UINT16, INT16, UINT32, INT32, FLOAT32, DOUBLE64
	};
	
	/**
	 * Data type for elements of feature vector. Used by indexer to optimize memory usage (internally feature vector is always double).
	 * There is no point to use values with greater granularity because of memory alignment it would be expensive.
	 */
	virtual DataType dataType()=0;
	
	
	/**
	 * Subclasses can reimplement these methods to support writing some params into index file as configuration
	 */
	virtual void setParams(QString params) {}
	virtual QString getParams() { return QString(); }
	
	/**
	 * Extract features from image.
	 * @param imageData Raw bitmap data in ARGB 32-bit format, as returned by QImage::bits(). Size of received data is width*height*4 bytes. Ordering is scan order (left-to-right, top-to-bottom), little-endian (BGRA).
	 * @param width Number of pixels in image row.
	 * @param height Number of rows in image.
	 */
	virtual FeatureVector extractFeatures(const uchar* imageData, int width, int height)=0;
	
	/**
	 * Indexer uses this method to detect DCTFeatures class (see below).
	 */
	virtual bool isDct() { return false; }
	
	/**
	 * Calculate distance between two feature vectors.
	 * This may be some specific algorithm for the actual features or it may be one of standard distance metrics.
	 */
	virtual double distance(FeatureVector f1, FeatureVector f2)=0;
	
	// Obsolete?
	virtual void setAverage(FeatureVector f) {}
	
	bool debug;
	
	static ImageFeatures* factory(QString name);
	
	
	// Some debugging functions
	static void vectorDump(QVector<double> vector);
	static void vectorDump(std::vector<double> vector);
	static void vectorDump(std::vector<int> vector);
	
	
	
	// Variables of feature that can be set from outside
	// Feature constructor should define the list of variables and set them to their optimal values
	struct Variable {
		QString name;
		double value, min, max, step;
		Variable(QString name, double value, double min, double max, double step) : name(name), value(value), min(min), max(max), step(step) {}
		Variable() : name(""), value(0), min(0), max(0), step(0) {}
	};
	
protected:
	// Since features access variable values a big number of times, we provide this accellerated model
	double variableValues[100];
	QVector<Variable> variables;

	// Method for derived classes to define new variables
	void addVariable(Variable v) {
		for (int i(0); i<variables.size(); i++) {
			if (v.name == variables[i].name) {
				variables[i] = v;
				variableValues[i] = v.value;
				return;
			}
		}
		if (variables.size() >= 100)
			throw "Too many variables";
		variableValues[variables.size()] = v.value;
		variables.append(v);
	}
	
public:
	Variable getVariable(QString name) const {
		Variable v;
		foreach (v, variables)
			if (v.name == name) return v;
		return v;
	}
	// Features may override this method to throw exception in case of invalid combination
	virtual void setVariable(QString name, double value) {
		Variable v;
		for (int i(0); i<variables.size(); i++)
			if (variables[i].name == name) { 
				variables[i].value = value;
				variableValues[i] = value;
				return;
			}
		throw QString("Unknown variable %1").arg(name);
	}
	QVector<Variable> getAllVariables() const {
		return variables;
	}
};



/**
 * Base class for algorithms that work with DCT coefficients.
 * DCTFeatures dont use method extractFeatures but has separate method processBlock that will be called once per each 8x8 block.
 */

class DCTFeatures : public ImageFeatures
{
public:
	DCTFeatures();
	
	
	/**
	 * Method name (used as ID in index files, should be unique)
	 */
	virtual QString name()=0; 
	
	/**
	 * Indexer uses this method to detect DCTFeatures class
	 */
	bool isDct() { return true; }
	
	/**
	 * This method will be called once before processing each image
	 */
	virtual void init()=0;
	
	/**
	 * This method will be called once for each 8x8 DCT block.
	 * Note that if chroma subsampling is used, this method will be called 4 times for component 0 then once for components 1 and 2 (last two 
	 * represent a subsampled 16x16 block)
	 * @param block Pointer to an array of 64 DCT coefficients in standard scanning order (not zig-zag order)
	 * @param component ID of JPEG color component (0 = Y', 1 = Cb, 2 = Cr)
	 */
	virtual void processBlock(short int* block, int component)=0; 
	
	/**
	 * This method will be called once after image processing is finished. 
	 * Use this to calculate various statistics.
	 */
	virtual FeatureVector calculateVector()=0; 
	
	/**
	 * Since libjpeg is C not C++ we must use this trick to call object method from libjpeg.
	 */
	static void processBlockWrapper(void* object, short int* block, int component) {
		DCTFeatures* myself = (DCTFeatures*) object;
		myself->processBlock(block, component);
	}
	
	/**
	 * Calculate distance between two feature vectors.
	 * This may be some specific algorithm for the actual features or it may be one of standard distance metrics.
	 */
	virtual double distance(FeatureVector f1, FeatureVector f2)=0;
	
private:
	// Disable this method
	FeatureVector extractFeatures(const uchar*, int, int) { return FeatureVector(); }
};

#endif // IMAGEFEATURES_H
