#include <QtGui/QApplication>
#include "mainwindow.h"

#include <iostream>
#include <QDebug>

#include "colorhistogram.h"
#include "sequentialindexer.h"
#include "prtest.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
	if (a.arguments().size()>1 && a.arguments()[1] == "-ht") {
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
	}
	
	MainWindow w;
	w.show();
	
	return a.exec();
}
