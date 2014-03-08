#include "imagefeatures.h"
#include "luetal_v2.h"

#include <QDebug>

ImageFeatures::ImageFeatures()
{
}

DCTFeatures::DCTFeatures()
{
}

ImageFeatures* ImageFeatures::factory(QString name)
{
	if (name == ColorHistogram::static_name())
		return new ColorHistogram();
	if (name == LuEtAl_v2::static_name())
		return new LuEtAl_v2();
}

void ImageFeatures::vectorDump(QVector<double> vector) {
	QString output;
	for (uint i(0); i<vector.size(); i++)
		output += QString("%1,").arg(vector[i]);
	qDebug() << output;
}

void ImageFeatures::vectorDump(std::vector<double> vector) {
	QString output;
	for (uint i(0); i<vector.size(); i++)
		output += QString("%1,").arg(vector[i]);
	qDebug() << output;
}

void ImageFeatures::vectorDump(std::vector<int> vector) {
	QString output;
	for (uint i(0); i<vector.size(); i++)
		output += QString("%1,").arg(vector[i]);
	qDebug() << output;
}
