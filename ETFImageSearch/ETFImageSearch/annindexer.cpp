#include "annindexer.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

ANNIndexer::ANNIndexer(ImageFeatures *alg, QString path) : Indexer(alg, path), tree(0)
{
}


ANNIndexer::~ANNIndexer() 
{
	clearIndex();
	delete tree;
	annClose();
}

void ANNIndexer::clearIndex()
{
	for (int i(0); i<index.size(); i++)
		delete[] index[i];
	index.clear();
}


bool ANNIndexer::loadIndexSub(QDataStream& in)
{
	while (!in.atEnd()) {
		QString name;
		in >> name;
		indexFileNames.append(name);
		
		ANNpoint point = new ANNcoord[alg->size()];
		for (int i(0); i<alg->size(); i++)
			point[i] = readDataType(in);
		index.append(point);
	}
}

void ANNIndexer::indexPostInit()
{
	// After loading stuff into index we recreate ANNkd_tree class
	delete tree;
	tree = new ANNkd_tree(index.data(), index.size(), alg->size());
}


bool ANNIndexer::writeIndexSub(QDataStream& out)
{
	for (int i(0); i<index.size(); i++) {
		out << indexFileNames[i];
		for (int j(0); j<alg->size(); j++)
			writeDataType(out, index[i][j]);
	}
}


bool ANNIndexer::putInIndex(QString filename, FeatureVector f)
{
	indexFileNames.append(filename);
	
	ANNpoint point = new ANNcoord[alg->size()];
	for (int i(0); i<alg->size(); i++)
		point[i] = f[i];
	index.append(point);
}



QVector<Indexer::Result> ANNIndexer::search(QString filePath, int nrResults)
{
	if (nrResults == 0) nrResults = index.size();
	QVector<Indexer::Result> results;
	QFileInfo fileInfo(filePath);
	
	// Look for file in current index (speeds up PRtest)
	ANNpoint query;			// Structure used by ANN search
	FeatureVector fvQuery;	// Structure used by alg->distance
	
	bool fromIndex(false);
	if (indexFileNames.contains(fileInfo.fileName())) {
		// File found in index
		int idx = indexFileNames.indexOf(fileInfo.fileName());
		query = index[idx];
		
		// Copy from query to fvQuery
		fvQuery.resize(alg->size());
		for (int i(0); i<alg->size(); i++)
			fvQuery[i] = query[i];
		fromIndex = true;

	} else {
		// File not found in index
		fvQuery = getFV(filePath);
		
		// Copy from fvQuery to query
		query = new ANNcoord[alg->size()];
		for (int i(0); i<alg->size(); i++) {
			query[i] = fvQuery[i];
			//qDebug() << fvQuery[i];
		}
	}
	
	ANNidxArray  annResults = new ANNidx[nrResults];
	ANNdistArray annDists   = new ANNdist[nrResults];
	
	tree->annkPriSearch(query, nrResults, annResults, annDists, 5);
	
	// Copy from annResults to results
	FeatureVector f;
	f.resize(alg->size());
	for (int i(0); i<nrResults; i++) {
		Indexer::Result r;
		r.fileName = indexFileNames[annResults[i]];
		if (r.fileName == fileInfo.fileName()) // Skip query image in results
			continue;
		for (int j(0); j<alg->size(); j++)
			f[j] = index[annResults[i]][j];
		r.distance = alg->distance(fvQuery, f);
		
		/*qDebug() << "name"<<r.fileName<<"dist"<<r.distance;
		if (r.fileName == "995.jpg") for (int j(0); j<alg->size(); j++)
			qDebug() << f[j] << fvQuery[j];*/
		results.push_back(r);
	}
	
	qSort(results.begin(), results.end(), Indexer::resultLessThen);

	// Free up memory
	if (!fromIndex)
		delete[] query;
	delete[] annResults;
	delete[] annDists;
	
	return results;
}

