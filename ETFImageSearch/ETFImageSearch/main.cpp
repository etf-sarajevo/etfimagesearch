#include <QtGui/QApplication>
#include "mainwindow.h"

#include <iostream>
#include <stdexcept>
#include <QDebug>
#include <execinfo.h>

#include "colorhistogram.h"
#include "sequentialindexer.h"
#include "prtest.h"

#include <sys/time.h>

double getTime() 
{
	static timeval t;
	timeval newtime;
	double elapsedTime;
	
	gettimeofday(&newtime, NULL);
	elapsedTime = (newtime.tv_sec - t.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (newtime.tv_usec - t.tv_usec) / 1000.0;   // us to ms
	t = newtime;
	return elapsedTime;
}

void usage() {
	std::cerr << "ETFImageSearch v2014-03-13\nby: Vedran Ljubovic (c) ETF 2012-2014\nLicensed under GNU GPL v3\n\n";
	std::cerr << "Usage:\n  -help\n  -path\n  -feature\n  -indexer\n  -optimize\n  -trainingset\n  -reuseindex\n  -params\n  -query\n  -verbose\n";
	return;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
	// No CLI arguments
	if (argc == 1) {
		try{
			MainWindow w;
			w.show();
			return a.exec();
		} catch(std::out_of_range e) {
			std::cout<<"Out-of-range: "<<e.what()<<std::endl;
			return -1;
		} catch(const char e[]) {
			std::cout<<"Exception: "<<e<<std::endl;
			return -1;
		}
	}
	
	// Common stuff
	QString imagePath(QDir::currentPath()), featureName, indexer("Sequential indexer"), trainingSet("training_set.txt"), params, queryImage;
	QStringList optimizeVars;
	int useIndex(-1);
	int verbosityLevel(0);
	QStringList qargv = a.arguments(); // Shortcut for argv as QStringList
	
	// Arguments parser
	for (int i(1); i<argc; i++) {
		if (qargv[i] == "-help") {
			usage();
			return 0;
		}
		
		else if (qargv[i] == "-path") {
			if (i+1 == argc) {
				std::cerr << "Error: -path requires an argument: path to images\n\n";
				usage();
				return -1;
			}
			imagePath = qargv[i+1];
			i++;
			//std::cerr << "Path " << imagePath.constData()-> << "\n\n";
			//qDebug() << "Path" << imagePath;
		}
		
		else if (qargv[i] == "-feature") {
			if (i+1 == argc) {
				std::cerr << "Error: -feature requires an argument: name of feature\n\n";
				usage();
				return -1;
			}
			featureName = qargv[i+1];
			i++;
			//qDebug() << "Ft" << featureName;
		}
		
		else if (qargv[i] == "-indexer") {
			if (i+1 == argc) {
				std::cerr << "Error: -indexer requires an argument: name of indexer\n\n";
				usage();
				return -1;
			}
			indexer = qargv[i+1];
			i++;
		}
		
		else if (qargv[i] == "-optimize") {
			if (i+1 == argc) {
				std::cerr << "Error: -optimize requires an argument: comma separated list of variables to optimize\n\n";
				usage();
				return -1;
			}
			optimizeVars = qargv[i+1].split(',');
			i++;
			//qDebug() << "Optimize" << optimizeVars;
		}
		
		else if (qargv[i] == "-trainingset") {
			if (i+1 == argc) {
				std::cerr << "Error: -trainingset requires an argument: filename of list of files for optimizer training\n\n";
				usage();
				return -1;
			}
			trainingSet = qargv[i+1];
			i++;
		}
		
		else if (qargv[i] == "-reuseindex") {
			int idx(0);
			// Check if second parameter is index number
			if (i+1 < argc) {
				bool ok;
				idx = qargv[i+1].toInt(&ok);
				if (!ok) idx=0;
				else i++;
			}
			useIndex = idx;
		}
		
		else if (qargv[i] == "-params") {
			if (i+1 == argc) {
				std::cerr << "Error: -params requires an argument: semicolon separated list of params\n\n";
				usage();
				return -1;
			}
			params = qargv[i+1];
			i++;
		}
		
		else if (qargv[i] == "-query") {
			if (i+1 == argc) {
				std::cerr << "Error: -query requires an argument: path to query image\n\n";
				usage();
				return -1;
			}
			queryImage = qargv[i+1];
			i++;
		}
		
		else if (qargv[i] == "-verbose") {
			verbosityLevel++; // TODO: add more levels
		}

		else {
			std::cerr << "Error: argument not recognized: "<<argv[i]<<"\n\n";
			usage();
			return -1;
		}
	}
	
	// No feature name given - nothing to do
	if (useIndex == -1 && featureName.isEmpty()) {
		std::cerr << "Error: you must specify -featurename with that option\n\n";
		usage();
		return -1;
	}
	
	// Start work
	Indexer* idx;
	ImageFeatures* feature;
	
	try {
	if (useIndex == -1) {
		feature = ImageFeatures::factory(featureName);
		if (!params.isEmpty()) {
			// Check for invalid params
			try {
				feature->setParams(params);
			} catch (QString s) {
				qDebug() << s;
				return -1;
			} catch (const char e[]) {
				std::cerr << e << std::endl;
				return -1;
			}
		}

		idx = Indexer::factory(indexer, feature, imagePath);
		if (verbosityLevel>0) std::cerr << "Building first index...\n";
		
		double t1 = getTime();
		idx->buildIndex();
		double t2 = getTime();
		if (verbosityLevel>0) std::cerr << "Indexing time: " << t2 << " ms\n";
	} else {
		idx = Indexer::createIndex(imagePath, useIndex);
		feature = idx->getAlgorithm();
	}
	
	// Query a single image
	if (!queryImage.isEmpty()) {
		QVector<Indexer::Result> results = idx->search(queryImage);
		for (int i(0); i<results.size(); i++)
			std::cout << results[i].fileName.toAscii().data() << "   " << results[i].distance << std::endl;
		return 0;
	}
	
	PRTest prtest(imagePath, feature, idx);
	if (!prtest.loadCategories()) {
		std::cerr << "Error: failed to load categories.\nDo you have a file named categories.txt in work path?\n";
		return -2;
	}
	
	// If not optimize, do a simple PR test
	if (optimizeVars.isEmpty()) {
		if (verbosityLevel>0) std::cerr << "Starting PRtest...\n";
		double t1 = getTime();
		prtest.execute();
		double t2 = getTime();
		std::cout << "MAP = "<<prtest.MAP << " AP16 = "<<prtest.AP16<<" AWP16 = "<<prtest.AWP16<<" ANMRR = "<<prtest.ANMRR<<std::endl;
		if (verbosityLevel>0) std::cerr << "PRtest time: " << t2 << " ms\n";
	}
	
	// Do optimize run
	else {
		// TODO: use signals-slots to read values from PRtest class and output while using verbosity level etc.
		prtest.optimize(optimizeVars, trainingSet);
	}
	
	} catch(const char e[]) {
		qDebug() << "EXCEPTION: "<<e;
	} catch(QString e) {
		qDebug() << "EXCEPTION: "<<e;
	}
	
	return 0;
	
	/*if (a.arguments().size()>1 && a.arguments()[1] == "-ht") {
		QFile input(a.arguments()[2]);
		if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
			std::cerr << "File htscript.txt not found" << std::endl;
			return -1;
		}
		
		QFile output(a.arguments()[3]);
		if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
			std::cerr << "Failed to open results.txt for writing" << std::endl;
			return -2;
		}
		
		while (!input.atEnd()) {
			QStringList parts = QString(input.readLine()).split(',');
			ColorHistogram alg;

			alg.setColorModel(Pixel::fromString(parts[1]));
			
			alg.setColorQuantization(parts[2].toInt(), parts[3].toInt(), parts[4].toInt());
			
			if (parts[5]=="split") alg.setHistogramType(ColorHistogram::SPLITHISTOGRAM);
			if (parts[5]=="combined") alg.setHistogramType(ColorHistogram::COMBINEDHISTOGRAM);
			
			if (parts[6]=="none") alg.setHistogramNormalization(ColorHistogram::NO_NORMALIZATION);
			if (parts[6]=="max") alg.setHistogramNormalization(ColorHistogram::MAX_NORMALIZATION);
			if (parts[6]=="both") alg.setHistogramNormalization(ColorHistogram::BOTH_NORMALIZATION);
			
			alg.setHistogramQuantization(parts[7].toInt());
			
			if (parts[8].toInt() == 1) alg.setHistogramCumulative(true); else alg.setHistogramCumulative(false);
			
			alg.setDistanceMetric(DistanceMetric::fromString(parts[9]));
			
			qDebug() << "Indexing"<<parts[0]<<"...";
			SequentialIndexer idx(&alg, parts[10]);
			idx.buildIndex();
			
			qDebug() << "PR test"<<parts[0]<<"...";
			PRTest prtest(parts[10], &alg, &idx);
			if (!prtest.loadCategories()) {
				std::cerr << "Categories file not found" << std::endl;
				return -2;
			}
			prtest.execute();
			QString result = QString("%1,%2,%3,%4,%5\n").arg(parts[0]).arg(prtest.AP).arg(prtest.AP16).arg(prtest.AWP16).arg(prtest.ANMRR);
			output.write(result.toAscii().constData());
			output.flush();
		}
		input.close();
		output.close();
		qDebug() << "Finished";
		return 0;
	}*/
	
	return a.exec();
}
