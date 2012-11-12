#include "picture.h"

Picture::Picture(string imagePath, string imageName, int imageHeight, int imageWidth){
    this->imagePath=imagePath;
    this->imageName=imageName;
    this->imageHeight=imageHeight;
    this->imageWidth=imageWidth;
}

Picture::Picture(){
    this->imagePath="";
    this->imageName="";
    this->imageHeight=0;
    this->imageWidth=0;

}

void Picture::setImageName(string name){
    imageName=name;

}
