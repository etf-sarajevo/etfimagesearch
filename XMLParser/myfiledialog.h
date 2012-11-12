#ifndef MYFILEDIALOG_H
#define MYFILEDIALOG_H

#include <QApplication>

#include <QFileDialog>

class MyFileDialog : public QWidget
{
public:
  QString openFile()
  {
   QString filename= QFileDialog::getOpenFileName( this, tr("Open Document"), QDir::homePath(), tr("All files (*.*)"), 0, QFileDialog::DontUseNativeDialog );


    if( !filename.isNull() )
    {
      qDebug( filename.toAscii() );
    }
    return filename;
  }
  void saveFile()
  {
    QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Save Document"),
        QDir::currentPath(),
        tr("Documents (*.doc)") );
    if( !filename.isNull() )
    {
      qDebug( filename.toAscii() );
    }
  }
};

#endif // MYFILEDIALOG_H
