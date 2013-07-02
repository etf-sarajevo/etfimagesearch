#include <QtGui/QApplication>
#include "mainwindow.h"

#include <iostream>
#include <QDebug>

#include "colorhistogram.h"
#include "indexer.h"
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

			if (parts[1]=="RGB") alg.setColorModel(ColorHistogram::RGB);
			if (parts[1]=="YUV") alg.setColorModel(ColorHistogram::YUV);
			if (parts[1]=="YIQ") alg.setColorModel(ColorHistogram::YIQ);
			if (parts[1]=="HSV") alg.setColorModel(ColorHistogram::HSV);
			if (parts[1]=="HSL") alg.setColorModel(ColorHistogram::HSL);
			if (parts[1]=="LAB") alg.setColorModel(ColorHistogram::LAB);
			if (parts[1]=="LUV") alg.setColorModel(ColorHistogram::LUV);
			
			alg.setColorQuantization(parts[2].toInt(), parts[3].toInt(), parts[4].toInt());
			
			if (parts[5]=="split") alg.setHistogramType(ColorHistogram::SPLITHISTOGRAM);
			if (parts[5]=="combined") alg.setHistogramType(ColorHistogram::COMBINEDHISTOGRAM);
			
			if (parts[6]=="none") alg.setHistogramNormalization(ColorHistogram::NO_NORMALIZATION);
			if (parts[6]=="max") alg.setHistogramNormalization(ColorHistogram::MAX_NORMALIZATION);
			if (parts[6]=="both") alg.setHistogramNormalization(ColorHistogram::BOTH_NORMALIZATION);
			
			alg.setHistogramQuantization(parts[7].toInt());
			
			if (parts[8].toInt() == 1) alg.setHistogramCumulative(true); else alg.setHistogramCumulative(false);
			
			enum DistanceMetric { EUCLIDEAN, MATSUSHITA, BRAY_CURTIS, MANHATTAN, SOERGEL, BHATTACHARYA, CHI_SQUARE, CANBERRA, HIST_INT, JSD, ANGULAR, CHORD, WAVE_HEDGES, WED, K_S, KUIPER, MEAN };
			
			if (parts[9]=="EUCLIDEAN") alg.setDistanceMetric(ColorHistogram::EUCLIDEAN);
			if (parts[9]=="MATSUSHITA") alg.setDistanceMetric(ColorHistogram::MATSUSHITA);
			if (parts[9]=="BRAY_CURTIS") alg.setDistanceMetric(ColorHistogram::BRAY_CURTIS);
			if (parts[9]=="MANHATTAN") alg.setDistanceMetric(ColorHistogram::MANHATTAN);
			if (parts[9]=="SOERGEL") alg.setDistanceMetric(ColorHistogram::SOERGEL);
			if (parts[9]=="BHATTACHARYA") alg.setDistanceMetric(ColorHistogram::BHATTACHARYA);
			if (parts[9]=="CHI_SQUARE") alg.setDistanceMetric(ColorHistogram::CHI_SQUARE);
			if (parts[9]=="CANBERRA") alg.setDistanceMetric(ColorHistogram::CANBERRA);
			if (parts[9]=="HIST_INT") alg.setDistanceMetric(ColorHistogram::HIST_INT);
			if (parts[9]=="JSD") alg.setDistanceMetric(ColorHistogram::JSD);
			if (parts[9]=="ANGULAR") alg.setDistanceMetric(ColorHistogram::ANGULAR);
			if (parts[9]=="CHORD") alg.setDistanceMetric(ColorHistogram::CHORD);
			if (parts[9]=="WAVE_HEDGES") alg.setDistanceMetric(ColorHistogram::WAVE_HEDGES);
			if (parts[9]=="WED") alg.setDistanceMetric(ColorHistogram::WED);
			if (parts[9]=="K_S") alg.setDistanceMetric(ColorHistogram::K_S);
			if (parts[9]=="KUIPER") alg.setDistanceMetric(ColorHistogram::KUIPER);
			if (parts[9]=="MEAN") alg.setDistanceMetric(ColorHistogram::MEAN);
			
			qDebug() << "Indexing"<<parts[0]<<"...";
			Indexer idx(&alg, parts[10]);
			idx.createIndex();
			
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
