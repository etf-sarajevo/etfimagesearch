#include "prtest.h"

#include <QFile>
#include <QDir>
#include <QProgressDialog>
#include <QTime>

#include "qcustomplot.h"

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
	
	QMap<QString, QString> categories;
	while (!file.atEnd()) {
		QString line(file.readLine());
		QStringList parts = line.split(' ', QString::SkipEmptyParts);
		categories[parts[0]] = parts[1];
	}
	file.close();

	QProgressDialog* progressDialog = new QProgressDialog("Testing precision and recall", QString(), 0, categories.size());
	progressDialog->show();
	QTime time = QTime::currentTime();
	
	QVector<double> precision(10), recall(10);
	
	QMapIterator<QString, QString> i(categories);
	while (i.hasNext()) {
		i.next();

		progressDialog->setLabelText(QString("Searching file %1").arg(i.key()));
		progressDialog->setValue(progressDialog->value()+1);

		QVector<Indexer::Result> results = idx->search(path + QDir::separator() + i.key());
		int relevantDocs(0);
		for (int k=0; k<10; k++) {
			for (int j=k*16; j<(k+1)*16; j++) {
				if (categories[results[j].fileName] == i.value()) 
					relevantDocs++;
			}
			precision[k] += qreal(relevantDocs) / ((k+1)*16);
			recall[k] += qreal(relevantDocs) / 100;
		}
	}
	
	for (int i=0; i<10; i++) {
		precision[i] /= categories.size();
		recall[i] /= categories.size();
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
	qcp->xAxis->setRange(0, 0.7);
	qcp->yAxis->setRange(0.3, 0.75);
	qcp->replot();
	
	qcp->show();
	
	return true;
}
