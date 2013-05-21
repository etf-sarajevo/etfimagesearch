#ifndef PRTEST_H
#define PRTEST_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QObject>
#include <QMap>

#include "searchalgorithm.h"
#include "indexer.h"

class PRTest : public QObject
{
	Q_OBJECT
	
public:
	PRTest(QString path, SearchAlgorithm* alg, Indexer* idx);
	
	bool loadCategories();
	void execute();
	bool optimize();
	void showGraph();
	
	double AP, AP16, AWP16, ANMRR;
	
signals:
	void startedPRTest(int count);
	void testingFile(QString fileName);
	void finishedPRTest();

private:
	QString path;
	SearchAlgorithm* alg;
	Indexer* idx;
	QMap<QString, QStringList> categories;
	QVector<double> PRGraph, XAxis;
	int timeElapsed;
};

#endif // PRTEST_H
