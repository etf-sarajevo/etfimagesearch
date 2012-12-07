#include "indexer.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QDebug>


// Libraries from the hacked version of libjpeg
extern "C" {
	#include "jinclude.h"
	#include "jpeglib.h"
	#include "jerror.h"
	#include "cderror.h"
}


Indexer::Indexer(SearchAlgorithm *alg, QString path) : path(path), alg(alg), pathIndexed(false)
{
	loadIndex();
}

// Set current working directory to path
void Indexer::setPath(QString path)
{
	this->path = path;
	loadIndex();
}

// Set current algorithm to alg
void Indexer::setAlgorithm(SearchAlgorithm* alg)
{
	this->alg = alg;
	loadIndex();
}

// Load existing image index or mark as not indexed
void Indexer::loadIndex()
{
	index.clear();
	pathIndexed = false;
	
	QFile file(path + QDir::separator() + "etfis_" + alg->name() + ".idx");
	if (file.exists()) {
		file.open(QIODevice::ReadOnly);
		QDataStream in(&file);
		while (!in.atEnd()) {
			QString key;
			FeatureVector vec;
			in >> key >> vec.features;
			index[key] = vec;
		}
		file.close();
		pathIndexed = true;
	}
}

// Call SearchAlgorithm alg to extract feature vector from image with given path
FeatureVector Indexer::getFV(QString imagePath)
{
	FeatureVector result;
	if (alg->isDct()) {
		DCTSearchAlgorithm* dctalg = (DCTSearchAlgorithm*) alg;
		
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
		libjpeg_cbir_process_block_callback = DCTSearchAlgorithm::processBlockWrapper;

		/* Read all data into buffer */
		JSAMPARRAY bugger = new JSAMPROW[cinfo.output_height];
		for (int i=0; i<cinfo.output_height; i++)
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

		for (int i=0; i<cinfo.output_height; i++)
			delete bugger[i];
		delete [] bugger;


	} else {
		
		// If algorithm is not DCT we just read the QImage and send it to algorithm
		QImage image(imagePath);
		result = alg->extractFeatures(image.constBits(), image.byteCount());
	}
	
	return result;
}


// Helper function for sorting search results by distance
bool resultLessThen(const Indexer::Result& r1, const Indexer::Result& r2)
{
	return r1.distance < r2.distance;
}

// Search image with given filepath in current index (directory must be indexed)
QVector<Indexer::Result> Indexer::search(QString filePath)
{
	FeatureVector searchVector = getFV(filePath);
	
	QVector<Indexer::Result> results;
	QMapIterator<QString, FeatureVector> i(index);
	while (i.hasNext()) {
		i.next();
		Indexer::Result r;
		r.fileName = i.key();
		r.distance = alg->distance(searchVector, i.value());
		results.push_back(r);
	}
	
	qSort(results.begin(), results.end(), resultLessThen);
	
	return results;
}


// Create index of images in current path (calculate feature vectors)
void Indexer::createIndex()
{
	if (path == "") return; // Path must be set
	
	index.clear();
	
	// Index all files in path
	QDir dir(path);
	QFileInfoList list = dir.entryInfoList();
	emit startedIndexing(list.size());
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		if (!fileInfo.isFile()) continue;
		if (fileInfo.completeSuffix().toLower() != "jpg") continue;
		
		//qDebug() << "Indexing "<<fileInfo.fileName();
		emit indexingFile(fileInfo.fileName());
		index[fileInfo.fileName()] = getFV(fileInfo.filePath());
	}
	
	// Serialize index to an index file
	QFile file(path + QDir::separator() + "etfis_" + alg->name() + ".idx");
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);
	QMapIterator<QString, FeatureVector> i(index);
	while (i.hasNext()) {
		i.next();
		out << i.key() << i.value().features;
	}
	file.close();
	pathIndexed = true;
	emit finishedIndexing();
}

