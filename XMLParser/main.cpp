#include <QtGui/QApplication>
#include "xmlparser.h"
#include "myfiledialog.h"
#include <iostream>
#include <QMessageBox>
using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);/*
    MainWindow w;
    w.show();*/

      MyFileDialog my;//Create dialog

      QString name=my.openFile();//Open dialog, and chose file. We get file path and file name as result

      cout<<name.toUtf8().constData()<<"Podaci uspješno uèitani!";




    return 0;
    

}
