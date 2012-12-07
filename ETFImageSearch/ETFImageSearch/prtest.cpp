#include "prtest.h"

#include <QFile>
#include <QDir>

PRTest::PRTest(QString path, SearchAlgorithm *alg, Indexer *ind) : path(path), alg(alg), ind(ind)
{
	
}

bool PRTest::loadCategories()
{
	QFile file(path + QDir::separator() + "categories.txt");
	if (!file.exists())
		return false;
	
	
	return true;
}
