#include "indexer.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QDebug>

Indexer::Indexer(SearchAlgorithm *alg) : path(""), alg(alg), pathIndexed(false)
{
}

void Indexer::setPath(QString path)
{
	this->path = path;
	qDebug()<<"setPath";
	loadIndex();
}

void Indexer::setAlgorithm(SearchAlgorithm* alg)
{
	this->alg = alg;
	loadIndex();
}

void Indexer::loadIndex()
{
	index.clear();
	pathIndexed = false;
	
	QFile file(path + QDir::separator() + "etfis_" + alg->name() + ".idx");
	if (file.exists()) {
		file.open(QIODevice::ReadOnly);
		QDataStream in(&file);
		while (!in.atEnd()) {
/*			QString key;
			char* data;
			uint datalen;
			in >> key;
			in.readBytes(data, datalen);
			FeatureVector vec;
			vec.features.append(data, datalen);
			delete[] data;*/
			QString key;
			FeatureVector vec;
			in >> key >> vec.features;
			index[key] = vec;
		}
		file.close();
		pathIndexed = true;
	}
}

bool resultLessThen(const Indexer::Result& r1, const Indexer::Result& r2)
{
	return r1.distance < r2.distance;
}

QVector<Indexer::Result> Indexer::search(QString filePath)
{
	FeatureVector searchVector;
	if (alg->isDct()) {
		// yadayada
	} else {
		QImage image(filePath);
		searchVector = alg->extractFeatures(image.constBits(), image.byteCount());
	}
	
	QVector<Indexer::Result> results;
	QMapIterator<QString, FeatureVector> i(index);
	while (i.hasNext()) {
		i.next();
		Indexer::Result r;
		r.fileName = i.key();
		r.distance = alg->distance(searchVector, i.value());
		results.push_back(r);
	}
	
	qSort(results.begin(), results.end(), resultLessThen);
	
	return results;
}


void Indexer::createIndex()
{
	if (path == "") return;
	
	index.clear();
	
	// Index files in dir
	QDir dir(path);
	QFileInfoList list = dir.entryInfoList();
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		if (!fileInfo.isFile()) continue;
		if (fileInfo.completeSuffix().toLower() != "jpg") continue;
		
		qDebug() << "Indexing "<<fileInfo.fileName();

		if (alg->isDct()) {
			// yadayada
		} else {
			QImage image(fileInfo.filePath());
			index[fileInfo.fileName()] = alg->extractFeatures(image.constBits(), image.byteCount());
		}
	}
	
	// Serialize index to an index file
	QFile file(path + QDir::separator() + "etfis_" + alg->name() + ".idx");
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);
	QMapIterator<QString, FeatureVector> i(index);
	while (i.hasNext()) {
		i.next();
		out << i.key() << i.value().features;
	}
	file.close();
	pathIndexed = true;
}

