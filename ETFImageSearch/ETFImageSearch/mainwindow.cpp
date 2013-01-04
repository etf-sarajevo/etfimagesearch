#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QTextBrowser>

#include "rgbhistogram.h"
#include "hsvhistogram.h"
#include "yuvhistogram.h"
#include "rgbsplithistogram.h"
#include "liuetal_v2.h"
#include "prtest.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), fsm(0), progressDialog(0)
{
	ui->setupUi(this);
	
	ui->lineEdit->setText(QDir::homePath());
	
	fsm = new QFileSystemModel;
	fsm->setRootPath(QDir::homePath());
	ui->treeView->setModel(fsm);
	ui->treeView->setRootIndex(fsm->index(QDir::homePath()));
	
	ui->treeView->setColumnWidth(0, 200);
	
	// Add known algorithms to menu
	QAction* rgbHistogramAction = ui->menuAlgorithm->addAction("RGB Histogram");
	rgbHistogramAction->setCheckable(true);
	connect (rgbHistogramAction, SIGNAL(triggered()), this, SLOT(rgbHistogram()));
	
	QAction* hsvHistogramAction = ui->menuAlgorithm->addAction("HSV Histogram");
	hsvHistogramAction->setCheckable(true);
	connect (hsvHistogramAction, SIGNAL(triggered()), this, SLOT(hsvHistogram()));
	
	QAction* yuvHistogramAction = ui->menuAlgorithm->addAction("YUV Histogram");
	yuvHistogramAction->setCheckable(true);
	connect (yuvHistogramAction, SIGNAL(triggered()), this, SLOT(yuvHistogram()));

	QAction* rgbSplitHistogramAction = ui->menuAlgorithm->addAction("RGB Split Histogram");
	rgbSplitHistogramAction->setCheckable(true);
	connect (rgbSplitHistogramAction, SIGNAL(triggered()), this, SLOT(rgbSplitHistogram()));
	
	QAction* liuAction = ui->menuAlgorithm->addAction("Liu et al. v2");
	liuAction->setCheckable(true);
	liuAction->setChecked(true);
	connect (liuAction, SIGNAL(triggered()), this, SLOT(liuAlgorithm()));

	QActionGroup* algorithms = new QActionGroup(this);
	algorithms->addAction(rgbHistogramAction);
	algorithms->addAction(hsvHistogramAction);
	algorithms->addAction(yuvHistogramAction);
	algorithms->addAction(rgbSplitHistogramAction);
	algorithms->addAction(liuAction);
	
	currentAlgorithm = new LiuEtAl_v2();
	
	idx = new Indexer(currentAlgorithm, QDir::homePath());
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_lineEdit_textChanged(const QString &path)
{
	if (fsm==0) return;

	qDebug() << path;
	QFileInfo file(path);
	if (file.exists() && file.isDir()) {
		ui->lineEdit->setStyleSheet("");
		fsm->setRootPath(path);
		ui->treeView->setRootIndex(fsm->index(path));
		idx->setPath(path);
		if (idx->indexed()) {
			ui->searchButton->setEnabled(true);
			ui->prtestButton->setEnabled(true);
		} else {
			ui->searchButton->setEnabled(false);
			ui->prtestButton->setEnabled(false);
		}
	} else {
		ui->lineEdit->setStyleSheet("QLineEdit { color: red }");
	}
}

void MainWindow::on_treeView_doubleClicked(const QModelIndex &index)
{
	if (!fsm->isDir(index)) return;

	QString path = fsm->filePath(index);
	fsm->setRootPath(path);
	ui->treeView->setRootIndex(index);
	ui->lineEdit->setText(path);
	idx->setPath(path);
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	} else {
		ui->searchButton->setEnabled(false);
		ui->prtestButton->setEnabled(false);
	}
}

void MainWindow::on_indexButton_clicked()
{
	connect(idx, SIGNAL(startedIndexing(int)), this, SLOT(startedIndexing(int)));
	connect(idx, SIGNAL(indexingFile(QString)), this, SLOT(indexingFile(QString)));
	connect(idx, SIGNAL(finishedIndexing()), this, SLOT(finishedIndexing()));
	
	idx->createIndex();
}

void MainWindow::on_searchButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, 
		"Open Image for search", ui->lineEdit->text(), "JPEG Files (*.jpg)");
	if (!fileName.isEmpty()) {
		QVector<Indexer::Result> results = idx->search(fileName);
		QString result("TOP 10 RESULTS:\n");
		//qDebug() << "TOP 10 RESULTS:";
		for (int i(0); i<results.size(); i++) {
			//qDebug() << results[i].fileName << results[i].distance;
			if (i<10) result += QString("%1 %2\n").arg(results[i].fileName).arg(results[i].distance);
		}
		QTextBrowser* br = new QTextBrowser(0);
		br->setPlainText(result);
		br->show();
	}
}


void MainWindow::on_prtestButton_clicked()
{
	PRTest prtest(ui->lineEdit->text(), currentAlgorithm, idx);
	if (!prtest.loadCategories()) {
//	if (!prtest.optimize()) {
		QTextBrowser* br = new QTextBrowser(0);
		br->setHtml("<h1>Precision-Recall test</h1><p>To run Precision-Recall test on your images, all images in this folder need to be classified into categories. Each image will be searched, and all results within the same category will be considered a &quot;hit&quot;, while other results will be &quot;miss&quot;. You need to create a file named categories.txt in the format:</p><tt>filename category (category category ...)</tt><p>Category is an arbitrary case-sensitive string that will be matched. Each file can be in multiple categories.</p>");
		br->show();
	}
}


void MainWindow::startedIndexing(int count)
{
	if (progressDialog) {
		progressDialog->hide();
		progressDialog->close();
		progressDialog->deleteLater();
	}
	progressDialog = new QProgressDialog("Indexing files", QString(), 0, count);
	progressDialog->setAutoReset(false);
	progressDialog->show();
	time = QTime::currentTime();
}

void MainWindow::indexingFile(QString fileName)
{
	progressDialog->setLabelText(QString("Indexing file %1").arg(fileName));
	progressDialog->setValue(progressDialog->value()+1);
}

void MainWindow::finishedIndexing()
{
	ui->searchButton->setEnabled(true);
	ui->prtestButton->setEnabled(true);
	progressDialog->setValue(progressDialog->value()+1);
	int passed = time.msecsTo(QTime::currentTime());
	progressDialog->setLabelText(QString("Finished indexing. Total time: %1 s").arg(qreal(passed)/1000));
	progressDialog->setCancelButtonText("&Close");
}

void MainWindow::rgbHistogram()
{
	delete currentAlgorithm;
	currentAlgorithm = new RGBHistogram(3, 3, 3);
	idx->setAlgorithm(currentAlgorithm);
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	} else {
		ui->searchButton->setEnabled(false);
		ui->prtestButton->setEnabled(false);
	}
}

void MainWindow::rgbSplitHistogram()
{
	delete currentAlgorithm;
	currentAlgorithm = new RGBSplitHistogram(4, 4, 4);
	idx->setAlgorithm(currentAlgorithm);
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	} else {
		ui->searchButton->setEnabled(false);
		ui->prtestButton->setEnabled(false);
	}
}

void MainWindow::hsvHistogram()
{
	delete currentAlgorithm;
	currentAlgorithm = new HSVHistogram(4, 2, 2);
	idx->setAlgorithm(currentAlgorithm);
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	} else {
		ui->searchButton->setEnabled(false);
		ui->prtestButton->setEnabled(false);
	}
}

void MainWindow::yuvHistogram()
{
	delete currentAlgorithm;
	currentAlgorithm = new YUVHistogram(2,3,3);
	idx->setAlgorithm(currentAlgorithm);
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	} else {
		ui->searchButton->setEnabled(false);
		ui->prtestButton->setEnabled(false);
	}
}

void MainWindow::liuAlgorithm()
{
	delete currentAlgorithm;
	currentAlgorithm = new LiuEtAl_v2();
	idx->setAlgorithm(currentAlgorithm);
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	} else {
		ui->searchButton->setEnabled(false);
		ui->prtestButton->setEnabled(false);
	}
}
