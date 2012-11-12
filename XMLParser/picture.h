#ifndef PICTURE_H
#define PICTURE_H
using namespace std;
#include <iostream>
#include <string>

class Picture//In this class picture information are hold
{
//Initial Picture attributes. Will be changed during project implementation
    string imagePath;
    string imageName;
    int imageHeight;
    int imageWidth;
    string comment;

public:
    void setImageName(string name);
    Picture(string imagePath, string imageName, int imageHeight, int imageWidth);
    Picture();

};

#endif // PICTURE_H
