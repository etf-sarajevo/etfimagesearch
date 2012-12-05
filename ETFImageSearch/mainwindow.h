#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>

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
	
private:
	Ui::MainWindow *ui;
	QFileSystemModel *fsm;
	Indexer* idx;
	
};

#endif // MAINWINDOW_H
