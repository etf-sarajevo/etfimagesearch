#ifndef JPEGIMAGE_H
#define JPEGIMAGE_H


#include "jpegdecode.h"
#include "jpegencode.h"

class JPEGIMAGESHARED_EXPORT JPEGImage: public ExportImportInterface {
    Q_INTERFACES(ExportImportInterface)
    Q_OBJECT


private:
    QTime myTimer;
    QString pluginName;
    QString imageFilter;//Image extension used by this program
    QStringList extensionList;//All possible extensions of JPEG file
public:
    JPEGImage();
    ~JPEGImage();


    void init(QWidget* );
    void deinit();

    //import
    QString filter() const;
    ImageStatePtr importImage(const QString& filename);

    //export
    QStringList extensions() const;
    QString name() const;
    bool exportImage(const ImageStatePtr imageState, const QString& filename);
    bool exportImage(const QImage& image, const QString& filename);

};

#endif // JPEGIMAGE_H
