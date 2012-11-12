
#ifndef XMLPARSER_H
#define XMLPARSER_H

using namespace std;

#include <iostream>
#include <string>
#include <QFile>
#include <QFileDialog>
#include "Picture.h";



class XMLParser
{
    QFile *XMLFile;
    Picture pictureData;//We need pointer because there is no constructor with zero operands
public:

    XMLParser(QString filePath);//Constructor which opens the file
    void parseFile();
    void saveDataInPictureClass();



};

#endif // XMLPARSER_H
