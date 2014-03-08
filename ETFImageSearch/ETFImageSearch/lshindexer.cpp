#include "lshindexer.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <cstdlib>
#include <QDebug>


LSHIndexer::LSHIndexer(ImageFeatures *alg, QString path) :  Indexer(alg, path), 
	bits(20),			// Increased from 12 in LIRe since results were abysmal in the UW dataset, poor P1 / P2 ratio
	projections(150),	// Value from LIRe
	dimensions(640),	// Value from LIRe, theoretical maximum
	w(4)				// Value from LIRe although it doesn't seem to have much effect on retrieval
{
	// Seed the random number generator
	srand(time(NULL));
	
	// Populate bit-sampling lookup table (powers of two)
	lookup[0]=1;
	for (int i(1); i<32; i++)
		lookup[i] = lookup[i-1]*2;
}


void LSHIndexer::clearIndex()
{
	index.clear();
	hashOfHashes.clear();
	hashOfHashes.resize(projections);
	createHash();
}


bool LSHIndexer::loadIndexSub(QDataStream& indexStream)
{
	// TODO implement me
	return false;
}



// This is an attempt to speed up retrieval by "sorting" the index by first hash then second etc.... doesn't work very well (huge decrease in retrieval precision)
bool LSHIndexer::hashsort(const LSHIndexer::IndexEntry& e1, const LSHIndexer::IndexEntry& e2)
{
	for (int i(0); i<e1.hashedFeature.size(); i++) {
		if (e1.hashedFeature[i] < e2.hashedFeature[i]) return true;
		if (e1.hashedFeature[i] > e2.hashedFeature[i]) return false;
	}
	return true;
}

void LSHIndexer::indexPostInit()
{
	//qSort(index.begin(), index.end(), LSHIndexer::hashsort);
}



bool LSHIndexer::writeIndexSub(QDataStream& out)
{
	for (int i(0); i<index.size(); i++) {
		out << index[i].fileName ;
		for (int j(0); j<index[i].feature.size(); j++)
			out << index[i].feature[j];
		for (int j(0); j<index[i].hashedFeature.size(); j++)
			out << index[i].hashedFeature[j];
	}
}



bool LSHIndexer::putInIndex(QString filename, FeatureVector f)
{
	IndexEntry e;
	e.feature = f;
	e.fileName = filename;
	e.hashedFeature = hashUpFV(f);
	index.append(e);
}



// Create a random hash function. This code is almost exactly the same as in LIRe
void LSHIndexer::createHash()
{
	hash.resize(projections);
	for (int i(0); i<projections; i++) {
		hash[i].resize(bits);
		for (int j(0); j<bits; j++) {
			hash[i][j].resize(dimensions);
			for (int k(0); k<dimensions; k++) {
				hash[i][j][k] = (double(rand()) / RAND_MAX) * w - w/2;
			}
		}
	}
}


// Apply hash function to given feature vector. This code is almost exactly the same as in LIRe
// This is where "bit-sampling" comes into place. Value of hashed feature is converted into 1 or 0 with equal probability and this bit
// is then put into appropriate place using lookup table.
QVector<uint> LSHIndexer::hashUpFV(FeatureVector f)
{
	double val;
	QVector<uint> result(projections);
	for (int i(0); i<projections; i++) {
		for (int j(0); j<bits; j++) {
			val = 0;
			for (int k(0); k<f.size(); k++) {
				val += hash[i][j][k] * f[k];
			}
			result[i] += lookup[j] * (val < 0 ? 0 : 1);
		}
	}
	return result;
}



// "Hamming distance" between to features
// It's the number of hashes that are different i.e. number of projections for which the point falls in the same bin
int LSHIndexer::hammingDistance(const IndexEntry &e1, const IndexEntry &e2) {
	int dist(0);
	for (int i(0); i<e1.hashedFeature.size(); i++) {
		if (e1.hashedFeature[i] != e2.hashedFeature[i])
			dist++;
	}
	return dist;
}



QVector<Indexer::Result> LSHIndexer::search(QString filePath, int nrResults)
{
	if (nrResults == 0) nrResults=100; // FIXME
	QVector<Indexer::Result> results;
	QFileInfo fileInfo(filePath);
	
	// Look for file in current index (speeds up PRtest)
	IndexEntry query;
	query.fileName = fileInfo.fileName();
	bool found(false);
	for (int i(0); i<index.size(); i++) {
		if (index[i].fileName == query.fileName) {
			query.feature = index[i].feature;
			query.hashedFeature = index[i].hashedFeature;
			found = true;
			break;
		}
	}
	if (!found) {
		query.feature = getFV(filePath);
		query.hashedFeature = hashUpFV(query.feature);
	}
	
	
	/*
	// Version 1: Checks first hash, then second (if first matches) etc. assuming index is sorted by hashes.
	// Fast, but gives poor retrieval precision

	QVector<int> distance(index.size());
	int minimumDistance(-1), resultsCounter(0);
	int i(0);
	for (; i<index.size(); i++) {
		// This distance calculation formula works on assumption that NN will have most (if not all) of the hashes the same
		// Later experiments revealed that this assumption is wrong
		//
		// So we start from first hash (first projection) and stop as soon as we find a difference
		// Distance: 0 = all hashes same, X = first (size-X) hashes are same
		
		distance[i] = 0;
		for (int j(0); j<query.hashedFeature.size(); j++)
			if (query.hashedFeature[j] != index[i].hashedFeature[j]) {
				distance[i] = query.hashedFeature.size() - j;
				break;
			}
		
		// Find minimum distance and number of results with that minimum distance
		if (distance[i] <= minimumDistance || minimumDistance == -1)
			minimumDistance = distance[i];

		// Since index is sorted by hashes, all elements with same hash as query will be grouped together
		// and after that point with distance=0 the distance will start growing again
		else {
			// Count number of elements with minimum distance
			resultsCounter++;
			if (resultsCounter >= nrResults && distance[i] != minimumDistance)
				break;
		}
	}
	
	// Find start of block of index entries with minimum distance
	i -= resultsCounter;
	if (i==index.size()) i--;
	int start(i), end(i);
	
	// This loop grows the region with minimum distance until nrResults elements are placed into results vector
	while (results.size() < nrResults) {
		Indexer::Result r;
		r.fileName = index[i].fileName;
		r.distance = alg->distance(query.feature, index[i].feature);
		results.push_back(r);
		
		if (start == 0) {
			i = ++end;
		} else if (end == index.size()-1) {
			i = --start;
		} else if (distance[start-1] < distance[end+1]) {
			i = --start;
		} else {
			i = ++end;
		}
	}
	*/
	
	
	
	// Version 2: Calculate hamming distance between hashes, as implied by wikipedia article
	// Almost perfect retrieval, but not much faster than linear (about 50% faster in UW and MIRFLICKR datasets)
	
	// Vector of elements with given distance
	QVector<QVector<int> > distance(projections+1);
	int minDistance(-1), maxDistance(-1), maxes(0);
	for (int i(0); i<index.size(); i++) {
		// Skip same file
		if (index[i].fileName == query.fileName)
			continue;
		int dist = hammingDistance(query, index[i]);
		
		// Append "i" to list of elements with distance "dist"
		distance[dist].append(i);
		
		// This is just statistics for the debugging output below
		if (minDistance==-1 || dist<minDistance)
			minDistance=dist;
		if (dist>maxDistance)
			maxDistance=dist;
		if (dist == maxDistance)
			maxes++;
	}
	
	qDebug() << query.fileName<<"mindist:"<<minDistance<<"maxdist:"<<maxDistance<<"maxes:"<<maxes;
	
	// Now add entries sorted by hamming distance into results vector until it has nrResults elements
	for (int i(minDistance); i<=projections; i++) {
		for (int j(0); j<distance[i].size(); j++) {
			int idx = distance[i][j];
			Indexer::Result r;
			r.fileName = index[idx].fileName;
			// This calculates the real distance instead of "hamming distance"
			r.distance = alg->distance(query.feature, index[idx].feature);
			results.push_back(r);
		}
		if (results.size() >= nrResults) break;
	}
	
	
	// Version 3: Use hash-of-hashes to avoid O(n) dependence, per Slaney et al.
	//
	// TODO - not implemented yet.
	// But it wouldn't work very well anyway because experiments with UW datasets reveal that minDistance on average is 30, maxDistance is always 150, and
	// number of images with maxDistance (maxes) on average is 300 (out of 1100)
	// So we would end up with 150 lists with 50-200 elements each and 800 elements combined and 800 is 0.75*n in the O(n) so how do we find the best 
	// nrResults among those without having O(n) like performance???
	
	qSort(results.begin(), results.end(), Indexer::resultLessThen);
	
	return results;
}


