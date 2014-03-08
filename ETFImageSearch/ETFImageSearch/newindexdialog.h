#ifndef NEWINDEXDIALOG_H
#define NEWINDEXDIALOG_H

#include <QDialog>

#include "imagefeatures.h"
#include "indexer.h"

namespace Ui {
class NewIndexDialog;
}

class NewIndexDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit NewIndexDialog(QWidget *parent = 0);
	~NewIndexDialog();
	
	ImageFeatures* getFeatures();
	Indexer* getIndexer(ImageFeatures* alg, QString path);
	
private slots:
	void on_featureTypeCombo_currentIndexChanged(const QString &arg1);
	
private:
	Ui::NewIndexDialog *ui;
};

#endif // NEWINDEXDIALOG_H
