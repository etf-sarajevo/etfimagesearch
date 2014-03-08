#include "treeindexer.h"
#include "colorhistogram.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QDebug>
#include <iostream>
#include <QTime>

#include <cmath>
#include <algorithm>

TreeIndexer::TreeIndexer(ImageFeatures *alg, QString path) : Indexer(alg, path)
{
	initTree(normalTree);
	initTree(antiAliasingTree);
	
	// Determine bitLength
	if (alg->name() == "Color histogram") {
		bitLength = ((ColorHistogram*)alg)->getHistogramQuantization();
	} else {
		switch (alg->dataType()) {
		case ImageFeatures::UINT8:
		case ImageFeatures::INT8:
			bitLength = 8;
			break;
		case ImageFeatures::UINT16:
		case ImageFeatures::INT16:
			bitLength = 16;
			break;
		case ImageFeatures::UINT32:
		case ImageFeatures::INT32:
			bitLength = 32;
			break;
		case ImageFeatures::FLOAT32:
			// FIXME this indexer wont really work with floats...
			bitLength = 32;
			break;
		case ImageFeatures::DOUBLE64:
			// ...or doubles...
			bitLength = 64;
			break;
		}
	}
}



void TreeIndexer::clearIndex()
{
	initTree(normalTree);
	initTree(antiAliasingTree);
}

void TreeIndexer::initTree(MyTree &tree)
{
	tree.clear();
	QVector<TreeNode> empty;
	tree.append(empty);
}



bool TreeIndexer::loadIndexSub(QDataStream& indexStream)
{
	// TODO implement me
	return false;
}

bool TreeIndexer::writeIndexSub(QDataStream& out)
{
	// Output normal tree
	for (int i(0); i<normalTree.size(); i++) {
		for (int j(0); j<normalTree[i].size(); j++) {
			for (int k(0); k<normalTree[i][j].feature.size(); k++)
				writeDataType(out, normalTree[i][j].feature[k]);
			out << normalTree[i][j].fileName << normalTree[i][j].resolution << normalTree[i][j].pos;
		}
	}
	
	// Output AA tree
	for (int i(0); i<antiAliasingTree.size(); i++) {
		for (int j(0); j<antiAliasingTree[i].size(); j++) {
			for (int k(0); k<antiAliasingTree[i][j].feature.size(); k++)
				writeDataType(out, antiAliasingTree[i][j].feature[k]);
			out << antiAliasingTree[i][j].fileName << antiAliasingTree[i][j].resolution << antiAliasingTree[i][j].pos;
		}
	}
}


bool TreeIndexer::putInIndex(QString filename, FeatureVector fv)
{
	addToTree(normalTree, fv, filename);
	antiAlias(fv);
	addToTree(antiAliasingTree, fv, filename);
	
}



void TreeIndexer::antiAlias(FeatureVector &f, bool reverse)
{
	int max(pow(2,bitLength)), shift(pow(2,bitLength-1)); // Optimum value for AA after experiments
	for (int j(0); j<f.size(); j++) {
		if (reverse) {
			f[j] -= shift;
			if (f[j] < 0) f[j] += max;
		} else {
			f[j] += shift;
			if (f[j] >= max) f[j] -= max;
		}
	}
}



void TreeIndexer::reduce(FeatureVector &f, int amount)
{
	for (int i(0); i<f.size(); i++) {
		int x = f[i];
		x = x>>amount;
		f[i] = x;
	}
}




void TreeIndexer::addToTree(MyTree &tree, FeatureVector f, QString fileName)
{
	// Create leaf entry
	TreeNode leaf;
	leaf.feature = f;
	leaf.pos = -1;
	leaf.resolution = bitLength;
	leaf.fileName = fileName;
	
	// Create reduced resolution vectors
	FeatureVector oneBit(f), twoBit(f), fourBit(f);
	reduce(oneBit, bitLength-1);
	reduce(twoBit, bitLength-2);
	reduce(fourBit, bitLength-4);
	
	// Position of 1-bit 2-bit and 4-bit vector in their respective nodes
	int pos1(nodeFindExact(tree[0], oneBit));
	int pos2(-1), pos4(-1);
	
	if (pos1 == -1) {
		// 1-bit vector not found
		
		// Create a new 2-bit node
		QVector<TreeNode> empty;
		tree.append(empty);
		pos1 = tree.size() - 1;
		
		// Add a new entry to 1-bit node
		TreeNode node;
		node.feature = oneBit;
		node.pos = pos1;
		node.resolution = 1;
		tree[0].append(node);
		
		//qDebug()<<"1-bit not found - adding at pos "<<pos1;
		
		pos2 = -1;
	} else {
		// Look for 2-bit vector
		pos1 = tree[0][pos1].pos;
		//qDebug()<< "pos1="<<pos1;
		pos2 = nodeFindExact(tree[pos1], twoBit);
	}

	if (pos2 == -1) {
		// 2-bit vector not found
		
		// Create a new 4-bit node
		QVector<TreeNode> empty;
		tree.append(empty);
		pos2 = tree.size() - 1;
		
		// Add a new entry to 2-bit node
		TreeNode node;
		node.feature = twoBit;
		node.pos = pos2;
		node.resolution = 2;
		tree[pos1].append(node);
		
		//qDebug()<<"2-bit not found - adding at pos "<<pos2;
		
		pos4 = -1;
	}
	else {
		// Look for 4-bit vector
		pos2 = tree[pos1][pos2].pos;
		//qDebug()<< "pos2="<<pos2;
		pos4 = nodeFindExact(tree[pos2], fourBit);
	}

	if (pos4 == -1) {
		// 4-bit vector not found
		
		// Create a new leaf node
		QVector<TreeNode> empty;
		tree.append(empty);
		pos4 = tree.size()-1;
		
		// Add new entry to 4-bit node
		TreeNode node;
		node.feature = fourBit;
		node.pos = pos4;
		node.resolution = 4;
		tree[pos2].append(node);
		
		//qDebug()<<"4-bit not found - adding at pos "<<pos4;
	} else {
		pos4 = tree[pos2][pos4].pos;
		//qDebug() << "pos4="<<pos4;
	}
	
	// Add leaf entry to leaf node
	tree[pos4].append(leaf);
}



int TreeIndexer::nodeFindExact(QVector<TreeNode> &node, FeatureVector f)
{
	for (int i(0); i<node.size(); i++) {
		bool found(true);
		for (int j(0); j<f.size(); j++) {
			if (f[j] != node[i].feature[j]) {
				found = false;
				break;
			}
		}
		if (found)
			return i;
	}
	return -1;
}



// std::unique requires that operator== is defined for datatype
bool operator==(const Indexer::Result& r1, const Indexer::Result& r2)
{
	return (r1.fileName==r2.fileName);
}

QVector<Indexer::Result> TreeIndexer::search(QString filePath, int nrResults)
{
	std::vector<Indexer::Result> results, resultsAA, resultsJoined;
	QFileInfo fileInfo(filePath);

	// Look for file in current index (speeds up PRtest)
	FeatureVector searchVector;
	bool found(false);
	for (int i(0); i<normalTree.size(); i++) {
		for (int j(0); j<normalTree[i].size(); j++) {
			if (normalTree[i][j].resolution <= 4) break;
			if (normalTree[i][j].fileName == fileInfo.fileName()) {
				searchVector = normalTree[i][j].feature;
				found = true;
				break;
			}
		}
		if (found) break;
	}
	
	if (!found)
		searchVector = getFV(filePath);
	
	// Search both trees recursively
	searchRecursive(normalTree, 0, results, fileInfo.fileName(), searchVector, nrResults);
	antiAlias(searchVector);
	searchRecursive(antiAliasingTree, 0, resultsAA, fileInfo.fileName(), searchVector, nrResults, true);
	
	// This is probably the most efficient method to join two vectors while sorting and removing duplicates
	// Don't have time to research and profiling shows that this isn't critical
	resultsJoined.reserve( results.size() + resultsAA.size() );
	resultsJoined.insert( resultsJoined.end(), results.begin(), results.end() );
	resultsJoined.insert( resultsJoined.end(), resultsAA.begin(), resultsAA.end() );
	std::sort(resultsJoined.begin(), resultsJoined.end(), Indexer::resultLessThen);
	resultsJoined.erase( unique( resultsJoined.begin(), resultsJoined.end() ), resultsJoined.end() );

	QVector<Indexer::Result> vec = QVector<Indexer::Result>::fromStdVector(resultsJoined);
	
	return vec;
}


// To reduce memory usage and avoid unneccessary searches, this helper struct contains just an index to node item and distance
struct SubTreeResult {
	int index;
	double distance;
};

// Now we must create another helper function for sorting
bool subTreeResultLessThen(const SubTreeResult& r1, const SubTreeResult& r2)
{
	return r1.distance < r2.distance;
}


void TreeIndexer::searchRecursive(QVector<QVector<TreeNode> > &tree, int nodeIndex, std::vector<Indexer::Result> &results, QString fileName, FeatureVector query, int nrResults, bool aa)
{
	//qDebug() << "Node index: "<<nodeIndex;
	
	// Found enough results?
	if (nrResults != 0 && results.size() >= nrResults)
		return;
	
	// Is node empty?
	if (tree[nodeIndex].size() == 0)
		return;
	
	// First we prepare a vector of all subtrees below current node
	std::vector<SubTreeResult> subTreeResults;
	
	// Prepare a version of query vector scaled to the same resolution as current node
	FeatureVector scaledQuery(query);
	// All items in given node should have the same resolution so lets just use the first one
	int nodeResolution = tree[nodeIndex][0].resolution;
	if (bitLength != nodeResolution)
		reduce(scaledQuery, bitLength - nodeResolution);
	
	// Put all items in node into vector. Calculate distance between scaled query and node item
	for (int i(0); i<tree[nodeIndex].size(); i++) {
		SubTreeResult r;
		r.index = i;
		r.distance = alg->distance(scaledQuery, tree[nodeIndex][i].feature);
		subTreeResults.push_back(r);
	}
	
	// Sort vector by scaled distance.
	// Note that this doesn't guarantee that the ordering of higher resolution results will be the same. So the results vector must
	// be sorted again in search() function.
	std::sort(subTreeResults.begin(), subTreeResults.end(), subTreeResultLessThen);
	
	for (int i(0); i<subTreeResults.size(); i++) {
		// Alias for shorter typing
		const TreeNode& currItem ( tree[ nodeIndex ][ subTreeResults[i].index ] );
		
		// Skip result that has the same filename as query
		if (currItem.fileName == fileName) continue;
		
		// Is this a leaf (non-leaf items have empty filename)?
		if (currItem.fileName != QString()) {
			// It's a leaf, copy into results vector
			Indexer::Result r;
			r.distance = subTreeResults[i].distance;
			r.fileName = currItem.fileName;
			
			// If this is AA tree we must recalculate distance after de-AA'ing
			if (aa) {
				FeatureVector q1(query), q2(currItem.feature);
				antiAlias(q1, /*reverse=*/true);
				antiAlias(q2, /*reverse=*/true);
				r.distance = alg->distance(q1, q2);
			}
			
			results.push_back(r);
			//qDebug() << "Adding: "<<tree[subtree][subTreeResults[i].index].fileName;
		} else {
			// This is not a leaf, recurse below it
			searchRecursive(tree, currItem.pos, results, fileName, query, nrResults, aa);
		}
		
		// Stop early if we have enough results
		if (nrResults != 0 && results.size() >= nrResults)
			return;
	}
}


