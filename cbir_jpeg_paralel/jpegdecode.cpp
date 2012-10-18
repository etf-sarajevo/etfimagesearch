#include "jpegdecode.h"
#include <iostream>
#include <iomanip>
#include <QDebug>
#include <QtEndian>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>


#define DEBUGLEVEL 0
//#define BENCHMARK


const double pi=3.1415926535897932384626433832795;


JpegDecode::JpegDecode(const char* fileName) : zigZagStart(0), zigZagEnd(63), /*adobeColorModel(Format::YCBCR), */losslessFormat(false) {

    QFile file(fileName);
    commentNo = 1;


    masks[0] = pow(2,15);
    for (int i=1; i<16; i++) {
        masks[i] = masks[i-1] + pow(2, 15-i);
    }


    struct timeval mstart, mend;
    huffmanTime = zigZagTime = quantizeTime = idctTime = addBlockTime = 0;
    acTime = dcTime = rmdTime = getNodeTime = 0;

    gettimeofday(&mstart, NULL);
//    this->imagePtr = ImageStatePtr(new ImageState(file.fileName()));
    if (!file.open(QFile::ReadOnly)) throw "Can't open file";

    readFile(file);

    file.close();

#ifdef BENCHMARK
    cout << "Huffman "<<huffmanTime<<" ms, ZigZag "<<zigZagTime<<" ms, Quantize "<<quantizeTime<<" ms, IDCT "<<idctTime<<" ms, Add Block "<<addBlockTime<<" ms"<<endl;
    cout << "AC "<<acTime<<" ms, DC "<<dcTime<<" ms, readMoreData "<<rmdTime<<" ms, getNode "<<getNodeTime<<" ms"<<endl;
    cout << "TOTAL: " << (huffmanTime + zigZagTime + quantizeTime + idctTime + addBlockTime) << " ms"<<endl;
#endif

    gettimeofday(&mend, NULL);

    cout << "Huffmannn "<<huffmanTime<<" us, ZigZag "<<zigZagTime<<" us, Quantize 0 us, Feature calc. "<<quantizeTime<<" us, IDCT "<<idctTime<<" us, Add Block "<<addBlockTime<<" us"<<endl;
    cout << "AC "<<acTime<<" us, DC "<<dcTime<<" us, readMoreData "<<rmdTime<<" us, getNode "<<getNodeTime<<" us"<<endl;
    cout << "Total: " << ((mend.tv_sec  - mstart.tv_sec) * 1000000 + mend.tv_usec - mstart.tv_usec) << " us" << endl;

//    imagePtr->activeLayer()->updatePreview();
}


void JpegDecode::readFile(QFile &file) {
    unsigned char ff, marker;

    // Is this a JPEG file?
    file.read((char*)&ff, sizeof(ff));
    file.read((char*)&marker, sizeof(ff));
    if (ff != 0xff || marker != 0xd8)
        throw "This is not a JPEG file";

    bool endOfImage=false;
    while (!file.atEnd()) {
        file.read((char*)&ff, sizeof(ff));

        if (ff != 0xff) {
            qDebug() << "0xFF expected but found "<<ff<<" at pos "<<file.pos();
            continue;
        }

        file.read((char*)&marker, sizeof(ff));
#if DEBUGLEVEL>1
        cout << "Found marker "<<hex<<int(marker) <<dec<< " pos "<<file.pos()<< endl;
#endif

        switch (marker) {
            case 0xd8: // SOI = Start Of Image
                if (file.pos() != 2)
                    qDebug() << "Start of image (D8) found inside file?";
                break;

            case 0xd9: // EOI = End Of Image
                endOfImage = true;
                break;

            // 0xe0 - 0xef : APPn = Application specific segments?
            case 0xe0:
            case 0xe1:
            case 0xe2:
            case 0xe3:
            case 0xe4:
            case 0xe5:
            case 0xe6:
            case 0xe7:
            case 0xe8:
            case 0xe9:
            case 0xea:
            case 0xeb:
            case 0xec:
            case 0xed:
            case 0xee:
            case 0xef:
                readAppSegment(file, marker);
                break;

            case 0xc0: // SOF = Start Of Frame
            case 0xc1:
            case 0xc2:
            case 0xc3:
            case 0xc5:
            case 0xc6:
            case 0xc7:
            case 0xc8:
            case 0xc9:
            case 0xca:
            case 0xcb:
            case 0xcd:
            case 0xce:
            case 0xcf:
                readFrameHeader(file, marker);
                break;

            case 0xc4: // DHT = Define Huffman Tables
                gettimeofday(&start, NULL);
                readHuffmanTables(file);
                gettimeofday(&end, NULL);
                idctTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
                break;

            case 0xcc: // DAC = Define Arithmetic Coding
                readArithmeticCoding(file);
                break;

            case 0xdb: // DQT = Define Quantization Tables
                readQuantizationTables(file);
                break;

            case 0xdc: // DNL = Define Number of Lines
                // DNL is always 4 bytes long
                // It is only used if a scan has different number of lines from previous scan (progressive?)
                // [ ITU-T T.81, chapter B.2.5 ]
                // If you find a progressive JPEG with this feature, please implement
                for (int i=0; i<4; i++)
                    file.read((char*)&ff, sizeof(ff));
                break;

            case 0xdd: // DRI = Define Restart Interval
                // DRI is always 4 bytes long
                // We are currently just ignoring restart markers
                for (int i=0; i<4; i++)
                    file.read((char*)&ff, sizeof(ff));
                break;

            case 0xde: // DHP = Define Hierarchical Progression
            case 0xdf: // EXP = Expand Reference Components
                throw "Hierarchical JPEG not supported yet";

            case 0xfe: // COM = Comment
                readComments(file);
                break;

            case 0xda: // SOS = Start Of Scan
                readScanHeader(file);

                // Scan starts immediately after header
                gettimeofday(&start, NULL);
                readImageData(file);
                gettimeofday(&end, NULL);
                huffmanTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
                return;
                break;

            default:
                qDebug() << "Unknown marker "<<marker<<" - lets ignore and hope for the best...";

        }

        if (endOfImage) break;
    }

//    if (!endOfImage)
//        qDebug() << "File ended prematurely.";

//    dumpMetaData();
}


void JpegDecode::readMetaDataJFIF(QFile &picture) {
    metadata["JPEG format"] = "JPEG/JFIF";
#if DEBUGLEVEL>1
    qDebug() << "Read metadata JFIF";
#endif

    unsigned char c1,c2;
    picture.read((char*)&c1, sizeof(c1));
    picture.read((char*)&c2, sizeof(c2));
    metadata["JFIF version"] = QString("%1.%2").arg(int(c1)).arg(int(c2));

    picture.read((char*)&c1, sizeof(c1));
    unsigned short res1, res2;
    picture.read((char*)&res1, sizeof(res1));
    picture.read((char*)&res2, sizeof(res2));
    res1 = qFromBigEndian(res1);
    res2 = qFromBigEndian(res2);

    if (c1==0)
        metadata["Resolution"] = QString("%1x%2 pixels").arg(res1).arg(res2);
    else if (c1==1)
        metadata["Resolution"] = QString("%1x%2 DPI").arg(res1).arg(res2);
    else if (c1==2)
        metadata["Resolution"] = QString("%1x%2 DPcm").arg(res1).arg(res2);

    picture.read((char*)&c1, sizeof(c1));
    picture.read((char*)&c2, sizeof(c2));
    if (c1>0 && c2>0) {
        metadata["Thumbnail format"] = "Uncompressed RGB";
        metadata["Thumbnail size"] = QString("%1x%2").arg(int(c1)).arg(int(c2));
        thumbnail.resize(3*c1*c2);
        for (int i=0; i < 3*c1*c2; i++) {
            picture.read((char*)&c1, sizeof(c1));
            thumbnail[i] = c1;
        }
    }

}


void JpegDecode::readMetaDataJFXX(QFile &picture, int headerLength) {
    metadata["JPEG format"] = "JPEG/JFXX";
#if DEBUGLEVEL>1
    qDebug() << "Read metadata JXFF";
#endif

    unsigned char tf,tw,th;

    picture.read((char*)&tf, sizeof(tf));
    if (tf==0x10) {
        metadata["Thumbnail format"] = "JPEG/JIF";
        thumbnail.resize(headerLength - 1); // Substract the TF byte that was just read
        for (int i=0; i < headerLength - 1; i++) {
            picture.read((char*)&tw, sizeof(tw));
            thumbnail[i] = tw;
        }

    } else {
        picture.read((char*)&tw, sizeof(tw));
        picture.read((char*)&th, sizeof(th));
        if (tw>0 && th>0) {
            metadata["Thumbnail size"] = QString("%1x%2").arg(int(tw)).arg(int(th));
            int thumbLen = 0;
            if (tf==0x11) {
                metadata["Thumbnail format"] = "Paletted";
                thumbLen = 768 + tw*th;
            }
            if (tf==0x11) {
                metadata["Thumbnail format"] = "Uncompressed RGB";
                thumbLen = 3*tw*th;
            }
            for (int i=0; i < thumbLen; i++) {
                picture.read((char*)&tw, sizeof(tw));
                thumbnail[i] = tw;
            }
        }
    }
}


void JpegDecode::readAppSegment(QFile &picture, unsigned char marker) {
    // Known application segments
    // http://www.ozhiker.com/electronics/pjmt/jpeg_info/app_segments.html
    // http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/JPEG.html
    const char* segmentTypes[11] = { "JFIF", "JFXX", "EXIF", "Exif", "http://ns.adobe.com/xap/1.0/", "ICC_PROFILE", "META", "Meta", "Ducky", "Photoshop 3.0", "Adobe" };
    char data[30];
    char* ptr=data;

    unsigned short len;
    picture.read((char*)&len, sizeof(len));
    len = qFromBigEndian(len) - 2; // Two bytes for len are included in len value
    bool found=false;

    switch (marker) {

    case 0xe0:
        if (len >= 5) {
            for (int i=0; i<5; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[0], 5) == 0) {
                readMetaDataJFIF(picture);
                return;
            }
            else if (strncmp(data, segmentTypes[1], 5) == 0) {
                readMetaDataJFXX(picture, len-5);
                return;
            }
            len -= 5;
        }
        break;

    case 0xe1:
        if (len >= 5) {
            for (int i=0; i<5; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[2], 5) == 0 || strncmp(data, segmentTypes[3], 5) == 0) {
                //readMetaDataExif(picture, headerLength); // TODO
                // Parsing Exif data is in fact parsing a whole TIFF file...
                qDebug() << "EXIF not supported yet...";
                found = true;

            } else if (len >= 28) {
                for (int i=0; i<23; i++)
                    picture.read(ptr++, sizeof(char));
                if (strncmp(data, segmentTypes[4], 23) == 0) {
                    qDebug() << "Adobe XMP (eXtensible Metadata Platform) not supported yet...";
                    found = true;
                }
                len -= 23;
            }
            len -= 5;
        }
        break;

    case 0xe2:
        if (len >= 12) {
            for (int i=0; i<12; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[5], 12) == 0) {
                qDebug() << "ICC profiles not supported...";
                found = true;
            }
            len -= 12;
        }
        break;

    case 0xe3:
        if (len >= 5) {
            for (int i=0; i<5; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[6], 5) == 0 || strncmp(data, segmentTypes[7], 5) == 0) {
                //readMetaDataExif(picture, headerLength); // TODO
                // Parsing Exif data is in fact parsing a whole TIFF file...
                qDebug() << "EXIF not supported yet...";
                found = true;
            }
            len -= 5;
        }
        break;

    case 0xec:
        if (len >= 6) {
            for (int i=0; i<6; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[8], 6) == 0) {
                // There is nothing in the "Ducky" tag that I can use, and my samples don't match description at
                // http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/APP12.html#Ducky
                metadata["Photoshop 'Save for Web'"] = "yes";
                len -= 6;
                found = true;

            } else {
                // Apparently APP12 (0xEC) is otherwise used as a comment
                char* text = new char[len];
                char* tmp=text;
                for (int i=0; i<len; i++)
                    picture.read(tmp++, sizeof(char));
                *tmp = '\0';
                metadata["APP12 Comment"] = QString(text);
                delete[] text;
                return;
            }
        }
        break;

    case 0xed:
        if (len >= 14) {
            for (int i=0; i<14; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[9], 14) == 0) {
                qDebug() << "Adobe Photoshop(R) metadata not supported...";
                found = true;
            }
            len -= 14;
        }
        break;

    case 0xee:
        if (len >= 6) {
            for (int i=0; i<6; i++)
                picture.read(ptr++, sizeof(char));
            if (strncmp(data, segmentTypes[10], 6) == 0) {
                // Adobe metadata - helps us detect CMYK and YCCK images
                unsigned char cc;
                for (int i=0; i<6; i++)
                    picture.read ((char*)&cc, sizeof(cc));
                /*if (cc == 0)
                    adobeColorModel = Format::CMYK; // It can also be RGB if there are three components!
                else if (cc == 1)
                    adobeColorModel = Format::YCBCR;
                else if (cc == 2)
                    throw "YCCK color model currently not supported!";*/
                return;
            }
            len -= 6;
        }
        break;

    }

    if (!found)
        qDebug() << "Unsupported Application segment, marker"<<marker<<len;

    unsigned char c;
    for (int i=0; i<len; i++)
        picture.read ((char*)&c, sizeof(c));
}



void JpegDecode::readComments(QFile &picture) {
    unsigned short len;
    picture.read((char*)&len, sizeof(len));
    len = qFromBigEndian(len);

    char* text = new char[len];
    char* tmp=text;
    for (int i=0; i<len-2; i++)
        picture.read(tmp++, sizeof(char));
    *tmp = '\0';
    metadata[QString("Comment %1").arg(commentNo++)] = QString(text);
    delete[] text;
}


void JpegDecode::readFrameHeader(QFile &picture, unsigned char marker) {
    if (marker == 0xc0) {
        metadata["Subformat"] = "Baseline DCT";

    } else if (marker == 0xc1) {
        metadata["Subformat"] = "Extended DCT";
        // The only difference is support for multiple Huffman tables and 12-bit precision
        // which we will support with baseline as well

    } else if (marker == 0xc2) {
        metadata["Subformat"] = "Progressive DCT";
        //throw "Progressive JPEG not supported yet";

    } else if (marker == 0xc3) {
        metadata["Subformat"] = "Lossless";
        losslessFormat = true;

    } else if (marker == 0xc9) {
        metadata["Subformat"] = "Arithmetic DCT";
        throw "Arithmetic coding JPEG not supported yet";

    } else if (marker == 0xca) {
        metadata["Subformat"] = "Arithmetic progressive DCT";
        throw "Arithmetic coding progressive JPEG not supported yet";

    } else if (marker == 0xcb) {
        metadata["Subformat"] = "Arithmetic lossless";
        losslessFormat = true;
        throw "Arithmetic coding lossless JPEG not supported yet";

    } else {
        throw "Unknown JPEG subformat - frame header not found.";
    }

    // Header length
    unsigned short headerLength;
    picture.read((char*)&headerLength, sizeof(headerLength));
    headerLength = qFromBigEndian(headerLength);

    picture.read((char*)&precision,sizeof(precision));
    metadata["Precision"] = QString("%1").arg(int(precision));

    //Taking picture height and width. These informations are stored in two bytes for each dimension
    unsigned short pictureHeight, pictureWidth;
    picture.read ((char*)&pictureHeight, sizeof(pictureHeight));
    picture.read ((char*)&pictureWidth, sizeof(pictureWidth));
    pictureHeight = qFromBigEndian(pictureHeight);
    pictureWidth = qFromBigEndian(pictureWidth);

    //Taking information for number of components and components data
    //Every component has 4 information. ComponentID, Horizontal sampling, Vertical Sampling, and Quantization table ID
    unsigned char numberOfComponents;
    picture.read((char*)&numberOfComponents,sizeof(numberOfComponents));
    if (headerLength != numberOfComponents*3 + 8)
        qDebug() << "Bad SOF header length: expected"<<(numberOfComponents*3+8)<<"but given"<<headerLength;
    metadata["Number of components"] = QString("%1").arg(int(numberOfComponents));

    for (int i=0;i<numberOfComponents;i++) {
        unsigned char componentData[3];
        picture.read ((char*)componentData,sizeof(componentData));

        if (losslessFormat) { // Lossless format doesn't use quantization tables
            QuantizationTable fake(false, false);
            Component a(componentData[0], (componentData[1]>>4), (componentData[1] & 0x0F), componentData[2], fake);
            this->components.push_back(a);
            continue;
        }

        bool foundTable=false;
        for (unsigned int x=0;x<quantizationTables.size();x++) {

            if (quantizationTables[x].tableID==componentData[2]) {
                Component a(componentData[0], (componentData[1]>>4), (componentData[1] & 0x0F), componentData[2], quantizationTables[x]);
                this->components.push_back(a);
                foundTable=true;

            }
        }

        if (!foundTable) {
            qDebug() << "Component "<<componentData[0]<<" specifies unknown quantization table - skipping.";
        }
    }

    // Set number of components to ImageState
    /*Format f = imagePtr->format();

    // Default JPEG color model
    f.setColorModel(Format::YCBCR);
    if (precision <= 8)
        f.setColorDepth(24);
    else
        // 12-bit is not directly supported by EtfShop, use 16-bit instead
        f.setColorDepth(48);

    if (numberOfComponents == 3) {
        // Note: Adobe nonstandard header uses code 0 to specify both CMYK and RGB
        if (adobeColorModel == Format::CMYK) {
            f.setColorModel(Format::RGB);
            metadata["Color model"] = "RGB";
        }
    }

    else if (numberOfComponents == 4) {
        // Adobe proprietary extension
        if (adobeColorModel == Format::CMYK) {
            f.setColorModel(Format::CMYK);
            metadata["Color model"] = "CMYK";

            if (precision <= 8)
                f.setColorDepth(32);
            else
                f.setColorDepth(64);
            f.setEncodingType(Format::COMPONENTWORD); // This is faster

        } else {
            qDebug() << "Specified 4 components, but no Adobe header!";
        }
    }

    else if (numberOfComponents == 1) {
        f.setColorModel(Format::GRAYSCALE);
        if (precision <= 8)
            f.setColorDepth(8);
        else
            f.setColorDepth(16);
//        f.setEncodingType(Format::COMPONENTWORD); // This is faster
    }

    else {
        qDebug() << "Specified "<<numberOfComponents<<" components, but no Adobe header!";
        // We will use JPEG standard (24-bit YCbCr) and hope that this is a glitch...
    }

    imagePtr->setFormat(f);

    imagePtr->setSize(pictureWidth, pictureHeight);
    imagePtr->activeLayer()->setSize(pictureWidth, pictureHeight);*/

#if DEBUGLEVEL>0
    qDebug()<<"Image height is: "<<pictureHeight<<" and image width is: "<< pictureWidth <<endl;
#endif
    imageWidth=pictureWidth; imageHeight=pictureHeight;
}



void JpegDecode::readQuantizationTables(QFile &picture) {
    //First two bytes from this point is Quantization Table length
    //Third byte is divided in two nibbles.
        //First nible is Quantization element table precision 8 or 16 bits
        //Second nibble is Quantization table destination identifier
    //Next 64 bytes are Quantization table element if the size of elements are 8 bits, or 128 bytes if the size of elements is two bytes

    unsigned short tableLength;//Two bytes
    unsigned char tableOptions=0;
    unsigned char tableIdentifier=0;
    unsigned char sizeOfElements=0;
    int tableData[64]; // 64 elements per table

    picture.read ((char*)&tableLength, sizeof(tableLength));
    tableLength = qFromBigEndian(tableLength);

    int m=2;
    while(m<tableLength) {
        picture.read ((char*)&tableOptions, sizeof(tableOptions));
        sizeOfElements  = tableOptions>>4;     // Higher 4 bits of byte
        tableIdentifier = tableOptions & 0x0F; // Lower 4 bits of byte

        QuantizationTable t(false, false);
        t.tableID = tableIdentifier;

        // Elements are represented by 8 bits
        if (sizeOfElements == 0) {
            unsigned char el;
            for (int i=0; i<64; i++) {
                picture.read ((char*)&el, sizeof(el));
                tableData[i] = el;
            }
            m=m+65;
        }
        // 16-bit elements
        else if (sizeOfElements==1) {
            unsigned short el;
            for (int i=0; i<64; i++) {
                picture.read ((char*)&el, sizeof(el));
                el = qFromBigEndian(el);
                tableData[i] = el;
            }
            m=m+129;
        }

        inverseZigZagCoding (tableData, t.quantizationTableData);

        quantizationTables.push_back(t);

#if DEBUGLEVEL>0
        qDebug() << "Read quantization table "<<tableIdentifier<<" depth "<<sizeOfElements;
#endif
    }
}


/*void JpegDecode::readHuffmanTables(QFile &picture) {
    //First two bytes are Huffman table length
    //Third byte is separated in two nibbles. First nibble is Table Class, second nibble is destination identifier or ID
    //Next 16 bytes are number of elements coded with 1-16 bits

    unsigned short tableLength; // First two bytes-table length
    unsigned char  tableID = 0; // Specifies one of component: 0 for luminance and 1 for chrominance
    unsigned char  tableClass = 0; // Specifies is it DC element or AC element of table. 0-DC element 1-AC element
    unsigned char  huffmanTableOptions = 0; // I will decompose this in two nibbles

    picture.read ((char*)&tableLength, sizeof(tableLength));
    tableLength = qFromBigEndian(tableLength);


    int m=2;
    while (m < tableLength) {
        picture.read((char*)&huffmanTableOptions,sizeof(huffmanTableOptions));
        m++;
        tableID = huffmanTableOptions & 0x0F;
        tableClass = huffmanTableOptions >> 4;

        // Looking for tableID in tables
//        HuffmanTable* table = 0;
        HuffmanTree* tree = 0;
        for (uint i=0; i<huffmanTrees.size(); i++) {
/*            HuffmanTable* t = huffmanTables[i];
            if (t->tableID == tableID && t->tableClass == tableClass) {
                table = huffmanTables[i];
                //break;
            }*//*
            HuffmanTree* t2 = huffmanTrees[i];
            if (t2->tableID == tableID && t2->tableClass == tableClass) {
                tree = huffmanTrees[i];
                //break;
            }
        }

        // Not found, create a new table
/*        if (table == 0) {
            table = new HuffmanTable();
            table->tableID = tableID;
            table->tableClass = tableClass;
            huffmanTables.push_back(table);
        }*//*
        if (tree == 0) {
//            gettimeofday(&start, NULL);
            tree = new HuffmanTree();
            tree->tableID = tableID;
            tree->tableClass = tableClass;
            for (int i=0; i<65536; i++) tree->huffmanShortLUT[i]=0;
            for (int i=0; i<65536; i++) tree->huffmanShortLUT2[i]=0;
            //memset(tree->huffmanShortLUT, 0, sizeof(HuffmanNode*)*65536);
            huffmanTrees.push_back(tree);
//            gettimeofday(&end, NULL);
//            quantizeTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
        }

        unsigned char codeLengths[16];
        for(int i=0;i<16;i++)
            codeLengths[i]=0;

        unsigned long int code=0;//Huffman code which will be connected with element
        unsigned char element=0;//Read element from file

        // Read 16 bytes for number of elements which are coded by 1-16 bits
        picture.read ((char*)codeLengths, sizeof(codeLengths));
        m+=16;

        for (int i=0; i<16; i++) { //We iterate through every element
//            cout << "i="<<i<<" codeLengths[i]="<<int(codeLengths[i])<<endl;
            if (codeLengths[i]==0) { //If code length is 0, then we continue because no element is coded wiht "i" bits
                code *= 2; // When tree depth changes, we add additional bit, and shift code one place left
//                cout << "code becomes "<<code<<endl;
                continue;
            }
            for (int j=0; j<codeLengths[i]; j++) { //If there is at least one element coded with "i" bits, then we have to assign code to it
//                cout << "code="<<code<<endl;
                picture.read ((char*)&element, sizeof(element));
                m++;

//                table->codes[element] = code;
//                table->codeLengths[element] = i+1;

                tree->codes[element] = code;
                tree->codeLengths[element] = i+1;
                //tree->addNode(code, i+1, element);

                // Add shorter codes to lookup table
//                if (i < 16) {
                    int idx = code + (1 << (i+1));
                    int rightaligned = (code << (15-i));
#if DEBUGLEVEL>1
                    cout << "Adding code "<<hex<<rightaligned<<" to LUT for "<<code<<" ("<<idx<<")"<<dec<<endl;
#endif
                    HuffmanNode *tmpnode = new HuffmanNode;
                    tmpnode->codeValue = code;
                    tmpnode->codeLength = i+1;
                    tmpnode->dataValue = element;

                    tree->huffmanShortLUT[rightaligned] = tmpnode;
                    if (idx < 65536)
                    tree->huffmanShortLUT2[idx] = tmpnode;
                //}
                code++; //Elements on the same tree depth have code incremented by one
            }

            code *= 2; //When tree depth changes, we add additional bit, and shift code one place left
//            cout << "code*2 ="<<code<<endl;
        }
#if DEBUGLEVEL>0
        qDebug() << "Read Huffman table ID: " << tableID << "class" << tableClass;
#endif
    }
}*/

void JpegDecode::readHuffmanTables(QFile &picture) {
    //First two bytes are Huffman table length
    //Third byte is separated in two nibbles. First nibble is Table Class, second nibble is destination identifier or ID
    //Next 16 bytes are number of elements coded with 1-16 bits

    unsigned short tableLength; // First two bytes-table length
    unsigned char  tableID = 0; // Specifies one of component: 0 for luminance and 1 for chrominance
    unsigned char  tableClass = 0; // Specifies is it DC element or AC element of table. 0-DC element 1-AC element
    unsigned char  huffmanTableOptions = 0; // I will decompose this in two nibbles

    picture.read ((char*)&tableLength, sizeof(tableLength));
    tableLength = qFromBigEndian(tableLength);

    int m=2;
    while (m < tableLength) {
        picture.read((char*)&huffmanTableOptions,sizeof(huffmanTableOptions));
        m++;
        tableID = huffmanTableOptions & 0x0F;
        tableClass = huffmanTableOptions >> 4;

        // Looking for tableID in tables
        HuffmanTable* table = 0;
        for (uint i=0; i<huffmanTables.size(); i++) {
            HuffmanTable* t = huffmanTables[i];
            if (t->tableID == tableID && t->tableClass == tableClass) {
                table = huffmanTables[i];
                break;
            }
        }

        // Not found, create a new table
        if (table == 0) {
            table = new HuffmanTable();
            table->tableID = tableID;
            table->tableClass = tableClass;
            huffmanTables.push_back(table);
        }

        unsigned char codeLengths[16];
        for(int i=0;i<16;i++)
            codeLengths[i]=0;

        unsigned long int code=0;//Huffman code which will be connected with element
        unsigned char element=0;//Read element from file

        // Read 16 bytes for number of elements which are coded by 1-16 bits
        picture.read ((char*)codeLengths, sizeof(codeLengths));
        m+=16;

        for (int i=0; i<16; i++) { //We iterate through every element
            if (codeLengths[i]==0) { //If code length is 0, then we continue because no element is coded wiht "i" bits
                code *= 2; // When tree depth changes, we add additional bit, and shift code one place left
                continue;
            }
            for (int j=0; j<codeLengths[i]; j++) { //If there is at least one element coded with "i" bits, then we have to assign code to it
                picture.read ((char*)&element, sizeof(element));
                m++;

                table->codes[element] = code;
                table->codeLengths[element] = i+1;
                code++; //Elements on the same tree depth have code incremented by one
            }

            code *= 2; //When tree depth changes, we add additional bit, and shift code one place left
        }
#if DEBUGLEVEL>0
        qDebug() << "Read Huffman table ID: " << tableID << "class" << tableClass;
#endif
    }
}



void JpegDecode::readArithmeticCoding(QFile &picture) {
    // Arithmetic coding not supported yet
    // Just skip data
    unsigned short len;
    unsigned char c;
    picture.read ((char*)&len, sizeof(len));
    len = qFromBigEndian(len);
    for (int i=0; i<len-2; i++)
        picture.read ((char*)&c, sizeof(c));

    qDebug() << "DAC found (0xCC) - Arithmetic coding not supported yet";
}



void JpegDecode::readScanHeader(QFile &picture) {
    /* --- Entropy-coded image data
       After FF DA marker starts header followed by data
       Header contains:
           - Header length (not data, just header)
           - Number of color components e.g. 3 for Y'CbCr, 4 for CMYK
           - Component IDs (2 bytes per component)
           - ZigZag definition (3 bytes)
       After header immediately comes data ending with marker FF D9
    */

    if (components.size() == 0) {
        throw "SOF header not present or specifies 0 components.";
    }

    unsigned short headerLength;
    unsigned char numberOfComponents;
    picture.read ((char*)&headerLength, sizeof(headerLength));
    picture.read ((char*)&numberOfComponents, sizeof(numberOfComponents));
    headerLength = qFromBigEndian(headerLength);

    if (headerLength != numberOfComponents*2 + 6)
        qDebug() << "Bad SOS header length: expected"<<(numberOfComponents*2+6)<<"but given"<<headerLength;

    if (numberOfComponents != this->components.size()) {
        qDebug() << "Number of components in SOS header is" << numberOfComponents << "but in SOF header is" << this->components.size();
        qDebug() << "We will trust SOF header, but this probably wont work :(";
        numberOfComponents = this->components.size();
    }

    if (numberOfComponents > ETF_FORMAT_MAX_COMPONENTS) {
        qDebug() << "Specified"<<numberOfComponents<<"components, maximum is "<<ETF_FORMAT_MAX_COMPONENTS;
        numberOfComponents = ETF_FORMAT_MAX_COMPONENTS;
    }

    // Read components and tables
    for (int i=0; i < numberOfComponents; i++) {
        unsigned char componentID, tableID;
        picture.read ((char*)&componentID, sizeof(componentID));
        picture.read ((char*)&tableID, sizeof(tableID));

        // Check component in components list
        if (this->components[i].componentID != componentID) {
            qDebug() << "Component ID in SOS header is" << componentID << "but in SOF header is" << this->components[i].componentID;
            qDebug() << "We will trust SOF header";
        }

        // Find AC and DC Huffman table in tables list
        unsigned char tableDC = tableID >> 4;
        unsigned char tableAC = tableID & 0x0F;
        componentTablesDC[i] = componentTablesAC[i] = 0;
        componentTreesDC[i] = componentTreesAC[i] = 0;

        for (uint j=0; j<huffmanTables.size(); j++) {
            HuffmanTable* t = huffmanTables[j];
            if (t->tableID == tableDC && t->tableClass == 0)
                componentTablesDC[i] = t;
            if (t->tableID == tableAC && t->tableClass == 1)
                componentTablesAC[i] = t;
            /*HuffmanTree* t2 = huffmanTrees[j];
            if (t2->tableID == tableDC && t2->tableClass == 0)
                componentTreesDC[i] = t2;
            if (t2->tableID == tableAC && t2->tableClass == 1)
                componentTreesAC[i] = t2;*/
        }

        // Huffman tables not found
        if (componentTablesDC[i] == 0) {
            // Get first usable table
            for (uint j=0; j<huffmanTables.size(); j++) {
                HuffmanTable* t = huffmanTables[j];
                if (t->tableClass == 0) {
                    componentTablesDC[i] = t;
                    break;
                }
            }

            if (componentTablesDC[i] == 0)
                throw "File contains no DC Huffman tables!";

            qDebug() << "SOS header specifies inexistant Huffman table" << tableDC << " - using " << componentTablesDC[i]->tableID;
        }

        if (componentTablesAC[i] == 0) {
            // Get first usable table
            for (uint j=0; j<huffmanTables.size(); j++) {
                HuffmanTable* t = huffmanTables[j];
                if (t->tableClass == 1) {
                    componentTablesAC[i] = t;
                    break;
                }
            }
            if (componentTablesAC[i] == 0) {
                // throw "File contains no AC Huffman tables!"; // Can happen in progressive JPEG!
                componentTablesAC[i] = componentTablesDC[i];
            }

            qDebug() << "SOS header specifies inexistant Huffman table" << tableAC << " - using " << componentTablesAC[i]->tableID;
        }



        /*// Huffman tables not found
        if (componentTreesDC[i] == 0) {
            // Get first usable table
            for (uint j=0; j<huffmanTrees.size(); j++) {
                HuffmanTree* t = huffmanTrees[j];
                if (t->tableClass == 0) {
                    componentTreesDC[i] = t;
                    break;
                }
            }

            if (componentTreesDC[i] == 0)
                throw "File contains no DC Huffman tables!";

            qDebug() << "SOS header specifies inexistant Huffman table" << tableDC << " - using " << componentTreesDC[i]->tableID;
        }

        if (componentTreesAC[i] == 0) {
            // Get first usable table
            for (uint j=0; j<huffmanTrees.size(); j++) {
                HuffmanTree* t = huffmanTrees[j];
                if (t->tableClass == 1) {
                    componentTreesAC[i] = t;
                    break;
                }
            }
            if (componentTreesAC[i] == 0) {
                // throw "File contains no AC Huffman tables!"; // Can happen in progressive JPEG!
                componentTreesAC[i] = componentTreesDC[i];
            }

            qDebug() << "SOS header specifies inexistant Huffman table" << tableAC << " - using " << componentTreesAC[i]->tableID;
        }*/
    }

    // Start and end point for zig-zag coding
    picture.read ((char*)&zigZagStart, sizeof(zigZagStart));
    picture.read ((char*)&zigZagEnd,   sizeof(zigZagEnd));

    // Bit approximation for progressive JPEG - TODO!
    unsigned char dummy;
    picture.read ((char*)&dummy, sizeof(dummy));
    approximationH = dummy >> 4;
    approximationL = dummy & 0x0F;
}



void JpegDecode::readImageData(QFile &file) {
    uint currentComponent = 0;
//    gettimeofday(&start, NULL);
    int counter2 = 1; // Counter for chroma subsampling
    int row[64], row2[8][8], row3[8][8];
    int blkNo = 0;

    // Statistics of last DC value
    for (uint i=0; i<components.size(); i++)
        previousDC[i]=0;
	
    if (losslessFormat) {
        // Predictor requires to hold last line in memory
        for (uint i=0; i<components.size(); i++)
            scanLineCache[i] = new int[imageWidth];
    }

    // Benchmarking

    clock_t t1, t2, t3, t4, t5, t6;

    // CBIR!
    for (uint i=0; i<components.size(); i++) {
        colorFeatures[i][0] = colorFeatures[i][1] = colorFeatures[i][2] = colorFeatures[i][3] = 0;
        colorFeaturesLast1000[i][0] = colorFeaturesLast1000[i][1] = colorFeaturesLast1000[i][2] = colorFeaturesLast1000[i][3] = 0;
        colorFeaturesCounters[i] = 0;
        colorFeaturesDoubleCounters[i] = 0;
    }
    for (uint i=0; i<components.size(); i++) {
        for (int j=0; j<6; j++) {
            specificBlocks[i][j] = 0;
            specificBlocksLast1000[i][j] = 0;
            specificBlocksSquares[i][j] = 0;
            specificBlocksSquaresLast1000[i][j] = 0;
        }
    }



    // Huffman data
    data = 0;
    currentDataLength = 0;
    byteno = 0;

    // Add block data
    for (uint i=0; i<components.size(); i++) {
        currentDataBlockX[i] = 0;
        currentDataBlockY[i] = 0;
    }


#if DEBUGLEVEL>0
    qDebug()<<"Start of scan";
#endif

//    char * rawdata = new char[55000];
//    rawdatasize = file.read(rawdata, 55000);

    endOfFile = false;
    bufpos=65536; bufsubpos=0;

    while (!endOfFile) {
#if DEBUGLEVEL>1
        qDebug()<<"--- Huffman read block "<<blkNo<<" component "<<currentComponent;
#endif


//        gettimeofday(&start, NULL);
        readHuffmanBlock(file, row, currentComponent);
//        gettimeofday(&end, NULL);
//        huffmanTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
//        start=end;

			// DC predictor
            previousDC[currentComponent] = row[0];

            //inverseZigZagCoding (row, row2);

            //multiplyWithQuantizationTable (row2, currentComponent);

            /* Instead of zig-zag coding we will use this table
            0  1  5  6 14 15 27 28
            2  4  7 13 16 26 29 42
            3  8 12 17 25 30 41 43
            9 11 18 24 31 40 44 53
           10 19 23 32 39 45 52 54
           20 22 33 38 46 51 55 60
           21 34 37 47 50 56 59 61
           35 36 48 49 57 58 62 63 */

            // Coefficients from formula (3) from paper
            int M11 = (row[0] + row[1] + row[2] + row[4]) >> 4;
            int M12 = (row[0] + row[1] - row[2] - row[4]) >> 4;
            int M21 = (row[0] - row[1] + row[2] - row[4]) >> 4;
            int M22 = (row[0] - row[1] - row[2] + row[4]) >> 4;

            colorFeaturesLast1000[currentComponent][0] += M11;
            colorFeaturesLast1000[currentComponent][1] += M12;
            colorFeaturesLast1000[currentComponent][2] += M21;
            colorFeaturesLast1000[currentComponent][3] += M22;

            colorFeaturesCounters[currentComponent]++;

#if DEBUGLEVEL>1
            cout <<"row[0]="<<row[0]<<" row[1]="<<row[1]<<" row[2]="<<row[2]<<" row[3]="<<row[3]<<endl;
            cout <<"M11="<<M11<<" M12="<<M12<<" M21="<<M21<<" M22="<<M22<<endl;
            cout <<"DEBUG ("<<currentComponent<<","<<colorFeaturesCounters[currentComponent]<<"): "<<colorFeaturesLast1000[currentComponent][0]<<" "<<colorFeaturesLast1000[currentComponent][1]<<" "<<colorFeaturesLast1000[currentComponent][2]<<" "<<colorFeaturesLast1000[currentComponent][3]<<endl;
#endif

            // Coefficients displayed on Figure 3 from paper
            int texture1 = row[0];
            int texture2 = row[1] + row[2] + row[3] + row[4] + row[5];
            int texture3 = 0;
            for (int i=6; i<15; i++) texture3 += row[i];

            int texture4 = /*row[0] +*/ row[2] + row[3] + row[9] + row[10] + row[20] + row[21] + row[35]; // Error in paper
            int texture5 = row[1] + row[5] + row[6] + row[14] + row[15] + row[27] + row[28];
            int texture6 = row[4] + row[12] + row[24] + row[39] + row[51] + row[59];

            specificBlocksLast1000[currentComponent][0] += texture1;
            specificBlocksLast1000[currentComponent][1] += texture2;
            specificBlocksLast1000[currentComponent][2] += texture3;
            specificBlocksLast1000[currentComponent][3] += texture4;
            specificBlocksLast1000[currentComponent][4] += texture5;
            specificBlocksLast1000[currentComponent][5] += texture6;

            specificBlocksSquaresLast1000[currentComponent][0] += texture1*texture1;
            specificBlocksSquaresLast1000[currentComponent][1] += texture2*texture2;
            specificBlocksSquaresLast1000[currentComponent][2] += texture3*texture3;
            specificBlocksSquaresLast1000[currentComponent][3] += texture4*texture4;
            specificBlocksSquaresLast1000[currentComponent][4] += texture5*texture5;
            specificBlocksSquaresLast1000[currentComponent][5] += texture6*texture6;

            // Recalculating average every 1024 blocks so that sums wouldn't overflow (especially the squares sum)
            if (colorFeaturesCounters[currentComponent] == 1024) {
                cout << "dio: ";
                for (int i=0; i<4; i++) {
                    double last1000Average = double(colorFeaturesLast1000[currentComponent][i]) / 1024;
                    colorFeatures[currentComponent][i] *= colorFeaturesDoubleCounters[currentComponent];
                    colorFeatures[currentComponent][i] += last1000Average;
                    colorFeatures[currentComponent][i] /= (colorFeaturesDoubleCounters[currentComponent] + 1);
#if DEBUGLEVEL>1
            printf ("AVERAGE %lf %lf\n", last1000Average, colorFeatures[currentComponent][i]);
#endif
                    colorFeaturesLast1000[currentComponent][i] = 0;
                }
//                cout << endl;
                colorFeaturesDoubleCounters[currentComponent]++;
                colorFeaturesCounters[currentComponent] = 0;

                for (int i=0; i<6; i++) {
                    double last1000Average = double(specificBlocksLast1000[currentComponent][i]) / 1024;
                    specificBlocks[currentComponent][i] *= colorFeaturesDoubleCounters[currentComponent];
                    specificBlocks[currentComponent][i] += last1000Average;
                    specificBlocks[currentComponent][i] /= (colorFeaturesDoubleCounters[currentComponent] + 1);
                    specificBlocksLast1000[currentComponent][i] = 0;
                }
                for (int i=0; i<6; i++) {
                    double last1000Average = double(specificBlocksSquaresLast1000[currentComponent][i]) / 1024;
                    specificBlocksSquares[currentComponent][i] *= colorFeaturesDoubleCounters[currentComponent];
                    specificBlocksSquares[currentComponent][i] += last1000Average;
                    specificBlocksSquares[currentComponent][i] /= (colorFeaturesDoubleCounters[currentComponent] + 1);
                    specificBlocksSquaresLast1000[currentComponent][i] = 0;
                }


                /*for (int i=0; i<4; i++) {
                    cout << colorFeatures[currentComponent][i] << ",";
                }
                cout << endl;
                for (int i=0; i<6; i++) {
                    cout << specificBlocks[currentComponent][i] << ",";
                }
                cout << endl;
                for (int i=0; i<6; i++) {
                    cout << specificBlocksSquares[currentComponent][i] << ",";
                }
                cout << endl;
                return;*/
            }


            //dumpBlock(row2);

            /*dumpBlock(row2);
            for (int i=0; i<4; i++) {
                cout << colorFeaturesLast1000[currentComponent][i] << ",";
            }
            cout << endl;
            for (int i=0; i<6; i++) {
                cout << specificBlocksLast1000[currentComponent][i] << ",";
            }
            cout << endl;
            for (int i=0; i<6; i++) {
                cout << specificBlocksSquaresLast1000[currentComponent][i] << ",";
            }
            cout << endl;
            return;*/

//            gettimeofday(&end, NULL);
//            quantizeTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);

            //IDCT (row2, row3);
            //t5=clock();

            // Adding data to ImageState
            addNewData8x8Block(row2, currentComponent);
            //t6=clock();
//            gettimeofday(&start, NULL);
//            addBlockTime += ((start.tv_sec  - end.tv_sec) * 1000000 + start.tv_usec - end.tv_usec);

        // Is there component subsampling ?
        if (counter2 < this->components[currentComponent].HFactor * this->components[currentComponent].VFactor)
            counter2++;
        else {
            counter2 = 1;
            currentComponent++;
            if (currentComponent == components.size()) currentComponent=0;
        }
        blkNo++;

        // Benchmarking
#ifdef BENCHMARK
        huffmanTime += (t2-t1) / (CLOCKS_PER_SEC / 1000);
        zigZagTime += (t3-t2) / (CLOCKS_PER_SEC / 1000);
        quantizeTime += (t4-t3) / (CLOCKS_PER_SEC / 1000);
        idctTime += (t5-t4) / (CLOCKS_PER_SEC / 1000);
        addBlockTime += (t6-t5) / (CLOCKS_PER_SEC / 1000);
        if (blkNo % 10000 == 0) {
            qDebug() << "Huffmannn "<<huffmanTime<<" ms, ZigZag "<<zigZagTime<<" ms, Quantize "<<quantizeTime<<" ms, IDCT "<<idctTime<<" ms, Add Block "<<addBlockTime<<" ms";
        }
#endif
    }

    for (uint j=0; j<components.size(); j++) {
//        cout << "Component: "<<j<<endl;
        for (int i=0; i<4; i++) {
            double last1000Average = double(colorFeaturesLast1000[j][i]) / 1024;
            colorFeatures[j][i] *= colorFeaturesDoubleCounters[j];
            colorFeatures[j][i] += last1000Average;
            double divisor = double (colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
            colorFeatures[j][i] /= divisor;
//            cout << "  M=" << colorFeatures[j][i];
            featureVectorME[j][i] = colorFeatures[j][i];
        }
//        cout << endl;
        for (int i=0; i<6; i++) {
            double last1000Average = double(specificBlocksLast1000[j][i]) / 1024;
            specificBlocks[j][i] *= colorFeaturesDoubleCounters[j];
            specificBlocks[j][i] += last1000Average;
            double divisor = double (colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
            specificBlocks[j][i] /= divisor;
//            cout << "  D=" << specificBlocks[j][i];
            featureVectorD[j][i] = specificBlocks[j][i];
        }
        for (int i=0; i<6; i++) {
            double last1000Average = double(specificBlocksSquaresLast1000[j][i]) / 1024;
            specificBlocksSquares[j][i] *= colorFeaturesDoubleCounters[j];
            specificBlocksSquares[j][i] += last1000Average;
            double divisor = double (colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
            specificBlocksSquares[j][i] /= divisor;

            // Std dev = square root of (mean of squares minus square of mean)
            featureVectorD[j][i+6] = sqrt(fabs(specificBlocksSquares[j][i] - pow(specificBlocks[j][i],2)));
//            cout << "  D=" << featureVectorD[j][i+6];
        }
    }


    if (losslessFormat) {
        for (uint i=0; i<components.size(); i++)
            delete[] scanLineCache[i];
    }
//            gettimeofday(&end, NULL);
//            huffmanTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);

#if DEBUGLEVEL>0
    qDebug()<<"End of scan";
#endif

}


bool JpegDecode::readMoreData(QFile &picture, unsigned int &data, unsigned int &currentDataLength) {

    unsigned char binaryData;

    // Detect errors
    if (currentDataLength > 24) { // Unsigned int can hold at most 32 = 24+8 bits
        cout << "ERROR: Code value not found in Huffman table: "<<data<< " CDL: "<< currentDataLength<<endl;

        // Truncate data one by one bit in hope that we will eventually find a correct code
        data = data - ((data >> (currentDataLength-1)) << (currentDataLength-1));
        currentDataLength--;
        return true;
    }

    if (picture.read((char*)&binaryData, sizeof binaryData) == 0)
        return false; // End of file
//    if (tmpdata - rawdata >= rawdatasize) return false;
//    binaryData = *tmpdata++;

    // We read byte and put it in low 8 bits of variable data
    if (binaryData == 0xFF) {
        data = (data << 8) + binaryData;
        currentDataLength += 8; // Increase current data length for 8 because we read one new byte
        if (picture.read((char*)&binaryData, sizeof binaryData) == 0)
            return false;
//        if (tmpdata - rawdata >= rawdatasize) return false;
//        binaryData = *tmpdata++;

        // End of Image marker
        if (binaryData == 0xd9) {
            // Drop 0xFF from data
            data = data >> 8;
            currentDataLength -= 8;
#if DEBUGLEVEL>1
                cout << "End of image marker"<<endl;
#endif
            return false;
        }

        // Restart marker means data goes blank
        if (binaryData >= 0xd0 && binaryData <= 0xd7) {
#if DEBUGLEVEL>1
            cout << "Restart marker"<<endl;
#endif

            data = 0;
            currentDataLength = 0;
            for (uint i=0; i<components.size(); i++)
                previousDC[i]=0;
        }

        // If after FF byte comes 0x00 byte, we ignore it, 0xFF is part of data (byte stuffing)
        else if (binaryData != 0) {
            data = (data << 8) + binaryData;
            currentDataLength += 8; //Increase current data length for 8 because we read one new byte
#if DEBUGLEVEL>1
            cout << "Stuffing"<<endl;
#endif
        }
    }
    else {
        data = (data << 8) + binaryData;
        currentDataLength += 8;
    }
    return true;
}


/*void JpegDecode::readHuffmanBlock(QFile &picture, int* dataBlock, int currentComponent) {
    // Data bytes must be preserved for next block, so they are marked static
    //qDebug() << "readHuffmanBlock";

    // Debugging
    struct timeval mstart, mend;
//    gettimeofday(&mstart, NULL);

    endOfFile = false;

    // Description of the 8x8 block currently being read
    enum { AC, DC } ACDC = DC;

    // How many AC elements should we read?
    int ACcount = zigZagEnd - zigZagStart;

    int m = 0; // Index into dataBlock

    // Fill block with zeros
    if (!losslessFormat) memset ((char*)dataBlock, 0, sizeof(int)*64);
    HuffmanTree* htree = componentTreesDC[currentComponent];

    HuffmanNode tmpnode;

    HuffmanNode* node = 0; //htree->getNode(data, currentDataLength);

    for (int i=0; i<256; i++) {
        if (htree->codeLengths[i] < currentDataLength) {
            int tmpcode = data >> (currentDataLength - htree->codeLengths[i]);
            if (tmpcode == htree->codes[i]) {
                tmpnode.codeValue = htree->codes[i];
                tmpnode.codeLength = htree->codeLengths[i];
                tmpnode.dataValue = i;
                node = &tmpnode;
                break;
            }
        }
    }


    tmpdata = rawdata;
//    gettimeofday(&mend, NULL);
//    acTime += ((mend.tv_sec  - mstart.tv_sec) * 1000000 + mend.tv_usec - mstart.tv_usec);

    // Main loop
    do {
//        gettimeofday(&mstart, NULL);
        // 3 bits is too small for a code
        if (currentDataLength<3 || node==0) {
            if (!readMoreData(picture, data, currentDataLength)) break;
/*            gettimeofday(&mstart, NULL);
            gettimeofday(&mend, NULL);
            rmdTime += ((mend.tv_sec  - mstart.tv_sec) * 1000000 + mend.tv_usec - mstart.tv_usec);*//*
        }

            // Some stats
        byteno++;
#if DEBUGLEVEL>1
        cout << "Byte "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<endl;
#elif DEBUGLEVEL>0
        if (byteno % 1000 == 0)
            cout << "Byte "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<endl;
#endif

        // Current Huffman table
        /*HuffmanTable* htable = componentTablesDC[currentComponent];
        if (ACDC == AC) htable = componentTablesAC[currentComponent];*//*
        if (ACDC == AC) htree = componentTreesAC[currentComponent];

/*        // Every one of 256 elements of the current Huffman table potentially has value, so we must go through all of them
        for (int i=0; i<256; i++) {
            // If code for i-th element is -1, then there is no Huffman code for i-th element
            if (htable->codes[i] == 0xFFFFFFFF)
                continue;

            // If current data length is greater or equal than n, compare first n bits (n - length of current Huffman code)
            uint n = htable->codeLengths[i];

            if (currentDataLength >= n && htable->codes[i] == data >> (currentDataLength-n)) {*//*
//        gettimeofday(&mstart, NULL);
        gettimeofday(&mend, NULL);
//        acTime += ((mend.tv_sec  - mstart.tv_sec) * 1000000 + mend.tv_usec - mstart.tv_usec);

        // Lookup table for short codes
//        if (node == 0) {
//            HuffmanNode* tmp = 0;


       /* for (int i=1; i<=currentDataLength && i<16; i++) {
            int idx = data >> (currentDataLength - i);
            //cout << "idx="<<idx<<endl;
            idx += (1 << i);
#if DEBUGLEVEL>1
            cout << "(A) Looking for code "<<hex<<idx<<dec<<endl;
#endif
            node = htree->huffmanShortLUT2[idx];
            if (node!=0) { break; }
        }*//*



        int tmpdata, tmpcdl;
        if (currentDataLength >= 16) {
            tmpdata = data >> (currentDataLength-16);
            tmpcdl = 17;
        }
        else {
            tmpdata = data << (16 - currentDataLength);
            tmpcdl = currentDataLength+1;
        }

        for (int i=1; i<tmpcdl; i++) {
            int tmpdata2 = tmpdata & masks[i-1];
#if DEBUGLEVEL>1
            cout << "Looking for code "<<hex<<tmpdata2<<" in LUT ("<<tmpdata<<","<<masks[i-1]<<dec<<","<<i<<")"<<endl;
#endif
            node = htree->huffmanShortLUT[tmpdata2];
            if (node!=0 && node->codeLength==i) {
#if DEBUGLEVEL>1
            cout << "Found "<<hex<<node->codeValue<<dec<<endl;
#endif
                break;
            }
            else if (node!=0) node=0;
        }


//            node=tmp;
//        }
        gettimeofday(&mstart, NULL);
        getNodeTime += ((mstart.tv_sec  - mend.tv_sec) * 1000000 + mstart.tv_usec - mend.tv_usec);

//       mend=mstart;
        if (node != 0) {
#if DEBUGLEVEL>1
                //cout << "Found data "<<hex<<htable->codes[i]<<dec<<" len "<<n<<" at index "<<i<<endl;
                cout << "Found data "<<hex<<node->codeValue<<dec<<" len "<<node->codeLength<<" at index "<<node->dataValue<<endl;
#endif

                // Remove first n bits from data;
                //currentDataLength -= n;
                //data = data - (htable->codes[i] << currentDataLength);
                currentDataLength -= node->codeLength;
                data = data - (node->codeValue << currentDataLength);

                // Reading of DC coefficients
                if (ACDC == DC) {
                    //unsigned char bitLength = i; // Next i bits represent DC coefficient value
                    unsigned char bitLength = node->dataValue; // Next i bits represent DC coefficient value

                    // Do we need to read more bits of data?
                    while (currentDataLength<bitLength) {
                        if (!readMoreData(picture, data, currentDataLength)) {
                            endOfFile = true;
#if DEBUGLEVEL>0
                            qDebug() << "End of file encountered inside a Huffman code!";
#endif
                            break;
                        }
                        byteno++;
#if DEBUGLEVEL>1
                        cout << "Byte(+) "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<endl;
#endif
                    }

                    // Read out DC coefficient
                    int DCCoeficient = data >> (currentDataLength-bitLength);
                    currentDataLength -= bitLength;
                    data = data - (DCCoeficient<<currentDataLength);

                    // If MSB in DC coefficient starts with 0, then substract value of DC with 2^bitlength+1
                    //cout << "Before substract "<<DCCoeficient<<" BL "<<int(bitLength)<<endl;
                    if ( bitLength != 0 && (DCCoeficient>>(bitLength-1)) == 0 ) {
                        DCCoeficient = DCCoeficient - (2 << (bitLength-1)) + 1;
                    }
                    //cout << "After substract "<<DCCoeficient<<" previousDC "<<previousDC[currentComponent]<<endl;

                    dataBlock[m] = DCCoeficient + previousDC[currentComponent];
#if DEBUGLEVEL>1
                    cout << "DC READ "<<dataBlock[m]<<" at index "<<m<<endl;
#endif
                    m++;

                    // No AC coefficients required?
                    if (ACcount == 0 || losslessFormat) return;

                    // We generated our DC coefficient, next one is AC coefficient
                    ACDC = AC;
                    //if (currentDataLength < 3) // If currentData length is < than 3, we need to read new byte, so leave this for loop
                      //  break;
                    //i =- 1; // CurrentDataLength is not zero, set i=0 to start from first element of array
                    //htable = componentTablesAC[currentComponent];
                    //htree = componentTreesAC[currentComponent];
                    //goto fixme;
//                    gettimeofday(&mend, NULL);
//                    dcTime += ((mend.tv_sec  - mstart.tv_sec) * 1000000 + mend.tv_usec - mstart.tv_usec);
                }

                // Reading of AC coefficients
                else {
                    //unsigned char ACElement=i;
                    unsigned char ACElement=node->dataValue;

                    /* Every AC component is composite of 4 bits (RRRRSSSS). R bits tells us relative position of
                       non zero element from the previous non zero element (number of zeros between two non zero elements)
                       SSSS bits tels us magnitude range of AC element
                       Two special values:
                          00 is END OF BLOCK (all AC elements are zeros)
                          F0 is 16 zeroes *//*

                    if (ACElement == 0x00) {
                        return;
                    }

                    else if (ACElement == 0xF0) {
                        for (int k=0;k<16;k++) {
                            dataBlock[m] = 0;
                            m++;
                            if (m >= ACcount+1) {
#if DEBUGLEVEL>0
                                qDebug() << "Huffman error: 16 AC zeros requested, but only "<<k<<" left in block!";
#endif
                                return;
                            }
                        }
                    }
                    else {
                        /* If AC element is 0xAB for example, then we have to separate it in two nibbles
                           First nible is RRRR bits, second are SSSS bits
                           RRRR bits told us how many zero elements are before this element
                           SSSS bits told us how many binary digits our AC element has (if 1001 then we have to read next 9 elements from file) *//*

                        // Let's separate byte to two nibles
                        unsigned char Rbits = ACElement >> 4;
                        unsigned char Sbits = ACElement & 0x0F;

                        // Before our element there is Rbits zero elements
                        for (int k=0; k<Rbits; k++) {
                            if (m >= ACcount) {
#if DEBUGLEVEL>0
                                qDebug() << "Huffman error: "<<Rbits<<" preceeding AC zeros requested, but only "<<k<<" left in block!";
#endif
                                // in case of error, doing the other stuff will just do more errors so return here
                                return;
                            }
                            dataBlock[m] = 0;
                            m++;
                        }

                        // Do we need to read more bits of data?
                        while (currentDataLength<Sbits) {
                            if (!readMoreData(picture, data, currentDataLength)) {
                                endOfFile = true;
#if DEBUGLEVEL>0
                                qDebug() << "End of file encountered inside a Huffman code!";
#endif
                                break;
                            }
                            byteno++;
#if DEBUGLEVEL>1
                            cout << "Byte(+) "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<" Sbits "<<int(Sbits)<<endl;
#endif
                        }

                        // Read out AC coefficient
                        int ACCoeficient = data >> (currentDataLength-Sbits);
                        currentDataLength -= Sbits;
                        data = data - (ACCoeficient<<currentDataLength);

                        // If MSB in AC coefficient starts with 0, then substract value of AC with 2^bitLength+1
                        if ( Sbits != 0 && (ACCoeficient>>(Sbits-1)) == 0 ) {
                            ACCoeficient = ACCoeficient - (2 << (Sbits-1)) + 1;
                        }
                        dataBlock[m] = ACCoeficient;
                        m++;
                    }

                    // End of block
                    if (m >= ACcount+1) {
                        return;
                    }

                    //if (currentDataLength<3) // If currentData length is < 3, we need to read new byte, so leave this for loop
                      //  break;
                    //i =- 1; // currentDataLength is not zero, set i=0 to start from first element of array
                    //goto fixme;
                }

            //}
        }
//        gettimeofday(&mend, NULL);
//        acTime += ((mend.tv_sec  - mstart.tv_sec) * 1000000 + mend.tv_usec - mstart.tv_usec);
    } while(true);

    endOfFile = true; // We reached an end
}*/


void JpegDecode::readHuffmanBlock(QFile &picture, int* dataBlock, int currentComponent) {
    // Data bytes must be preserved for next block, so they are marked static
    static unsigned int data = 0;
    static unsigned int currentDataLength = 0;

    // Debugging
    static unsigned int byteno = 0;

    endOfFile = false;

    // Description of the 8x8 block currently being read
    enum { AC, DC } ACDC = DC;

    // How many AC elements should we read?
    int ACcount = zigZagEnd - zigZagStart;

    int m = 0; // Index into dataBlock

    // Fill block with zeros
    if (!losslessFormat) memset ((char*)dataBlock, 0, sizeof(int)*64);

    // Main loop
    do {
        // 3 bits is too small for a code
        if (currentDataLength<3) continue;

            // Some stats
        byteno++;
#if DEBUGLEVEL>1
        cout << "Byte "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<endl;
#elif DEBUGLEVEL>0
        if (byteno % 1000 == 0)
            cout << "Byte "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<endl;
#endif

        // Current Huffman table
        HuffmanTable* htable = componentTablesDC[currentComponent];
        if (ACDC == AC) htable = componentTablesAC[currentComponent];

        // Every one of 256 elements of the current Huffman table potentially has value, so we must go through all of them
        for (int i=0; i<256; i++) {
            // If code for i-th element is -1, then there is no Huffman code for i-th element
            if (htable->codes[i] == 0xFFFFFFFF)
                continue;

            // If current data length is greater or equal than n, compare first n bits (n - length of current Huffman code)
            uint n = htable->codeLengths[i];

            if (currentDataLength >= n && htable->codes[i] == data >> (currentDataLength-n)) {
#if DEBUGLEVEL>1
                cout << "Found data "<<hex<<htable->codes[i]<<dec<<" len "<<n<<" at index "<<i<<endl;
#endif

                // Remove first n bits from data;
                currentDataLength -= n;
                data = data - (htable->codes[i] << currentDataLength);

                // Reading of DC coefficients
                if (ACDC == DC) {
                    unsigned char bitLength = i; // Next i bits represent DC coefficient value

                    // Do we need to read more bits of data?
                    while (currentDataLength<bitLength) {
                        if (!readMoreData(picture, data, currentDataLength)) {
                            endOfFile = true;
#if DEBUGLEVEL>0
                            qDebug() << "End of file encountered inside a Huffman code!";
#endif
                            break;
                        }
                        byteno++;
#if DEBUGLEVEL>1
                        cout << "Byte(+) "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<endl;
#endif
                    }

                    // Read out DC coefficient
                    int DCCoeficient = data >> (currentDataLength-bitLength);
                    currentDataLength -= bitLength;
                    data = data - (DCCoeficient<<currentDataLength);

                    // If MSB in DC coefficient starts with 0, then substract value of DC with 2^bitlength+1
                    //cout << "Before substract "<<DCCoeficient<<" BL "<<int(bitLength)<<endl;
                    if ( bitLength != 0 && (DCCoeficient>>(bitLength-1)) == 0 ) {
                        DCCoeficient = DCCoeficient - (2 << (bitLength-1)) + 1;
                    }
                    //cout << "After substract "<<DCCoeficient<<" previousDC "<<previousDC[currentComponent]<<endl;

                    dataBlock[m] = DCCoeficient + previousDC[currentComponent];
#if DEBUGLEVEL>1
                    cout << "DC READ "<<dataBlock[m]<<" at index "<<m<<endl;
#endif
                    m++;

                    // No AC coefficients required?
                    if (ACcount == 0 || losslessFormat) return;

                    // We generated our DC coefficient, next one is AC coefficient
                    ACDC = AC;
                    if (currentDataLength < 3) // If currentData length is < than 3, we need to read new byte, so leave this for loop
                        break;
                    i =- 1; // CurrentDataLength is not zero, set i=0 to start from first element of array
                    htable = componentTablesAC[currentComponent];
                }

                // Reading of AC coefficients
                else {
                    unsigned char ACElement=i;

                    /* Every AC component is composite of 4 bits (RRRRSSSS). R bits tells us relative position of
                       non zero element from the previous non zero element (number of zeros between two non zero elements)
                       SSSS bits tels us magnitude range of AC element
                       Two special values:
                          00 is END OF BLOCK (all AC elements are zeros)
                          F0 is 16 zeroes */

                    if (ACElement == 0x00)
                        return;

                    else if (ACElement == 0xF0) {
                        for (int k=0;k<16;k++) {
                            dataBlock[m] = 0;
                            m++;
                            if (m >= ACcount+1) {
#if DEBUGLEVEL>0
                                qDebug() << "Huffman error: 16 AC zeros requested, but only "<<k<<" left in block!";
#endif
                                return;
                            }
                        }
                    }
                    else {
                        /* If AC element is 0xAB for example, then we have to separate it in two nibbles
                           First nible is RRRR bits, second are SSSS bits
                           RRRR bits told us how many zero elements are before this element
                           SSSS bits told us how many binary digits our AC element has (if 1001 then we have to read next 9 elements from file) */

                        // Let's separate byte to two nibles
                        unsigned char Rbits = ACElement >> 4;
                        unsigned char Sbits = ACElement & 0x0F;

                        // Before our element there is Rbits zero elements
                        for (int k=0; k<Rbits; k++) {
                            if (m >= ACcount) {
#if DEBUGLEVEL>0
                                qDebug() << "Huffman error: "<<Rbits<<" preceeding AC zeros requested, but only "<<k<<" left in block!";
#endif
                                // in case of error, doing the other stuff will just do more errors so return here
                                return;
                            }
                            dataBlock[m] = 0;
                            m++;
                        }

                        // Do we need to read more bits of data?
                        while (currentDataLength<Sbits) {
                            if (!readMoreData(picture, data, currentDataLength)) {
                                endOfFile = true;
#if DEBUGLEVEL>0
                                qDebug() << "End of file encountered inside a Huffman code!";
#endif
                                break;
                            }
                            byteno++;
#if DEBUGLEVEL>1
                            cout << "Byte(+) "<<byteno<<" CDL "<<currentDataLength<<" value "<<hex<<data<<dec<<" Sbits "<<int(Sbits)<<endl;
#endif
                        }

                        // Read out AC coefficient
                        int ACCoeficient = data >> (currentDataLength-Sbits);
                        currentDataLength -= Sbits;
                        data = data - (ACCoeficient<<currentDataLength);

                        // If MSB in AC coefficient starts with 0, then substract value of AC with 2^bitLength+1
                        if ( Sbits != 0 && (ACCoeficient>>(Sbits-1)) == 0 ) {
                            ACCoeficient = ACCoeficient - (2 << (Sbits-1)) + 1;
                        }
                        dataBlock[m] = ACCoeficient;
                        m++;
                    }

                    // End of block
                    if (m >= ACcount+1)
                        return;

                    if (currentDataLength<3) // If currentData length is < 3, we need to read new byte, so leave this for loop
                        break;
                    i =- 1; // currentDataLength is not zero, set i=0 to start from first element of array
                }

            }
        }
    } while(readMoreData(picture, data, currentDataLength));

    endOfFile = true; // We reached an end
}



void JpegDecode::multiplyWithQuantizationTable(int dataBlock[8][8], int currentComponent) {
    QuantizationTable* table = this->components[currentComponent].componentQuantizationTable;

    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            dataBlock[i][j] = dataBlock[i][j] * table->quantizationTableData[i][j];
        }
    }
}

void JpegDecode::addNewData8x8Block(int dataBlock[8][8], int currentComponent) {
    // Current X and Y within picture for each component

    unsigned int X = currentDataBlockX[currentComponent];
    unsigned int Y = currentDataBlockY[currentComponent];
    static int currentBlockHFactor=0, currentBlockVFactor=0;
	static int maxSample = pow(2, precision)-1;
//	static const uchar* data = imagePtr->activeLayer()->getData();

//    cout << "Adding block at "<<X<<","<<Y<<" component "<<currentComponent<<endl;

    // Is sub-sampling needed?
    int maxHFactor=1, maxVFactor=1;
    for (uint i=0; i<components.size(); i++) {
        if (components[i].HFactor > maxHFactor) maxHFactor = components[i].HFactor;
        if (components[i].VFactor > maxVFactor) maxVFactor = components[i].VFactor;
    }
    // Non-integer subsamplings are not supported
    int HFactor = maxHFactor / components[currentComponent].HFactor;
    int VFactor = maxVFactor / components[currentComponent].VFactor;

    // Iterate through layer in imagePtr and put pixels
    /*LayerPtr layer = imagePtr->activeLayer();
    for (uint y=0; y<8; y++) {
        if (X >= imagePtr->width()) break;
        if (Y+y >= imagePtr->height()) break;

        // Repeat each line VFactor times
        for (int vfy=0; vfy<VFactor; vfy++) {
            int imageY = Y+y*VFactor+vfy;
            if (imageY >= imagePtr->height()) break;

            Pixel p = layer->getPixelAt(X, imageY);
            for (uint x=0; x<8; x++) {
                if (X+x*HFactor >= imagePtr->width()) break;

                // Round value - x and y are reverted compared to image
                int value = dataBlock[y][x];
                if (value<0) value=0;
                if (value>maxSample) value=maxSample;

                // Hack: Adobe JPEG CMYK format is actually inverted
                if (imagePtr->format().getColorModel() == Format::CMYK)
                    value=maxSample-value;
				
                // ETFShop doesn't support 12-bit precision so we're using 16-bit
                if (precision == 12)
                    value = value << 4;

                // Repeat each pixel HFactor times
                int realx=X+x*HFactor;
                for (int i=0; i<HFactor; i++) {
                    p.setComponent(currentComponent, value);
                    p.next();
                    if (++realx >= imagePtr->width()) break;
                }
            }
        }
    }*/
	

    // Update starting X and Y for next block, taking into account subsampling
    currentDataBlockX[currentComponent] += 8 * HFactor;
    currentBlockHFactor++;
    if (currentBlockHFactor >= components[currentComponent].HFactor) {
        currentDataBlockX[currentComponent] -= 8 * HFactor * components[currentComponent].HFactor;
        currentBlockHFactor = 0;

        currentDataBlockY[currentComponent] += 8 * VFactor;
        currentBlockVFactor++;

        if (currentBlockVFactor >= components[currentComponent].VFactor) {
            currentDataBlockY[currentComponent] -= 8  * VFactor * components[currentComponent].VFactor;
            currentBlockVFactor = 0;

            currentDataBlockX[currentComponent] += 8 * HFactor * components[currentComponent].HFactor;
            if (currentDataBlockX[currentComponent] >= imageWidth) {
                currentDataBlockX[currentComponent] = 0;
                currentDataBlockY[currentComponent] += 8  * VFactor * components[currentComponent].VFactor;

                // Force end here because Progressive JPEG file has more stuff after this
                if (currentDataBlockY[currentComponent] >= imageHeight && currentComponent == components.size()-1)
                    endOfFile = true;
            }
        }
    }
}


void JpegDecode::inverseZigZagCoding(char *array, unsigned char** matrix){
    //k- is array index, i,j are index of matrix
    matrix[0][0]=array[0];//Take the first element
    int k=1;//Inex of array set to second element
    int i=0,j=1;//Define index for matrix
    while(1){
        while(j!=0 && i!=7){//Going upside down until j!=0
           matrix[i][j]=array[k];
           i=i+1;
           j=j-1;
           k=k+1;
        }
        matrix[i][j]=array[k];//Take the edge element
        k=k+1;//Increment array index

        if(i<7)//If not last row, increment i
            i=i+1;

        else if(i==7)//If we hit the last row, we go right one place
            j=j+1;


        while(i!=0 && j!=7){//Going bottom up
            matrix[i][j]=array[k];
            i=i-1;
            j=j+1;
            k=k+1;
        }
        matrix[i][j]=array[k];//Take edge element
        k=k+1;//Increment array index
        if(j<7)//If we didn't hit the edge, increment j
            j=j+1;

        else if(j==7)//If we hit the last element, go down one place
            i=i+1;

        if(i>=7 && j>=7)//If we hit last element matrix[8][8] exit
            break;
    }
}

void JpegDecode::inverseZigZagCoding(int *array, int matrix[8][8] ) {
    int matrixX[64] = {0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 5, 6, 7, 7, 6, 7};
    int matrixY[64] = {0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 4, 5, 6, 7, 7, 6, 5, 6, 7, 7};


    for (int i=zigZagStart; i<=zigZagEnd; i++)
//        (*matrix)[matrixY[i]][matrixX[i]] = array[i-zigZagStart];
        matrix[matrixY[i]][matrixX[i]] = array[i-zigZagStart];
	
/*    //k- is array index, i,j are index of matrix
    (*matrix)[0][0]=array[0];//Take the first element
    unsigned int k=1;//Inex of array set to second element
    unsigned int i=0,j=1;//Define index for matrix
    while(1){
        while(j!=0 && i!=7){//Going upside down until j!=0
          (*matrix)[i][j]=array[k];
           i=i+1;
           j=j-1;
           k=k+1;
        }
       (*matrix)[i][j]=array[k];//Take the edge element
        k=k+1;//Increment array index

        if(i<7)//If not last row, increment i
            i=i+1;

        else if(i==7)//If we hit the last row, we go right one place
            j=j+1;


        while(i!=0 && j!=7){//Going bottom up
            (*matrix)[i][j]=array[k];
            i=i-1;
            j=j+1;
            k=k+1;
        }
        (*matrix)[i][j]=array[k];//Take edge element
        k=k+1;//Increment array index
        if(j<7)//If we didn't hit the edge, increment j
            j=j+1;

        else if(j==7)//If we hit the last element, go down one place
            i=i+1;

        if(i>=7 && j>=7)//If we hit last element matrix[8][8] exit
            break;
    }*/
}

/*void JpegDecode::inverseZigZagCoding(char *array, vector<vector<int> > &matrix ){

    //k- is array index, i,j are index of matrix
    matrix[0][0]=array[0];//Take the first element
    int k=1;//Inex of array set to second element
    int i=0,j=1;//Define index for matrix
    while(1){
        while(j!=0 && i!=7){//Going upside down until j!=0
          matrix[i][j]=array[k];
           i=i+1;
           j=j-1;
           k=k+1;
        }
        matrix[i][j]=array[k];//Take the edge element
        k=k+1;//Increment array index

        if(i<7)//If not last row, increment i
            i=i+1;

        else if(i==7)//If we hit the last row, we go right one place
            j=j+1;


        while(i!=0 && j!=7){//Going bottom up
            matrix[i][j]=array[k];
            i=i-1;
            j=j+1;
            k=k+1;
        }
        matrix[i][j]=array[k];//Take edge element
        k=k+1;//Increment array index
        if(j<7)//If we didn't hit the edge, increment j
            j=j+1;

        else if(j==7)//If we hit the last element, go down one place
            i=i+1;

        if(i>=7 && j>=7)//If we hit last element matrix[8][8] exit
            break;
    }
}*/

void JpegDecode::IDCT (int block[8][8], int transformedBlock[8][8]) {
    double IDCTValue=0;
    double sum=0;

    static double cosine[106];
    static double coefficients[4096];
    static bool firstTime=true;
    if (firstTime) {
        firstTime = false;
        for (int i=0; i<106; i++)
             cosine[i] = cos(i*pi/16);
        for(int i=0;i<8;i++){
            for(int j=0;j<8;j++){
                for(int u=0;u<8;u++){
                    for(int v=0;v<8;v++){
                        int index = i*512 + j*64 + u*8+v;
                        coefficients[index] = cosine[(2*i+1)*u] * cosine[(2*j+1)*v];
                        if (u==0 && v==0)
                            coefficients[index] /= 8;
                        else if (u>0 && v>0)
                            coefficients[index] /= 4;
                        else
                            coefficients[index] *= 0.1767766952966368811;
                    }
                }
            }
        }
    }

    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            sum=0;
            for(int u=0;u<8;u++){
                for(int v=0;v<8;v++){
                    sum = sum + block[u][v] * coefficients[ i*512 + j*64 + u*8+v ];
                }
            }

            // Only 8 and 12-bit is supported by DCT per JPEG standard
            if (precision == 8)
                transformedBlock[i][j]=sum+128;
            else if (precision == 12)
                transformedBlock[i][j]=sum+2048;

        }
    }
}


void JpegDecode::addLossless(int *array, int currentComponent) {
    // Different meanings for SOF values
    unsigned char predictor = zigZagStart;
    unsigned char pointTransform = approximationL;

    static unsigned int currentX[ETF_FORMAT_MAX_COMPONENTS] = {0};
    static unsigned int currentY[ETF_FORMAT_MAX_COMPONENTS] = {0};
/*    static Pixel* pixels[ETF_FORMAT_MAX_COMPONENTS] = {0};
    if (pixels[0] == 0) {
        for (int i=0; i<components.size(); i++)
            pixels[i] = new Pixel(imagePtr->activeLayer()->getPixelAt(0,0));
    }
	
	// Predictors
	int* slc = scanLineCache[currentComponent];
	int Ra=array[0], Rb=slc[currentX[currentComponent]+1], Rc=slc[currentX[currentComponent]];
	if (currentX[currentComponent] == 0 && currentY[currentComponent] == 0)
		previousDC[currentComponent] = pow(2, precision - pointTransform - 1);
	else if (predictor == 0) 
		previousDC[currentComponent] = 0;
	else if (currentX[currentComponent] == imagePtr->width() - 1)
		previousDC[currentComponent] = slc[0];
	else if (predictor == 1 || currentY[currentComponent] == 0)
		previousDC[currentComponent] = Ra;
	else if (predictor == 2)
		previousDC[currentComponent] = Rb;
	else if (predictor == 3)
		previousDC[currentComponent] = Rc;
	else if (predictor == 4)
		previousDC[currentComponent] = Ra + Rb - Rc;
	else if (predictor == 5)
		previousDC[currentComponent] = Ra + ((Rb - Rc) >> 1);
	else if (predictor == 6)
		previousDC[currentComponent] = Rb + ((Ra - Rc) >> 1);
	else if (predictor == 7)
		previousDC[currentComponent] = (Ra + Rb) >> 1;
	
	slc[currentX[currentComponent]] = array[0];


    // we are using either 8-bit or 16-bit precision so we must increase value
    if (precision > 8)
        array[0] = array[0] << (16-precision);
    else
        array[0] = array[0] << (8-precision);
	
    pixels[currentComponent]->setComponent(currentComponent, array[0]);
    pixels[currentComponent]->next();

    currentX[currentComponent] ++;
    if (currentX[currentComponent] >= imagePtr->width()) {
        currentX[currentComponent] -= imagePtr->width();
        currentY[currentComponent]++;
    }
    if (currentY[currentComponent] >= imagePtr->height() && currentComponent == components.size()-1)
        endOfFile = true;*/
}








// ---------- VARIOUS DEBUGGING METHODS

string JpegDecode::binary (unsigned int v) {
    char binstr[17] ;
    int i ;

    binstr[16] = '\0' ;
    for (i=0; i<16; i++) {
        binstr[15-i] = v & 1 ? '1' : '0' ;
        v = v / 2 ;
    }

    return binstr ;
    string a=binstr;
    return a;
}


void JpegDecode::outputQuantizationTables(){
    for(uint x=0;x<this->components.size();x++){
        for(int i=0;i<8;i++){
            for(int j=0;j<8;j++)
                std::cout<<(int)this->components[x].componentQuantizationTable->quantizationTableData[i][j]<<" ";
            std::cout<<endl;
        }
        std::cout<<endl<<endl;
    }
}


void JpegDecode::outputHuffmanCodes(){
    qDebug() << "";

    for (uint i=0; i<huffmanTables.size(); i++) {
        HuffmanTable* table = huffmanTables[i];
        qDebug() << "====== Table no."<<i<<"ID"<<table->tableID<<"Class"<<table->tableClass;
        for (uint j=0; j<table->codes.size(); j++) {
            if (table->codes[j] == 0xFFFFFFFF)
                continue;
            qDebug() << "Code no"<<j<<"code"<<binary((int)table->codes[j]).c_str()<<"Length"<<table->codeLengths[j];
        }
    }

    qDebug() << "";

}


void JpegDecode::dumpMetaData() {
    qDebug() << "METADATA:";
    QMapIterator<QString, QString> i(metadata);
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << ": " << i.value();
    }
}


void JpegDecode::dumpBlock(int block[8][8]) {
    cout << "Dump block:" << endl;
    for (int i=0; i<8; i++) {
        for (int j=0; j<8; j++)
            cout << setw(5) << block[i][j] << " ";
        cout <<endl;
    }
}
