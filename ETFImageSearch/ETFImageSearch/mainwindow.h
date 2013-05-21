#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QProgressDialog>
#include <QTime>

#include "indexer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	
private slots:
	void on_lineEdit_textChanged(const QString &arg1);
	
	void on_treeView_doubleClicked(const QModelIndex &index);
	
	void on_indexButton_clicked();
	
	void on_searchButton_clicked();
	
	void on_prtestButton_clicked();
	
	void startedIndexing(int count);
	void indexingFile(QString fileName);
	void finishedIndexing();
	
	void startedPRTest(int count);
	void testingFile(QString fileName);
	void finishedPRTest();
	
	void rgbHistogram();
	void hsvHistogram();
	void yuvHistogram();
	void liuAlgorithm();
	void rgbSplitHistogram();
	
private:
	Ui::MainWindow *ui;
	QFileSystemModel *fsm;
	Indexer* idx;
	QProgressDialog* progressDialog;
	QTime time;
	SearchAlgorithm* currentAlgorithm;
	
};

#endif // MAINWINDOW_H
