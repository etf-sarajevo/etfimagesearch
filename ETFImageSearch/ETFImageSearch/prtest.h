#ifndef PRTEST_H
#define PRTEST_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QObject>
#include <QMap>

#include "imagefeatures.h"
#include "indexer.h"

class PRTest : public QObject
{
	Q_OBJECT
	
public:
	PRTest(QString path, ImageFeatures* alg, Indexer* idx);
	
	bool loadCategories();
	void execute();
	bool optimize(QStringList variables);
	void showGraph();
	
	double AP, AP16, AWP16, ANMRR, MAP;
	
signals:
	void startedPRTest(int count);
	void testingFile(QString fileName);
	void finishedPRTest();

private:
	QString path;
	ImageFeatures* alg;
	Indexer* idx;
	QMap<QString, QStringList> categories;
	QVector<double> PRGraph, XAxis;
	int timeElapsed;
};

#endif // PRTEST_H
