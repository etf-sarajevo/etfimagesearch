#include "prtest.h"

#include <QFile>
#include <QDir>
#include <QProgressDialog>
#include <QTime>

#include "qcustomplot.h"
#include "liuetal_v2.h"

PRTest::PRTest(QString path, SearchAlgorithm *alg, Indexer *idx) : path(path), alg(alg), idx(idx)
{
	
}

bool PRTest::loadCategories()
{
	QFile file(path + QDir::separator() + "categories.txt");
	if (!file.exists())
		return false;
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	QMap<QString, QStringList> categories;
	while (!file.atEnd()) {
		QString line(file.readLine().trimmed());
		QStringList parts = line.split(' ', QString::SkipEmptyParts);
		QString catFile = parts[0];
		parts.removeFirst();
		if (parts.size() == 0) continue; // skip files that are in no categories
		categories[catFile] = parts;
	}
	file.close();

	QProgressDialog* progressDialog = new QProgressDialog("Testing precision and recall", QString(), 0, categories.size());
	progressDialog->show();
	QTime time = QTime::currentTime();
	
/*	QVector<double> precision(11), recall(11);
	int searchedFiles(0);
	
	QMapIterator<QString, QString> i(categories);
	while (i.hasNext()) {
		i.next();

		progressDialog->setLabelText(QString("Searching file %1").arg(i.key()));
		progressDialog->setValue(progressDialog->value()+1);

		QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key());
		QVector<double> APprecision(results.size());
		QVector<double> AMPprecision(results.size());
		int relevantDocsAP(0);
		double relevantDocsAMP(0);
		for (int k=0; k<results.size(); k++) {
			int wordsFound(0);
			QString cat;
			foreach (cat, i.value()) {
				if (categories[results[k].fileName].contains(cat))
					wordsFound++;
			}
			if (wordsFound>0) relevantDocsAP++;
			relevantDocsAMP += double(wordsFound) / i.value().size();
			
			APprecision[k] = double(relevantDocsAP) / (k+1);
			AMPprecision[k] = relevantDocsAMP / (k+1);
		}
		
		int targetPrecision(10);
		double targetRecall(1), maxAPprecision(0), maxAMPprecision(0);
		for (int k=results.size()-1; k>=0; k--) {
			if (APprecision[k]>maxAPprecision) maxAPprecision=APprecision[k];
			while (APprecision[k]*k / relevantDocsAP < targetRecall) {
				precision[targetPrecision] = maxAPprecision;
				targetPrecision--;
				targetRecall -= 0.1;
			}
		}
		
/*		int relevantDocs(0);
		for (int k=0; k<10; k++) {
			for (int j=k*16; j<(k+1)*16; j++) {
				if (categories[results[j].fileName] == i.value()) 
					relevantDocs++;
			}
			precision[k] += qreal(relevantDocs) / ((k+1)*16);
			recall[k] += qreal(relevantDocs) / 100;
		}*//*
	}
	
	for (int i=0; i<10; i++) {
		precision[i] /= categories.size();
		recall[i] /= categories.size();
	}

	// /home/vedran/mms/etfimagesearch/trunk/wang1000/image.orig
	qDebug() << "MAP10 = " << precision[0];*/
	
	QVector<double> PRGraph(11);
	double AP16(0), AWP16(0), ANMRR(0);
	int counter(0);

	QMapIterator<QString, QStringList> i(categories);
	while (i.hasNext()) {
		i.next();

		progressDialog->setLabelText(QString("Searching file %1").arg(i.key()));
		progressDialog->setValue(progressDialog->value()+1);

		// Create PR graph for file i
		QVector<double> PRGraphI(11);
		QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key());
		
		int relevantDocs(0);
		QVector<double> cumulativeRelevantDocs(results.size());
		double AP16k(0), AWP16k(0);
		
		for (int k=0; k<results.size(); k++) {
			// Is result k relevant?
			int wordsFound(0);
			QString cat;
			foreach (cat, i.value()) {
				if (categories[results[k].fileName].contains(cat))
					wordsFound++;
			}

/*			if (wordsFound>0) {
				qDebug()<<results[k].fileName<<"cat: "<<cat<<" words:" <<categories[results[k].fileName];
			}*/
			
			if (wordsFound>0) relevantDocs++;
			cumulativeRelevantDocs[k] = relevantDocs;
			
			if (k<16 && wordsFound>0) AP16k++;
			if (k<16) AWP16k += double(wordsFound) / i.value().size();
		}
		
		// PR graph is using interpolated precision (IP)
		// IP(i) = max( P(i), P(i+1), ..., P(size) )
		double maxPrecision=0;
		double currentRecall=1;
		int currentPRIndex=10;
		
		for (int k=results.size()-1; k>=0; k--) {
			// Calculate precision and recall for index k
			double precision = double(cumulativeRelevantDocs[k]) / (k+1);
			double recall = double(cumulativeRelevantDocs[k]) / relevantDocs;
			if (precision > maxPrecision) maxPrecision = precision;
			
			// Did we reach the recall bracket?
			if (recall < currentRecall) {
				PRGraphI[currentPRIndex] = maxPrecision;
				currentPRIndex--;
				currentRecall -= 0.1;
				k++;
			}
		}
		
		// Add value for recall==0 if neccessarry
		while (currentPRIndex >= 0) {
			PRGraphI[currentPRIndex--] = maxPrecision;
		}
		
		// Update averages for PRgraph
		for (int i(0); i<11; i++) {
			PRGraph[i] = ( PRGraph[i]*counter + PRGraphI[i] ) / (counter+1);
		}
		
		// Calculate ANMRR
		// Since ground truth is large, we use K=NG*2
		double AVR(0), K(relevantDocs*2);
		for (int k(0); k<results.size(); k++) {
			// Is result k relevant?
			int wordsFound(0);
			QString cat;
			foreach (cat, i.value()) {
				if (categories[results[k].fileName].contains(cat))
					wordsFound++;
			}
			
			if (wordsFound == 0) continue;
			
			// Result is relevant, calculate rank
			if (k > K)
				AVR += K * 1.25;
			else
				AVR += k;
		}
		AVR = AVR / relevantDocs;
		
		double MRR(AVR - 0.5*(1+relevantDocs));
		double NMRR = MRR / (1.25*K - 0.5*(1+relevantDocs));

		if (relevantDocs == 0) continue;
		
		AP16  = (  AP16 * counter + AP16k  / 16 ) / (counter+1);
		AWP16 = ( AWP16 * counter + AWP16k / 16 ) / (counter+1);
		ANMRR = ( ANMRR * counter + NMRR ) / (counter + 1);
		
		counter++;
	}
	
	// AP
	double AP=0;
	for (int i(0); i<11; i++)
		AP += PRGraph[i];
	AP /= 11;
	qDebug() << "AP = " << AP << "AP16 = "<<AP16<<" AWP16 = "<<AWP16 << " ANMRR = "<<ANMRR;
	
	QVector<double> XAxis(11);
	XAxis[0] = 0;
	for (int i(1); i<11; i++)
		XAxis[i] = XAxis[i-1] + 0.1;
	
	progressDialog->hide();
	int passed = time.msecsTo(QTime::currentTime());
	
	QCustomPlot* qcp = new QCustomPlot;
	qcp->setTitle(QString("Precision-Recall graph - %1 s").arg(qreal(passed)/1000));
	qcp->addGraph();
	qcp->graph(0)->setData(XAxis, PRGraph);
	qcp->xAxis->setLabel("Recall");
	qcp->yAxis->setLabel("Precision");
	qcp->xAxis->setRange(0, 1);
	qcp->yAxis->setRange(0, 1);
	qcp->replot();
	
	qcp->show();
	
	return true;
}


bool PRTest::optimize()
{
	QFile file(path + QDir::separator() + "categories.txt");
	if (!file.exists())
		return false;
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	QMap<QString, QString> categories;
	while (!file.atEnd()) {
		QString line(file.readLine());
		QStringList parts = line.split(' ', QString::SkipEmptyParts);
		categories[parts[0]] = parts[1];
	}
	file.close();

	QProgressDialog* progressDialog = new QProgressDialog("Optimizing kME & kD", QString(), 0, 20);
	progressDialog->show();
	QTime time = QTime::currentTime();
	
	QVector<double> precision(20), recall(20);
	
	int k(0);
	for (double kME=0.05; kME<0.15; kME += 0.005) {
		LiuEtAl_v2* liu = (LiuEtAl_v2*) alg;
		liu->setkMEkD(kME, 1-kME);

		progressDialog->setLabelText(QString("Pass %1").arg(k+1));
		progressDialog->setValue(progressDialog->value()+1);
		
		QMapIterator<QString, QString> i(categories);
		while (i.hasNext()) {
			i.next();
		
		
			QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key());
			int relevantDocs(0);
			for (int j=0; j<16; j++) {
				if (categories[results[j].fileName] == i.value()) 
					relevantDocs++;
			}
			precision[k] += qreal(relevantDocs) / 16;
			//recall[k] += qreal(relevantDocs) / 100;
			recall[k] = kME;
		}
		k++;
	}
	
	for (int i=0; i<20; i++) {
		precision[i] /= categories.size();
//		recall[i] /= categories.size();
	}

	// /home/vedran/mms/etfimagesearch/trunk/wang1000/image.orig
	progressDialog->hide();
	int passed = time.msecsTo(QTime::currentTime());
	
	QCustomPlot* qcp = new QCustomPlot;
	qcp->setTitle(QString("Precision-Recall graph - %1 s").arg(qreal(passed)/1000));
	qcp->addGraph();
	qcp->graph(0)->setData(recall, precision);
	qcp->xAxis->setLabel("Recall");
	qcp->yAxis->setLabel("Precision");
	qcp->xAxis->setRange(0.05, 0.15);
	qcp->yAxis->setRange(0.3, 0.8);
	qcp->replot();
	
	qcp->show();
	
	return true;
}


