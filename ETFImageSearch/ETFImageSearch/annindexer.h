#ifndef ANNINDEXER_H
#define ANNINDEXER_H

#include <ANN/ANN.h>

#include "indexer.h"


/**
 * Class based on libANN (Approximate Nearest Neighbor). Used method is kd-tree with priority search. Results are excellent.
 */

class ANNIndexer : public Indexer
{
	Q_OBJECT
	
public:
	ANNIndexer(ImageFeatures *alg, QString path);
	~ANNIndexer();
	
	QString name() { return QString("ANN indexer"); }
	static QString static_name() { return QString("ANN indexer"); }
	
	QVector<Indexer::Result> search(QString filePath, int nrResults=0);
	
protected:
	virtual void clearIndex();
	virtual bool loadIndexSub(QDataStream& indexStream);
	virtual void indexPostInit();
	virtual bool writeIndexSub(QDataStream& indexStream);
	virtual bool putInIndex(QString filename, FeatureVector f);
	
private:
	QVector<double*> index;
	QVector<QString> indexFileNames;
	ANNkd_tree* tree;
};

#endif // ANNINDEXER_H
