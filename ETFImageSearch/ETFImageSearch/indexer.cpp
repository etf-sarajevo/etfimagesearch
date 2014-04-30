#include "indexer.h"
#include "globals.h"

#include "lshindexer.h"
#include "treeindexer.h"
#include "sequentialindexer.h"

#ifdef LIB_ANN
#include "annindexer.h"
#endif

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QDebug>

#include <iostream>
#include <vector>
#include <algorithm>


// Libraries from the hacked version of libjpeg
extern "C" {
	#include "jinclude.h"
	#include "jpeglib.h"
	#include "jerror.h"
	#include "cderror.h"
}


Indexer::Indexer(ImageFeatures *alg, QString path) : path(path), alg(alg), pathIndexed(false), indexFile("")
{
	
}

// Set current working directory to path
void Indexer::setPath(QString path)
{
	this->path = path;
}

// Set current algorithm to alg
void Indexer::setAlgorithm(ImageFeatures *alg)
{
	this->alg = alg;
}



// Load existing image index or mark as not indexed
bool Indexer::loadIndex(QString indexFile)
{
	clearIndex();
	pathIndexed = false;

	QFile file(path + QDir::separator() + indexFile);
	if (!file.exists()) return false;
	
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);
	
	QString indexerId, indexerParams, imageFeaturesId, imageFeaturesParams;
	quint32 magic;
	in >> magic;
	if (magic != MAGIC)
		return false; // Not a ETFImageSearch file
	
	int len = strlen(APP_ID)+1;
	char* applicationId = new char[len];
	in >> applicationId;
	if (strcmp(applicationId, APP_ID) != 0) {
		//qDebug() << "Bad appid: "<<applicationId;
		delete[] applicationId;
		return false; // Not this version of ETFImageSearch
	}
	delete[] applicationId;
	
	// Skip header
	in >> indexerId >> indexerParams >> imageFeaturesId >> imageFeaturesParams;
	
	if (!loadIndexSub(in))
		return false;
	
	file.close();
	
	// Initialize whatever datastructures need to be initialized after putting in index
	indexPostInit();
	
	pathIndexed = true;
	this->indexFile = indexFile;
	return true;
}


void Indexer::writeIndex()
{
	// Find all appropriately named files in current path and see if we are just rebuilding an existing index
	QDir dir(path);
	int i(1);
	do {
		indexFile = QString("index%1.idx").arg(i);
		if (!dir.exists(indexFile)) break;
		
		QFile file(path + QDir::separator() + indexFile);
		file.open(QIODevice::ReadOnly);
		QDataStream in(&file);
		
		quint32 magic;
		in >> magic;
		if (magic != MAGIC) continue;
		
		int len = strlen(APP_ID)+1;
		char* applicationId = new char[len];
		
		in >> applicationId;
		if (strcmp(applicationId, APP_ID) != 0) {
			delete[] applicationId;
			continue;
		}
		delete[] applicationId;
		
		QString indexerId, indexerParams, imageFeaturesId, imageFeaturesParams;
		in >> indexerId >> indexerParams >> imageFeaturesId >> imageFeaturesParams;
		file.close();
		if (indexerId == name() && indexerParams == getParams() && imageFeaturesId == alg->name() && imageFeaturesParams == alg->getParams())
			// This is the same type of index, so we will just overwrite it
			break;
		i++;
	} while(true);
	
	// Proceed to write index file
	QFile file(path + QDir::separator() + indexFile);
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);
	
	out << MAGIC << APP_ID << name() << getParams() << alg->name() << alg->getParams();
	
	writeIndexSub(out);
	file.close();
}

void Indexer::writeDataType(QDataStream& out, double item)
{
	switch (alg->dataType()) {
	case ImageFeatures::INT8: {
		qint8 tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::UINT8: {
		quint8 tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::INT16: {
		qint16 tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::UINT16: {
		quint16 tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::INT32: {
		qint32 tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::UINT32: {
		quint32 tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::FLOAT32: {
		float tmp = item;
		out << tmp;
		break; }
	case ImageFeatures::DOUBLE64: {
		out << item;
		break; }
	}
}

double Indexer::readDataType(QDataStream& in)
{
	double item;
	switch (alg->dataType()) {
	case ImageFeatures::INT8: {
		qint8 tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::UINT8: {
		quint8 tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::INT16: {
		qint16 tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::UINT16: {
		quint16 tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::INT32: {
		qint32 tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::UINT32: {
		quint32 tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::FLOAT32: {
		float tmp;
		in >> tmp;
		item = tmp;
		break; }
	case ImageFeatures::DOUBLE64: {
		in >> item;
		break; }
	}
	return item;
}


void Indexer::buildIndex()
{
	if (path == "") return; // Path must be set
	
	clearIndex();
	
	// Index all files in path
	QDir dir(path);
	QFileInfoList list = dir.entryInfoList();
	emit startedIndexing(list.size());
	for (int i = 0; i < list.size(); i++) {
		QFileInfo fileInfo = list.at(i);
		if (!fileInfo.isFile()) continue;
		if (fileInfo.suffix().toLower() != "jpg") continue;
		
		//qDebug() << "Indexing "<<fileInfo.fileName();
		emit indexingFile(fileInfo.fileName());
		FeatureVector fv;
		fv=getFV(fileInfo.filePath());
		putInIndex(fileInfo.fileName(), fv);
	}
	
	// Initialize whatever datastructures need to be initialized after putting in index
	indexPostInit();
	
	// Serialize index to an index file
	writeIndex();
	pathIndexed = true;
	emit finishedIndexing();
}


// Call ImageFeatures alg to extract feature vector from image with given path
FeatureVector Indexer::getFV(QString imagePath)
{
	FeatureVector result;
	if (alg->isDct()) {
		DCTFeatures* dctalg = (DCTFeatures*) alg;
		
		/* Extracting DCT coefficients from image using hacked libjpeg */
		/* Below is the usual code for decoding JPEG image */
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		FILE * input_file;
		JDIMENSION num_scanlines;

		/* Initialize the JPEG decompression object with default error handling. */
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);

		if ((input_file = fopen(imagePath.toLatin1().data(), "rb")) == NULL)
			throw "Can't open image file";

		/* Specify data source for decompression */
		jpeg_stdio_src(&cinfo, input_file);

		/* Read file header, set default decompression parameters */
		(void) jpeg_read_header(&cinfo, TRUE);

		/* Start decompressor */
		(void) jpeg_start_decompress(&cinfo);

		/* Initialize search algorithm */
		dctalg->init();

		/* Set callback function. This will be called for each block! */
		libjpeg_cbir_process_block_callback_object = dctalg;
		libjpeg_cbir_process_block_callback = DCTFeatures::processBlockWrapper;

		/* Read all data into buffer */
		JSAMPARRAY bugger = new JSAMPROW[cinfo.output_height];
		for (uint i=0; i<cinfo.output_height; i++)
			bugger[i] = new JSAMPLE[cinfo.output_width * 3];

		/* Process data */
		while (cinfo.output_scanline < cinfo.output_height) {
			num_scanlines = jpeg_read_scanlines(&cinfo, bugger,
												cinfo.output_height);
			/* do nothing, dctalg->procesBlock will be called by libjpeg */
		}

		/* Get the feature vector */
		result = dctalg->calculateVector();

		/* Close JPEG file and release memory */
		(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);

		fclose(input_file);

		for (uint i=0; i<cinfo.output_height; i++)
			delete[] bugger[i];
		delete [] bugger;


	} else {
		
		// If algorithm is not DCT we just read the QImage and send it to algorithm
		QImage image(imagePath);
		result = alg->extractFeatures(image.bits(), image.width(), image.height());
		/*qDebug()<<"INDEXER "<<image.byteCount()<<" "<<imagePath;
		for (int i(0); i<result.size(); i++)
			qDebug() << result[i];*/
	}
	
	return result;
}


// Helper function for sorting search results by distance
bool Indexer::resultLessThen(const Indexer::Result& r1, const Indexer::Result& r2)
{
	return r1.distance < r2.distance;
}

Indexer* Indexer::factory(QString name, ImageFeatures* alg, QString path)
{
	Indexer* indexer;
	if (name == SequentialIndexer::static_name())
		indexer = new SequentialIndexer(alg, path);
#ifdef LIB_ANN
	if (name == ANNIndexer::static_name())
		indexer = new ANNIndexer(alg, path);
#endif
	if (name == LSHIndexer::static_name())
		indexer = new LSHIndexer(alg, path);
	if (name == TreeIndexer::static_name())
		indexer = new TreeIndexer(alg, path);
	return indexer;
}

QList<QStringList> Indexer::listIndices(QString path)
{
	QDir dir(path);
	QStringList filters;
	filters << "*.idx";
	dir.setNameFilters(filters);
	QFileInfoList list = dir.entryInfoList();
	QList<QStringList> results;
	
	for (int i = 0; i < list.size(); i++) {
		QFile file(list.at(i).canonicalFilePath());
		file.open(QIODevice::ReadOnly);
		QDataStream in(&file);
		//qDebug() << "File: "<<list.at(i).canonicalFilePath();
		
		QString indexerId, indexerParams, imageFeaturesId, imageFeaturesParams;
		quint32 magic;
		in >> magic;
		if (magic != MAGIC) {
			//qDebug() << "Bad magic: "<<magic;
			continue; // Not a ETFImageSearch file
		}
		
		int len = strlen(APP_ID)+1;
		char* applicationId = new char[len];
		in >> applicationId;
		if (strcmp(applicationId, APP_ID) != 0) {
			//qDebug() << "Bad appid: "<<applicationId;
			delete[] applicationId;
			continue; // Not this version of ETFImageSearch
		}
		delete[] applicationId;
		
		in >> indexerId >> indexerParams >> imageFeaturesId >> imageFeaturesParams;
		//qDebug() << indexerId << indexerParams << imageFeaturesId << imageFeaturesParams;
		
		QStringList info;
		info << list.at(i).fileName() << indexerId << indexerParams << imageFeaturesId << imageFeaturesParams;
		results.append(info);
		
		file.close();
	}
	return results;
}

QStringList Indexer::indicesMenu(QString path)
{
	QStringList results;
	QList<QStringList> indices = listIndices(path);
	for (int i(0); i<indices.size(); i++) {
		QString featuresId(indices[i][3]), featuresParams(indices[i][4]), indexerId(indices[i][1]), indexerParams(indices[i][2]);
		
		// Beautify - some alg-specific hacks
		if (indexerParams != "") indexerParams = " ("+indexerParams+")";
		if (featuresParams != "") featuresParams = " ("+featuresParams+")";
		featuresParams.replace(QString("NO_NORMALIZATION"), QString("no norm."));
		featuresParams.replace(QString("MAX_NORMALIZATION"), QString("max norm."));
		featuresParams.replace(QString("BOTH_NORMALIZATION"), QString("both norm."));
		featuresParams.replace(QString("NOT_CUMULATIVE"), QString("not cumulative"));
		featuresParams.replace(QString("CUMULATIVE"), QString("cumulative"));
		featuresParams.replace(QString("COMBINED"), QString("combined"));
		featuresParams.replace(QString("SPLIT"), QString("split"));
		
		QString text = QString("%1%2 - %3%4").arg(featuresId).arg(featuresParams).arg(indexerId).arg(indexerParams);
		results.append(text);
	}
	return results;
}

Indexer* Indexer::createIndex(QString path, int ordinal)
{
	Indexer* indexer;
	QList<QStringList> indices = listIndices(path);
	QStringList info = indices[ordinal];
	
	ImageFeatures* alg = ImageFeatures::factory(info[3]);
	alg->setParams(info[4]);
	indexer = factory(info[1], alg, path);
	indexer->setParams(info[2]);
	
	indexer->loadIndex(info[0]);
	
	return indexer;
}


