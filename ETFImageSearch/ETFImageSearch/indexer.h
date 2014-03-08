#ifndef INDEXER_H
#define INDEXER_H

#include <QString>
#include <QList>
#include <QVector>
#include <QObject>
#include <QDataStream>

#include "imagefeatures.h"

/**
 * Base class for Indexer data structures
 */

// Derive Indexer from QObject so we could use signals and slots
class Indexer : public QObject
{
	Q_OBJECT
	
public:
	Indexer(ImageFeatures* alg, QString path);
	virtual ~Indexer() {}
	
	/**
	 * Name of this index structure (used as ID in index files, should be unique)
	 */
	virtual QString name()=0;
	
	/**
	 * Set current working directory to path
	 */
	void setPath(QString path);
	
	/**
	 * Set feature extraction algorithm to alg
	 */
	void setAlgorithm(ImageFeatures* alg);
	ImageFeatures* getAlgorithm() { return alg; }
	
	
	/**
	 * Subclasses can reimplement these methods to support writing some params into index file as configuration
	 */
	virtual void setParams(QString params) {}
	virtual QString getParams() { return QString(); }
	
	
	/**
	 * Is there an index for current path?
	 */
	bool indexed() { return pathIndexed; }
	
	struct Result {
		QString fileName;
		double distance;
	};
	
	/**
	 * Search image with given filepath in current index (directory must be indexed)
	 * @param filePath Full canonical path to query image
	 * @param nrResults Number of results expected. Actual list of results may be larger. 0 means retrieve all results.
	 * @return Vector of results sorted by distance (from closer to further). Smaller value for distance means that result is closer to query.
	 */
	virtual QVector<Result> search(QString filePath, int nrResults=0)=0;
	
	/**
	 * Create index of images in current path (calculate feature vectors and store them in index structure)
	 */
	void buildIndex();
	
	virtual void stats() {}
	
	
	/**
	 * Load existing image index file.
	 * Subclasses should not reimplement loadIndex but loadIndexSub
	 */
	bool loadIndex(QString indexFile);
	
	/**
	 * Write index data into a file (possibly create a new index file).
	 * Subclasses should not reimplement writeIndex but writeIndexSub
	 */
	void writeIndex();
	
	
	/**
	 * List all indices available for given path.
	 * @return A list of information in order: filename, indexer name, indexer params, features name, features params
	 */
	static QList<QStringList> listIndices(QString path);
	
	/**
	 * A nice human-readable menu of indices available in given path.
	 */
	static QStringList indicesMenu(QString path);
	
	/**
	 * Create a new instance of Indexer class based on path and ordinal number of index in the list returned by listindices
	 * @param path Full canonical path to directory of images
	 * @param ordinal Ordinal number in list of indices returned by listIndices for path
	 */
	static Indexer* createIndex(QString path, int ordinal);
	
	static Indexer* factory(QString name, ImageFeatures* alg, QString path);
	
signals:
	void startedIndexing(int count);
	void indexingFile(QString fileName);
	void finishedIndexing();
	
protected:
	
	/**
	 * Call ImageFeatures class to extract feature vector from image with given path
	 * This method also contains logic for interfacing with libjpeg for DCTFeatures
	 */
	FeatureVector getFV(QString imagePath);
	
	/**
	 * Helper function for sorting search results by distance
	 */
	static bool resultLessThen(const Result& r1, const Result& r2);
	
	
	/**
	 * Empty index
	 */
	virtual void clearIndex()=0;
	
	/**
	 * Deserialize index from stream into memory.
	 * Subclasses that reimplement this method receive file already opened and header skipped
	 */
	virtual bool loadIndexSub(QDataStream& indexStream)=0;
	
	/**
	 * Reads one item from stream in features datatype format and converts it to double.
	 * Convenience function to call from loadIndexSub to avoid ugly code duplication.
	 */
	double readDataType(QDataStream& indexStream);
	
	/**
	 * Serialize index from memory into stream
	 * Subclasses that reimplement this method receive file already opened and header written
	 */
	virtual bool writeIndexSub(QDataStream& indexStream)=0;
	
	/**
	 * Writes one item converting it from double to features datatype.
	 * Convenience function to call from writeIndexSub to avoid ugly code duplication.
	 */
	void writeDataType(QDataStream& indexStream, double item);
	
	/**
	 * Put this key-value pair into index
	 */
	virtual bool putInIndex(QString filename, FeatureVector f)=0;
	
	/**
	 * This method will be called after loading index into memory or from feature vectors to do any postprocessing on the datastructure
	 */
	virtual void indexPostInit() {}
	
	QString path;
	ImageFeatures* alg;
	bool pathIndexed;
	QString indexFile;
};

#endif // INDEXER_H
