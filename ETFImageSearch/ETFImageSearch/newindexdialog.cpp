#include "newindexdialog.h"
#include "ui_newindexdialog.h"

#include "distancemetric.h"

#include <QDebug>

NewIndexDialog::NewIndexDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::NewIndexDialog)
{
	ui->setupUi(this);
	ui->distanceMetricCombo->addItems(DistanceMetric::allMetrics());
	on_featureTypeCombo_currentIndexChanged(ui->featureTypeCombo->currentText());
}

NewIndexDialog::~NewIndexDialog()
{
	delete ui;
}

void NewIndexDialog::on_featureTypeCombo_currentIndexChanged(const QString &arg1)
{
	if (arg1 == "Color Histogram" || arg1 == "Zhang et al." || arg1 == "HMMD quantization from CSD") {
		ui->blankFeatureOptions->setVisible(false);
		ui->colorHistogramOptions_2->setVisible(true);
	} else {
		ui->blankFeatureOptions->setVisible(true);
		ui->colorHistogramOptions_2->setVisible(false);
	}
}

ImageFeatures* NewIndexDialog::getFeatures()
{
	QString featureName = ui->featureTypeCombo->currentText();
	ImageFeatures* alg = ImageFeatures::factory(featureName);
	
	if (featureName == "Color Histogram" || featureName == "Zhang et al." || featureName == "HMMD quantization from CSD") {
		QStringList paramsList;
		paramsList.append( ui->colorModelCombo->currentText() );
		paramsList.append( ui->cq1spin->text() + "," + ui->cq2spin->text() + "," + ui->cq3spin->text() );
		if (ui->histogramTypeCombo->currentIndex() == 0)
			paramsList.append("COMBINED");
		else
			paramsList.append("SPLIT");
		
		if (ui->normalizationCombo->currentIndex() == 0)
			paramsList.append("NO_NORMALIZATION");
		else if (ui->normalizationCombo->currentIndex() == 1)
			paramsList.append("MAX_NORMALIZATION");
		else if (ui->normalizationCombo->currentIndex() == 2)
			paramsList.append("BOTH_NORMALIZATION");
		
		paramsList.append(ui->histogramQuantizationSpin->text());
		
		if (ui->cumulativeCheck->isChecked())
			paramsList.append("CUMULATIVE");
		else 
			paramsList.append("NOT_CUMULATIVE");
		
		paramsList.append(ui->distanceMetricCombo->currentText());
		
		alg->setParams(paramsList.join(QString(';')));
	}
	
	return alg;
}

Indexer* NewIndexDialog::getIndexer(ImageFeatures* alg, QString path)
{
	return Indexer::factory(ui->indexerCombo->currentText(), alg, path);  
}

