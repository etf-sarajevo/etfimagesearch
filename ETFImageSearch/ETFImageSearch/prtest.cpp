#include "prtest.h"

#include <QFile>
#include <QDir>
#include <QProgressDialog>
#include <QTime>

#include "qcustomplot.h"
#include "luetal_v2.h"

PRTest::PRTest(QString path, ImageFeatures *alg, Indexer *idx) : AP(0), AP16(0), AWP16(0), ANMRR(0), 
	path(path), alg(alg), idx(idx), timeElapsed(0)
{
	PRGraph.resize(11);
	XAxis.resize(11);
}

bool PRTest::loadCategories()
{
	QFile file(path + QDir::separator() + "categories.txt");
	if (!file.exists())
		return false;
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	int i(0);
	while (!file.atEnd()) {
		QString line(file.readLine().trimmed());
		QStringList parts = line.split(' ', QString::SkipEmptyParts);
		if (parts.size() == 0) continue; // skip files that are in no categories
		QString catFile = parts[0];
		parts.removeFirst();
		i++;
		categories[catFile] = parts;
	}
	file.close();
	return true;
}

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
		
*//*		int relevantDocs(0);
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


void PRTest::execute()
{
	emit startedPRTest(categories.size());
	QTime time = QTime::currentTime();
	
	AP16=0; AWP16=0; ANMRR=0; MAP=0;
	int counter(0);

	QMapIterator<QString, QStringList> i(categories);
	while (i.hasNext()) {
		/*for (int k(0); k<100; k++) {
			i.next();
			if (!i.hasNext()) break;
		}
		if (!i.hasNext()) break;*/
		i.next();

		emit testingFile(i.key());
		QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key(), 17);

		// Create PR graph for file i
		QVector<double> PRGraphI(11);
		
		int relevantDocs(0);
		QVector<double> cumulativeRelevantDocs(results.size());
		double AP16k(0), AWP16k(0), MAPk(0);
		
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
			
			if (wordsFound>0) {
				relevantDocs++;
				MAPk += double(relevantDocs) / (k+1);
			}
			cumulativeRelevantDocs[k] = relevantDocs;
			
			if (k<16 && wordsFound>0) AP16k++;
			if (k<16) AWP16k += double(wordsFound) / i.value().size();
		}
		
		// Finish calculation of MAP
		MAPk /= relevantDocs;
		
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
		for (int k(0); k<11; k++) {
			PRGraph[k] = ( PRGraph[k]*counter + PRGraphI[k] ) / (counter+1);
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
		MAP   = (   MAP * counter + MAPk ) / (counter + 1);
		//qDebug() << "img ="<<i.key()<<"AP16k =" << AP16k << "AP16 ="<<AP16;
		//qDebug() << "img = "<<i.key()<<"MAPk = "<<MAPk<<" MAP = "<<MAP;
		
		counter++;
	}
	
	// AP
	AP=0;
	for (int i(0); i<11; i++)
		AP += PRGraph[i];
	AP /= 11;
	
	XAxis[0] = 0;
	for (int i(1); i<11; i++)
		XAxis[i] = XAxis[i-1] + 0.1;
	
	emit finishedPRTest();
	timeElapsed = time.msecsTo(QTime::currentTime());
	
	idx->stats();
}

void PRTest::showGraph()
{
	QCustomPlot* qcp = new QCustomPlot;
	qcp->setTitle(QString("Precision-Recall graph - %1 s").arg(double(timeElapsed)/1000));
	qcp->addGraph();
	qcp->graph(0)->setData(XAxis, PRGraph);
	qcp->xAxis->setLabel("Recall");
	qcp->yAxis->setLabel("Precision");
	qcp->xAxis->setRange(0, 1);
	qcp->yAxis->setRange(0, 1);
	qcp->replot();
	
	qcp->show();
}


bool PRTest::optimize(QStringList variables, QString trainingSetFilename)
{
	QFile file(path + QDir::separator() + trainingSetFilename);
	if (!file.exists())
		return false;
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	if (variables.size() == 0) {
		qDebug() << "Nothing to optimize";
		return false;
	}
	
	QString var;
	QVector<ImageFeatures::Variable> vars(alg->getAllVariables()); // Shortcut
	QVector<int> varIndices;
	foreach (var, variables) {
		bool found(false);
		for (int i(0); i<vars.size(); i++) {
			if (vars[i].name == var) {
				found=true;
				varIndices.append(i);
			}
		}
		if (!found) {
			qDebug() << "Variable"<<var<<"passed but not found in algorithm " << alg->name();
			return false;
		}
	}
	
	// Read in the training set
	int i(0);
	QMap<QString, QStringList> trainingSet;
	while (!file.atEnd()) {
		QString line(file.readLine().trimmed());
		QStringList parts = line.split(' ', QString::SkipEmptyParts);
		if (parts.size() == 0) continue; // skip files that are in no categories
		QString catFile = parts[0];
		parts.removeFirst();
		i++;
		trainingSet[catFile] = parts;
	}
	file.close();
	
	// Number of passes
	int currentPass(0), totalPasses(0);
	for (int i(0); i<varIndices.size(); i++) {
		int idx=varIndices[i];
		totalPasses += int((vars[idx].max - vars[idx].min) / vars[idx].step + 0.5);
		vars[idx].value = vars[idx].min;
		alg->setVariable(vars[idx].name, vars[idx].value);
	}
	
	// Setup progress dialog
	emit startedOptimize(totalPasses);
	// PRTest class should be headless - no dialog here
	//QProgressDialog* progressDialog = new QProgressDialog(QString("Optimizing variables for %1").arg(alg->name()), QString(), 0, totalPasses);
	//progressDialog->show();
	QTime time = QTime::currentTime(); // unused! do something with time
	
	// Go through all possible variable values
	bool done(false);
	while (!done) {
		currentPass++;
		//progressDialog->setLabelText(QString("Pass %1").arg(currentPass));
		//progressDialog->setValue(progressDialog->value()+1);
		emit optimizePass(currentPass);
		
		// Output
		QString output = QString("Pass %1 ").arg(currentPass);
		for (int i(0); i<varIndices.size(); i++) {
			int idx=varIndices[i];
			output += QString("%1=%2 ").arg(vars[idx].name).arg(vars[idx].value);
		}
		qDebug() << output;

		// Indexer
		qDebug() << "Indexing...";
		idx->buildIndex();
		qDebug() << "Calculating ANMRR...";

		// We will optimize for ANMRR
		int counter(0);
		ANMRR = 0;
		QMapIterator<QString, QStringList> i(trainingSet);
		while (i.hasNext()) {
			i.next();
	
			QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key(), 17);
			int relevantDocs(0);
			
			
			// This code is ugly since we copypaste from prtest...
			QString res;
			for (int k=0; k<results.size(); k++) {
				res += results[k].fileName + " ";
				// Is result k relevant?
				int wordsFound(0);
				QString cat;
				foreach (cat, i.value()) {
					if (categories[results[k].fileName].contains(cat))
						wordsFound++;
				}
				
				if (wordsFound>0)
					relevantDocs++;
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
			
			ANMRR = ( ANMRR * counter + NMRR ) / (counter + 1);
			counter++;
		}
		
		qDebug() << "ANMRR: "<<ANMRR;
		
		// Next optimize values
		for (int i(0); i<varIndices.size(); i++) {
			int idx=varIndices[i];
			
			// Try incrementing value by step
			for(;;) {
				vars[idx].value = vars[idx].value + vars[idx].step;
				
				// If greater than max, set to min so the next variable gets incremented
				if (vars[idx].value > vars[idx].max) {
					vars[idx].value = vars[idx].min;
					break;
				}
				try {
					alg->setVariable(vars[idx].name, vars[idx].value);
					break;
				} catch(...) {}
			}
			
			// If value is min, that means we must increment the next variable
			if (vars[idx].value != vars[idx].min) break;

			// This is the last variable, test is done
			if (i == vars.size()-1) done=true;
		}
	}
	emit finishedOptimize();
	
	return true;
}


