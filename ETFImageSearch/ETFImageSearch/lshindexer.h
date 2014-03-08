#ifndef LSHINDEXER_H
#define LSHINDEXER_H

#include <vector>
#include <QObject>
#include <QMultiHash>

#include "indexer.h"


/**
 * Indexer based on Locality Sensitive Hashing (LSH).
 * A naive implementation based on Bit sampling for Hamming distance inspired by LIRe.
 * However, it seems that LIRe just dumps the hashes into Lucene which in our tests gave fairly slow retrieval. We need to analize code for LIRe "unit tests".
 * This implementation after all the tweaks either gives a huge error (5-10%) or runs not much faster than sequential :( So I'm missing something.
 *
 * Also: reading from index file not implemented.
 *
 * See:
 *  Slaney, Malcolm, and Michael Casey. "Locality-sensitive hashing for finding nearest neighbors [lecture notes]." Signal Processing Magazine, IEEE 25.2 (2008): 128-131.
 *  http://www.research.yahoo.net/files/Slaney2008(LSHTutorial).pdf
 */

class LSHIndexer : public Indexer
{
	Q_OBJECT
	
public:
	LSHIndexer(ImageFeatures *alg, QString path);
	
	QString name() { return QString("LSH indexer"); }
	static QString static_name() { return QString("LSH indexer"); }
	
	QVector<Result> search(QString filePath, int nrResults=100);
	
	struct IndexEntry {
		FeatureVector feature;
		QVector<uint> hashedFeature;
		QString fileName;
	};
	
protected:
	virtual void clearIndex();
	virtual bool loadIndexSub(QDataStream& indexStream);
	virtual void indexPostInit();
	virtual bool writeIndexSub(QDataStream& indexStream);
	virtual bool putInIndex(QString filename, FeatureVector f);

private:
	// Table for easier bit sampling
	uint lookup[32];
	// Hash function
	QVector<QVector<QVector<double> > > hash;
	// Actual index
	QVector<IndexEntry> index;
	// Hash of hashes for faster querying into index (implemented using QMultiHash instead of manually)
	QVector<QMultiHash<int,int> > hashOfHashes;
	
	// Parameters from paper
	int bits;			// Number of hashes in the k dot product (called "k" in paper). Here it is number of bits because each hash is stored in one bit
	int projections;	// Number of projections for each point (called "L" in paper or "functionBundles" in LIRe)
	int dimensions;		// Number of dimensions of point (size of feature vector)
	double w;			// Width of each bin
	
	/**
	 * Create a random hash function.
	 */
	void createHash();
	
	/**
	 * Apply hash function to given feature vector.
	 */
	QVector<uint> hashUpFV(FeatureVector f);
	
	
	/**
	 * Helper function for sorting index by hashes.
	 */
	static bool hashsort(const IndexEntry& e1, const IndexEntry& e2);
	
	
	
	int hammingDistance(const IndexEntry &e1, const IndexEntry &e2);
	void writeIndexFile();
};

#endif // LSHINDEXER_H
