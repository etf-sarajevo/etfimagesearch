#include "sequentialindexer.h"

#include <QFileInfo>
#include <QImage>
#include <QDebug>


#include "globals.h"


SequentialIndexer::SequentialIndexer(ImageFeatures *alg, QString path) : Indexer(alg, path)
{
}

void SequentialIndexer::clearIndex()
{
	index.clear();
}

bool SequentialIndexer::loadIndexSub(QDataStream& in)
{
	while (!in.atEnd()) {
		QString key;
		FeatureVector vec;
		vec.resize(alg->size());
		in >> key;
		for (int i(0); i<alg->size(); i++)
			vec[i] = readDataType(in);
		index[key] = vec;
	}
	return true;
}

bool SequentialIndexer::writeIndexSub(QDataStream& out)
{
	QMapIterator<QString, FeatureVector> i(index);
	while (i.hasNext()) {
		i.next();
		out << i.key();
		for (int j(0); j<alg->size(); j++)
			writeDataType(out, i.value()[j]);
	}
	return true;
}


bool SequentialIndexer::putInIndex(QString filename, FeatureVector f)
{
	index[filename] = f;
}


// Search image with given filepath in current index (directory must be indexed)
QVector<Indexer::Result> SequentialIndexer::search(QString filePath, int nrResults)
{
	QFileInfo fileInfo(filePath);
	FeatureVector searchVector;

	// Look for file in current index (speeds up PRtest)
	if (index.contains(fileInfo.fileName()))
		searchVector = index[fileInfo.fileName()];
	else
		searchVector = getFV(filePath);
	
	//ImageFeatures::vectorDump(searchVector);
	
	QVector<Indexer::Result> results;
	QMapIterator<QString, FeatureVector> i(index);
	while (i.hasNext()) {
		i.next();
		Indexer::Result r;
		r.fileName = i.key();
		//qDebug() << "Srcing "<<r.fileName;
		if (r.fileName == fileInfo.fileName()) continue;
		r.distance = alg->distance(searchVector, i.value());
		results.push_back(r);
	}
	
	qSort(results.begin(), results.end(), Indexer::resultLessThen);
	
	return results;
}
