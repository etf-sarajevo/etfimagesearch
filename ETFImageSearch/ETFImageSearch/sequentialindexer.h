#ifndef SEQUENTIALINDEXER_H
#define SEQUENTIALINDEXER_H

#include "indexer.h"

#include <QMap>

/**
 * Class for sequential searching in index (brute-force search).
 */

class SequentialIndexer : public Indexer
{
public:
	SequentialIndexer(ImageFeatures* alg, QString path);
	
	QString name() { return QString("Sequential indexer"); }
	static QString static_name() { return QString("Sequential indexer"); }
	
	/**
	 * Search image with given filepath in current index (directory must be indexed)
	 * @param filePath Full canonical path to query image
	 * @param nrResults Ignored - since sequential indexer must always go through the whole list, this parameter is useless.
	 * @return Vector of results sorted by distance (from closer to further). Smaller value for distance means that result is closer to query.
	 */
	QVector<Result> search(QString filePath, int nrResults=0);
	
	
protected:
	virtual void clearIndex();
	virtual bool loadIndexSub(QDataStream& indexStream);
	virtual bool writeIndexSub(QDataStream& indexStream);
	virtual bool putInIndex(QString filename, FeatureVector f);

private:
	QMap<QString, FeatureVector> index;
};

#endif // SEQUENTIALINDEXER_H
