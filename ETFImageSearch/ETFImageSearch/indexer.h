#ifndef INDEXER_H
#define INDEXER_H

#include <QString>
#include <QMap>

#include "searchalgorithm.h"

class Indexer
{
public:
	Indexer(SearchAlgorithm* alg);
	
	void setPath(QString path);
	void setAlgorithm(SearchAlgorithm* alg);
	bool indexed() { return pathIndexed; }
	
	struct Result {
		QString fileName;
		qreal distance;
	};
	
	QVector<Result> search(QString filePath);
	
	void createIndex();
	
private:
	void loadIndex();
	
	QString path;
	SearchAlgorithm* alg;
	QMap<QString, FeatureVector> index;
	bool pathIndexed;
	
};

#endif // INDEXER_H
