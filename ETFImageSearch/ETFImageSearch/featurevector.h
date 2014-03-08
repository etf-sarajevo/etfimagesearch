#ifndef FEATUREVECTOR_H
#define FEATUREVECTOR_H

#include <QVector>

/** 
 * Vector of features representing one image
 */
//typedef QVector<double> FeatureVector;

class FeatureVector {
private:
	double fv[10000]; // Can't be bigger than this
	int m_size;
public:
	FeatureVector() {
		m_size=0;
	}
	FeatureVector(int size) : m_size(size) {
		for (int i(0); i<size; i++) fv[i]=0;
	}
	void append(double el) { fv[m_size++]=el; }
	void push_back(double el) { fv[m_size++]=el; }
	void resize(int size) { m_size=size; }
	double& operator[](int i) {
		if (i<0 || i>=m_size) throw "Exception";
		return fv[i];
	}
	double operator[](int i) const {
		if (i<0 || i>=m_size) throw "Exception";
		return fv[i];
	}
	int size() const { return m_size; }
	void fill(int size, double value) {
		if (m_size<size) m_size=size;
		for (int i(0); i<size; i++) fv[i]=value;
	}
};

// Integer variant

/*class FeatureVector {
private:
	int fv[10000]; // Can't be bigger than this
	int m_size;
public:
	FeatureVector() {
		m_size=0;
	}
	FeatureVector(int size) : m_size(size) {
		for (int i(0); i<size; i++) fv[i]=0;
	}
	void append(int el) { fv[m_size++]=el; }
	void push_back(int el) { fv[m_size++]=el; }
	void resize(int size) { m_size=size; }
	int& operator[](int i) {
		if (i<0 || i>=m_size) throw "Exception";
		return fv[i];
	}
	int operator[](int i) const {
		if (i<0 || i>=m_size) throw "Exception";
		return fv[i];
	}
	int size() const { return m_size; }
	void fill(int size, int value) {
		if (m_size<size) m_size=size;
		for (int i(0); i<size; i++) fv[i]=value;
	}
};*/




#endif // FEATUREVECTOR_H