#ifndef JPEGDECODE_H
#define JPEGDECODE_H

using namespace std;

#include "JPEGImage_global.h"
//#include "interfaces/exportimportinterface.h"

#include <QFile>
#include <QList>
#include <QTime>
#include <QMap>

#include <math.h>
#include <fstream>

#include "component.h"
#include "huffmantable.h"

//#include "image.h"

#define ETF_FORMAT_MAX_COMPONENTS 3


class JpegDecode {

public:

    JpegDecode(const char* jpegFile);
    ~JpegDecode() {
        for (int i=0; i<huffmanTrees.size(); i++) {
            /*for (int j=0; j<65536; j++)
                if (huffmanTrees[i]->huffmanShortLUT[j] != 0)
                    delete huffmanTrees[i]->huffmanShortLUT[j];*/
            delete huffmanTrees[i];
        }
    }

  //  ImageStatePtr imagePtr;
//    QImage getImage(){
  //      return imagePtr->mergedQImage();
    //}

    double featureVectorME[ETF_FORMAT_MAX_COMPONENTS][4];
    double featureVectorD[ETF_FORMAT_MAX_COMPONENTS][12];

private:

    // Various metadata and picture information
    QMap<QString, QString> metadata;
    vector<unsigned char> thumbnail;
    unsigned char precision; // Precison of elements (8 or 12 bits)

    // Data needed for decoding
    vector<Component> components;
    vector <QuantizationTable> quantizationTables;
    vector<HuffmanTable*> huffmanTables;
    vector<HuffmanTree*> huffmanTrees;

    HuffmanTable* componentTablesDC[ETF_FORMAT_MAX_COMPONENTS]; // from format.h
    HuffmanTable* componentTablesAC[ETF_FORMAT_MAX_COMPONENTS];
    HuffmanTree* componentTreesDC[ETF_FORMAT_MAX_COMPONENTS]; // from format.h
    HuffmanTree* componentTreesAC[ETF_FORMAT_MAX_COMPONENTS];

    bool endOfFile;
    int previousDC[ETF_FORMAT_MAX_COMPONENTS];
    //Format::ColorModel adobeColorModel;
    bool losslessFormat;
    unsigned char zigZagStart, zigZagEnd;
    unsigned char approximationH, approximationL;
    int *scanLineCache[ETF_FORMAT_MAX_COMPONENTS];
    int imageWidth, imageHeight;

    // CBIR
    double colorFeatures[ETF_FORMAT_MAX_COMPONENTS][4];
    int colorFeaturesLast1000[ETF_FORMAT_MAX_COMPONENTS][4];
    int colorFeaturesCounters[ETF_FORMAT_MAX_COMPONENTS];
    int colorFeaturesDoubleCounters[ETF_FORMAT_MAX_COMPONENTS];

    double specificBlocks[ETF_FORMAT_MAX_COMPONENTS][6];
    int specificBlocksLast1000[ETF_FORMAT_MAX_COMPONENTS][6];
    double specificBlocksSquares[ETF_FORMAT_MAX_COMPONENTS][6];
    int specificBlocksSquaresLast1000[ETF_FORMAT_MAX_COMPONENTS][6];


    // Main loop
    void readFile(QFile &file);

    // Loop steps
    void readAppSegment(QFile &picture, unsigned char marker);
        void readMetaDataJFIF(QFile &picture);
        void readMetaDataJFXX(QFile &picture, int headerLength);

    void readFrameHeader(QFile &picture, unsigned char marker);
    void readHuffmanTables(QFile &picture);
    void readArithmeticCoding(QFile &picture);
    void readQuantizationTables(QFile &picture);
    void readComments(QFile &picture);
    void readScanHeader(QFile &picture);
    void readImageData(QFile &picture);

    //HuffmanTable huffmanTable;
    void readHuffmanBlock(QFile &picture, int* block, int currentComponent);
        bool readMoreData(QFile &picture, unsigned int &data, unsigned int &currentDataLength);

    bool isEndOfFile() { return endOfFile; }

    void IDCT (int block8x8[8][8], int block2[8][8]);
    void multiplyWithQuantizationTable(int dataBlock[8][8], int currentComponent);
    void addNewData8x8Block(int dataBlock[8][8], int currentComponent);
    void addLossless(int *array, int currentComponent);

    void inverseZigZagCoding(char *array,unsigned  char ** matrix);
    void inverseZigZagCoding(int *array, int matrix[8][8]);

    // Debug
    string binary (unsigned int v);
    void outputQuantizationTables();
    void outputHuffmanCodes();
    void dumpMetaData();
    void dumpBlock(int block[8][8]);

    // Benchmark
    long huffmanTime, zigZagTime, quantizeTime, idctTime, addBlockTime;
    long acTime, dcTime, rmdTime, getNodeTime;
    struct timeval start, end;

    // Huffman data
    unsigned int data;
    unsigned int currentDataLength;
    unsigned int byteno;
    int commentNo;
    unsigned int currentDataBlockX[ETF_FORMAT_MAX_COMPONENTS];
    unsigned int currentDataBlockY[ETF_FORMAT_MAX_COMPONENTS];
    char* rawdata;
    char* tmpdata;
    int rawdatasize;
    int masks[16];
    char buff[65536];
    int bufpos, bufsubpos;

};


#endif // JPEGDECODE_H
