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
	
	QVector<double> precision(10), recall(10);
	
	QMapIterator<QString, QStringList> i(categories);
	int count(0);
	while (i.hasNext()) {
		i.next();

		progressDialog->setLabelText(QString("Searching file %1").arg(i.key()));
		progressDialog->setValue(progressDialog->value()+1);

		QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key());
		double relevantDocs(0);
		for (int k=0; k<10; k++) {
			for (int j=k*16; j<(k+1)*16; j++) {
				QString cat;
				int wordsFound(0);
				foreach (cat, i.value()) {
					if (categories[results[j].fileName].contains(cat)) {
						wordsFound++;
					}
				}
				relevantDocs += double(wordsFound) / i.value().size();
				//if (found) break;
			}
			precision[k] += qreal(relevantDocs) / ((k+1)*16);
			recall[k] += qreal(relevantDocs) / 100;
		}
		count++;
		//if (count == 2) break;
	}
	
	for (int i=0; i<10; i++) {
		precision[i] /= count; //categories.size();
		recall[i] /= count; //categories.size();
	}
	
	qDebug() << precision[0];

	progressDialog->hide();
	int passed = time.msecsTo(QTime::currentTime());
	
	QCustomPlot* qcp = new QCustomPlot;
	qcp->setTitle(QString("Precision-Recall graph - %1 s").arg(qreal(passed)/1000));
	qcp->addGraph();
	qcp->graph(0)->setData(recall, precision);
	qcp->xAxis->setLabel("Recall");
	qcp->yAxis->setLabel("Precision");
	qcp->xAxis->setRange(0, 0.7);
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
	// /home/vedran/cbir/etfimagesearch/trunk/wang1000/image.orig
	// /home/vedran/cbir/etfimagesearch/trunk/mirflickr-25000/images
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


