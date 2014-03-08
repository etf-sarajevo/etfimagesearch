#ifndef TREEINDEXER_H
#define TREEINDEXER_H

#include "indexer.h"

#include <vector>
#include <QDebug>


/**
 * This class implements a simple metric B-tree based on bit coding with an anti-aliasing tree for error reduction.
 * In experiments this tree has a very fast performance (slightly slower then ANN), while error is about 0,5% (with ANN it's practically zero).
 * Also: reading from index file not implemented, doesn't work with float values.
 * So use ANN :) this is just a fun experiment in implementing a tree.
 */

class TreeIndexer : public Indexer
{
	Q_OBJECT
	
public:
	TreeIndexer(ImageFeatures *alg, QString path);
	
	QString name() { return QString("Tree indexer"); }
	static QString static_name() { return QString("Tree indexer"); }
	
	QVector<Result> search(QString filePath, int nrResults=100);
	
	void stats() {
		qDebug() << "getFV:"<<(double(timers[0])/1109)<<"ms fileinfo:"<<(double(timers[1])/1109)<<"ms recurse:"<<(double(timers[2])/1109)<<"ms sort:"<<(double(timers[3])/1109)<<"ms qvec from std:"<<(double(timers[4])/1109);
	}
	
protected:
	virtual void clearIndex();
	virtual bool loadIndexSub(QDataStream& indexStream);
	virtual bool writeIndexSub(QDataStream& indexStream);
	virtual bool putInIndex(QString filename, FeatureVector fv);
	
private:
	struct TreeNode {
		FeatureVector feature;
		QString fileName;
		int resolution;
		int pos;
	};
	
	typedef QVector<QVector<TreeNode> > MyTree;

	MyTree normalTree;
	MyTree antiAliasingTree;
	/**
	 * Number of bits per bin (range of values in feature vector, for rescaling).
	 */
	int bitLength;
	
	
	// HELPER FUNCTIONS
	
	/**
	 * Clear the tree and add an empty root node.
	 */
	void initTree(MyTree &tree);
	
	/**
	 * This method does a right shift of all entries in the feature vector by amount bits. It's assumed that entries are integers.
	 * @param f Reference to the FeatureVector (will be changed).
	 * @param amount Number of bits for right shift.
	 */
	void reduce(FeatureVector& f, int amount);
	
	/**
	 * Increment all items in feature vector so that they can be placed into antiAliasing tree.
	 * @param f Reference to the FeatureVector (will be changed).
	 * @param reverse Do the operation in reverse (when querying AA tree)
	 */
	void antiAlias(FeatureVector& f, bool reverse=false);
	
	/**
	 * Add one item to given tree.
	 */
	void addToTree(MyTree &tree, FeatureVector f, QString fileName);
	
	
	/**
	 * Looks for feature vector in one node.
	 * @return Index of vector f in node n, or -1 if not found
	 */
	int nodeFindExact(QVector<TreeNode>& node, FeatureVector f);
	
	
	/**
	 * Helper recursive function for search.
	 * @param tree Which tree (normal or AA)
	 * @param nodeIndex Index to the current node to search
	 * @param results Reference to vector where results will be appended
	 * @param fileName File name of query image (so it can be ommitted from results)
	 * @param query Full resolution feature vector that is searched in tree
	 * @param nrResults Expected number of results. Recursion will terminate when size of the results vector is this or greater.
	 * @param aa Flag for AA (whether antiAlias function should be called in reverse before calculating distance)
	 */
	void searchRecursive(QVector<QVector<TreeNode> > &tree, int nodeIndex, std::vector<Indexer::Result> &results, QString fileName, FeatureVector query, int nrResults=0, bool aa=false);
	
	
	int timers[100];
};

#endif // TREEINDEXER_H
