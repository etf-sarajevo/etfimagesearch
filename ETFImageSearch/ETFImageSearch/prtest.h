#ifndef PRTEST_H
#define PRTEST_H

#include <QString>

#include "searchalgorithm.h"
#include "indexer.h"

class PRTest
{
public:
	PRTest(QString path, SearchAlgorithm* alg, Indexer* idx);
	
	bool loadCategories();
	
private:
	QString path;
	SearchAlgorithm* alg;
	Indexer* idx;
};

#endif // PRTEST_H
