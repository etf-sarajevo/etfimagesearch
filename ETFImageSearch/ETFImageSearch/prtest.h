#ifndef PRTEST_H
#define PRTEST_H

#include <QString>

#include "searchalgorithm.h"

class PRTest
{
public:
	PRTest(QString path, SearchAlgorithm* alg, Indexer* ind);
	
	bool loadCategories();
	
private:
	QString path;
	SearchAlgorithm* alg;
	Indexer* ind;
};

#endif // PRTEST_H
