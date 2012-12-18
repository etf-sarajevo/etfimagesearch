#ifndef INDEXER_H
#define INDEXER_H

#include <QString>
#include <QMap>
#include <QObject>

#include "searchalgorithm.h"

// Derive Indexer from QObject so we could use signals and slots
class Indexer : public QObject
{
	Q_OBJECT
	
public:
	Indexer(SearchAlgorithm* alg, QString path);
	
	void setPath(QString path);
	void setAlgorithm(SearchAlgorithm* alg);
	bool indexed() { return pathIndexed; }
	
	struct Result {
		QString fileName;
		double distance;
	};
	
	QVector<Result> search(QString filePath);
	
	void createIndex();
	
signals:
	void startedIndexing(int count);
	void indexingFile(QString fileName);
	void finishedIndexing();
	
private:
	void loadIndex();
	FeatureVector getFV(QString imagePath);
	
	QString path;
	SearchAlgorithm* alg;
	QMap<QString, FeatureVector> index;
	bool pathIndexed;
	
};

#endif // INDEXER_H
