#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QTextBrowser>

#include "imagefeatures.h"
#include "indexer.h"
#include "prtest.h"

#include "newindexdialog.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), fsm(0), idx(0), progressDialog(0), currentAlgorithm(0), menuMapper(0)
{
	ui->setupUi(this);
	
	ui->lineEdit->setText(QDir::homePath());
	
	fsm = new QFileSystemModel;
	fsm->setRootPath(QDir::homePath());
	ui->treeView->setModel(fsm);
	ui->treeView->setRootIndex(fsm->index(QDir::homePath()));
	
	ui->treeView->setColumnWidth(0, 200);
	
	// Add known algorithms to menu
	/*QAction* rgbHistogramAction = ui->menuAlgorithm->addAction("RGB Histogram");
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
	algorithms->addAction(liuAction);*/
	
	/*currentAlgorithm = new LuEtAl_v2();
	
	//idx = new Indexer(currentAlgorithm, QDir::homePath());
	//idx = new TreeIndexer(currentAlgorithm, QDir::homePath());
	//idx = new LSHIndexer(currentAlgorithm, QDir::homePath());
	idx = new ANNIndexer(currentAlgorithm, QDir::homePath());
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	}*/
	
	// Update buttons and menu
	on_lineEdit_textChanged(QDir::homePath());
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_lineEdit_textChanged(const QString &path)
{
	if (fsm==0) return;

	QFileInfo file(path);
	if (file.exists() && file.isDir()) {
		qDebug() << "Path:"<<path;
		
		ui->lineEdit->setStyleSheet("");
		fsm->setRootPath(path);
		ui->treeView->setRootIndex(fsm->index(path));
		
		int menuCount = rebuildMenu(path);
		
		delete idx;
		delete currentAlgorithm;
		
		// Use last index in the list
		if (menuCount > 0) {
			indexChanged(menuCount-1);
			
		} else {
			// No index in current path
			idx = 0;
			currentAlgorithm = 0;
			ui->searchButton->setEnabled(false);
			ui->prtestButton->setEnabled(false);
		}
		
	} else {
		// Path does not exists
		ui->lineEdit->setStyleSheet("QLineEdit { color: red }");
	}
}


int MainWindow::rebuildMenu(const QString &path)
{
	// Add all known indices to menu
	ui->menuAlgorithm->clear();
	QActionGroup* algorithms = new QActionGroup(this);
	
	QStringList menuItems = Indexer::indicesMenu(path);
	
	delete menuMapper;
	menuMapper = new QSignalMapper(this);
	
	for (int i(0); i<menuItems.size(); i++) {
		QAction* menuAction = ui->menuAlgorithm->addAction(QString("&%1: %2").arg(i+1).arg(menuItems[i]));
		menuAction->setCheckable(true);
		if (i == menuItems.size()-1)
			menuAction->setChecked(true);
		algorithms->addAction(menuAction);
		
		menuMapper->setMapping(menuAction, i);
		connect(menuAction, SIGNAL(triggered()), menuMapper, SLOT(map()));
	}
	
	connect(menuMapper, SIGNAL(mapped(int)), this, SLOT(indexChanged(int)));
	
	if (menuItems.size() > 0)
		ui->menuAlgorithm->setEnabled(true);
	else
		ui->menuAlgorithm->setEnabled(false);
	
	return menuItems.size();
}



void MainWindow::indexChanged(int ordinal)
{
	idx = Indexer::createIndex(fsm->rootPath(), ordinal);
	currentAlgorithm = idx->getAlgorithm();
	if (idx->indexed()) {
		ui->searchButton->setEnabled(true);
		ui->prtestButton->setEnabled(true);
	}
}




void MainWindow::on_treeView_doubleClicked(const QModelIndex &index)
{
	if (!fsm->isDir(index)) return;

	QString path = fsm->filePath(index);
	fsm->setRootPath(path);
	ui->treeView->setRootIndex(index);
	ui->lineEdit->setText(path);
	on_lineEdit_textChanged(path);
}




void MainWindow::on_indexButton_clicked()
{
	NewIndexDialog* nid = new NewIndexDialog(this);
	if (nid->exec() == QDialog::Accepted) {
		ui->menuAlgorithm->setEnabled(true);
		delete idx;
		delete currentAlgorithm;
		
		currentAlgorithm = nid->getFeatures();
		idx = nid->getIndexer(currentAlgorithm, fsm->rootPath());
		
		connect(idx, SIGNAL(startedIndexing(int)), this, SLOT(startedIndexing(int)));
		connect(idx, SIGNAL(indexingFile(QString)), this, SLOT(indexingFile(QString)));
		connect(idx, SIGNAL(finishedIndexing()), this, SLOT(finishedIndexing()));
		
		idx->buildIndex();
		
		rebuildMenu(fsm->rootPath());
	}
}


void MainWindow::on_searchButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, 
		"Open Image for search", ui->lineEdit->text(), "JPEG Files (*.jpg)");
	if (!fileName.isEmpty()) {
		QVector<Indexer::Result> results = idx->search(fileName);
		QString path = fsm->rootPath() + QDir::separator();
		
		QString result = QString("<h1>Top 10 results</h1><p>Search image:<br><img src=\"%1\" border=\"0\" width=\"120\" height=\"80\"></p><hr><table border=\"0\"><tr>").arg(fileName);
		for (int i(0); i<results.size(); i++) {
			//qDebug() << results[i].fileName << results[i].distance;
			if (i<15) {
				result += QString("<td><img src=\"%1\" width=\"120\" height=\"80\" border=\"0\"><br>%2 (%3)</td>\n").arg(path+results[i].fileName).arg(results[i].fileName).arg(results[i].distance);
				if (i%3==2) result += "</tr><tr>";
			}
		}
		result += "</tr></table>";
		
		QTextBrowser* br = new QTextBrowser(0);
		br->resize(400,600);
		br->setHtml(result);
		br->show();
	}
}


void MainWindow::on_prtestButton_clicked()
{
	PRTest prtest(ui->lineEdit->text(), currentAlgorithm, idx);
	if (!prtest.loadCategories()) {
		QTextBrowser* br = new QTextBrowser(0);
		br->setHtml("<h1>Precision-Recall test</h1><p>To run Precision-Recall test on your images, all images in this folder need to be classified into categories. Each image will be searched, and all results within the same category will be considered a &quot;hit&quot;, while other results will be &quot;miss&quot;. You need to create a file named categories.txt in the format:</p><tt>filename category (category category ...)</tt><p>Category is an arbitrary case-sensitive string that will be matched. Each file can be in multiple categories.</p>");
		br->show();
		return;
	}
	
	connect(&prtest, SIGNAL(startedPRTest(int)), this, SLOT(startedPRTest(int)));
	connect(&prtest, SIGNAL(testingFile(QString)), this, SLOT(testingFile(QString)));
	connect(&prtest, SIGNAL(finishedPRTest()), this, SLOT(finishedPRTest()));
	
	prtest.execute();
	
	qDebug() << "MAP ="<<prtest.MAP << "AP16 ="<<prtest.AP16<<"AWP16 ="<<prtest.AWP16<<"ANMRR ="<<prtest.ANMRR;
	
	prtest.showGraph();
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


void MainWindow::startedPRTest(int count)
{
	if (progressDialog) {
		progressDialog->hide();
		progressDialog->close();
		progressDialog->deleteLater();
	}
	progressDialog = new QProgressDialog("Testing precision and recall", QString(), 0, count);
	progressDialog->setAutoReset(false);
	progressDialog->show();
	time = QTime::currentTime();
}

void MainWindow::testingFile(QString fileName)
{
	progressDialog->setLabelText(QString("Searching file %1").arg(fileName));
	progressDialog->setValue(progressDialog->value()+1);
}

void MainWindow::finishedPRTest()
{
	progressDialog->hide();
}

void MainWindow::on_actionOptimize_triggered()
{
	/*QTextEdit* qte = new QTextEdit(0);
	qte->setWindowTitle("Enter variables to optimize");
	qte->show();*/
	
	QStringList vars;
	vars << "blackLower" << "blackUpper";
	
	PRTest prtest(ui->lineEdit->text(), currentAlgorithm, idx);
	if (!prtest.loadCategories()) {
		return;
	}
	
	if (!prtest.optimize(vars)) {
/*		QTextBrowser* br = new QTextBrowser(0);
		br->setHtml("<h1></h1><p>To run Precision-Recall test on your images, all images in this folder need to be classified into categories. Each image will be searched, and all results within the same category will be considered a &quot;hit&quot;, while other results will be &quot;miss&quot;. You need to create a file named categories.txt in the format:</p><tt>filename category (category category ...)</tt><p>Category is an arbitrary case-sensitive string that will be matched. Each file can be in multiple categories.</p>");
		br->show();*/
		return;
	}
	
}

void MainWindow::on_actionCreate_index_triggered()
{
	on_indexButton_clicked();
}

void MainWindow::on_actionSearch_image_triggered()
{
	on_searchButton_clicked();
}

void MainWindow::on_actionPR_test_triggered()
{
	on_prtestButton_clicked();
}
