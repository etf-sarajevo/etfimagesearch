#include "imagefeatures.h"
#include "colorhistogram.h"
#include "colormoments.h"
#include "fuzzyhistogram.h"
#include "fuzzyspatialhistogram.h"
#include "spatialhistogram.h"
#include "luetal_v2.h"
#include "cedd.h"
#include "hmmdquant.h"
#include "zhangetal.h"

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
	if (name == ColorMoments::static_name())
		return new ColorMoments();
	if (name == FuzzyHistogram::static_name())
		return new FuzzyHistogram();
	if (name == FuzzySpatialHistogram::static_name())
		return new FuzzySpatialHistogram();
	if (name == SpatialHistogram::static_name())
		return new SpatialHistogram();
	if (name == CEDD::static_name())
		return new CEDD();
	if (name == LuEtAl_v2::static_name())
		return new LuEtAl_v2();
	if (name == HMMDquant::static_name())
		return new HMMDquant();
	if (name == ZhangEtAl::static_name())
		return new ZhangEtAl();
	throw QString("Unknown feature '%1'").arg(name);
}

template<typename T>
void ImageFeatures::vectorDump(QVector<T> vector) {
	QString output;
	for (uint i(0); i<vector.size(); i++)
		output += QString("%1,").arg(vector[i]);
	qDebug() << output;
}

template<typename T>
void ImageFeatures::vectorDump(std::vector<T> vector) {
	QString output;
	for (uint i(0); i<vector.size(); i++)
		output += QString("%1,").arg(vector[i]);
	qDebug() << output;
}

void ImageFeatures::vectorDump(FeatureVector vector) {
	QString output;
	for (uint i(0); i<vector.size(); i++)
		output += QString("%1,").arg(vector[i]);
	qDebug() << output;
}
