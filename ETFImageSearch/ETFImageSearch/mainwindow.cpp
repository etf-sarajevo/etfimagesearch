#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QTextBrowser>

#include "rgbhistogram.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), fsm(0)
{
	ui->setupUi(this);
	
	ui->lineEdit->setText(QDir::homePath());
	
	fsm = new QFileSystemModel;
	fsm->setRootPath(QDir::homePath());
	ui->treeView->setModel(fsm);
	ui->treeView->setRootIndex(fsm->index(QDir::homePath()));
	
	idx = new Indexer(new RGBHistogram(256));
	idx->setPath(QDir::homePath());
	if (idx->indexed())
		ui->searchButton->setEnabled(true);
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
		if (idx->indexed())
			ui->searchButton->setEnabled(true);
		else
			ui->searchButton->setEnabled(false);
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
	if (idx->indexed())
		ui->searchButton->setEnabled(true);
	else
		ui->searchButton->setEnabled(false);
}

void MainWindow::on_indexButton_clicked()
{
	idx->createIndex();
	ui->searchButton->setEnabled(true);
}

void MainWindow::on_searchButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, 
		"Open Image for search", ui->lineEdit->text(), "JPEG Files (*.jpg)");
	if (!fileName.isEmpty()) {
		QVector<Indexer::Result> results = idx->search(fileName);
		qDebug() << "TOP 10 RESULTS:";
		QString result("TOP 10 RESULTS:\n");
		for (int i(0); i<results.size(); i++) {
			qDebug() << results[i].fileName << results[i].distance;
			if (i<10) result += QString("%1 %2\n").arg(results[i].fileName).arg(results[i].distance);
		}
		QTextBrowser* br = new QTextBrowser(0);
		br->setPlainText(result);
		br->show();
	}
}

