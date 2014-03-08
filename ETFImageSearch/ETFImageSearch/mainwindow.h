#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QProgressDialog>
#include <QTime>
#include <QSignalMapper>
#include <QLabel>

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
	// Event for changing URL in location bar
	void on_lineEdit_textChanged(const QString &arg1);
	
	// Event for changing URL by double-clicking on folder
	void on_treeView_doubleClicked(const QModelIndex &index);
	
	// Event for clicking the "Create index" button
	void on_indexButton_clicked();
	
	// Event for clicking on the "Search" button
	void on_searchButton_clicked();
	
	// Event for clicking on the "PR test" button
	void on_prtestButton_clicked();
	
	// Progress bar events for indexing
	void startedIndexing(int count);
	void indexingFile(QString fileName);
	void finishedIndexing();
	
	// Progress bar events for PR test
	void startedPRTest(int count);
	void testingFile(QString fileName);
	void finishedPRTest();
	
	// Event for menu option (changing index)
	void indexChanged(int ordinal);
	
	void on_actionOptimize_triggered();
	
	void on_actionCreate_index_triggered();
	
	void on_actionSearch_image_triggered();
	
	void on_actionPR_test_triggered();
	
private:
	Ui::MainWindow *ui;
	QFileSystemModel *fsm;
	Indexer* idx;
	QProgressDialog* progressDialog;
	QTime time;
	ImageFeatures* currentAlgorithm;
	QSignalMapper* menuMapper;
	
	// Helper function to populate menu with indices in current folder
	int rebuildMenu(const QString &path);
};

#endif // MAINWINDOW_H
