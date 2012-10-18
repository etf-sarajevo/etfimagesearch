#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QDir>
#include <QThread>

#include <ctime>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <cmath>

extern "C" {
    #include "cbir.h"
    #include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"
#include "cderror.h"
}

#define THREADS 4

using namespace std;

const char separator='/';

class MyThread : public QThread
 {
 public:
     void run();
     string name;
     vector<string> fileNames;
     vector<vector<vector<double> > > featureVectorsME, featureVectorsD;
 };

 void MyThread::run()
 {
     for (int i=0; i<fileNames.size(); i++) {
//         gettimeofday(&start, NULL);
         int R=0, G=0, B=0;

         struct jpeg_decompress_struct cinfo;
         struct jpeg_error_mgr jerr;
         FILE * input_file;
         JDIMENSION num_scanlines;

         /* Initialize the JPEG decompression object with default error handling. */
         cinfo.err = jpeg_std_error(&jerr);
         jpeg_create_decompress(&cinfo);

         if ((input_file = fopen(fileNames[i].c_str(), "rb")) == NULL) {
           exit(-1);
         }

         /* Specify data source for decompression */
         jpeg_stdio_src(&cinfo, input_file);

         /* Read file header, set default decompression parameters */
         (void) jpeg_read_header(&cinfo, TRUE);

         /* Start decompressor */
         (void) jpeg_start_decompress(&cinfo);
         (void) libjpeg_cbir_clear_stats();

         JSAMPARRAY bugger = new JSAMPROW[cinfo.output_height];
         for (int i=0; i<cinfo.output_height; i++)
             bugger[i] = new JSAMPLE[cinfo.output_width * 3];

         /* Process data */
         while (cinfo.output_scanline < cinfo.output_height) {
           num_scanlines = jpeg_read_scanlines(&cinfo, bugger,
                           cinfo.output_height);
           /* nista */
         }


         double cbir_featureVectorME[3][4];
         double cbir_featureVectorD[3][12];
         (void) libjpeg_cbir_final_stats(cbir_featureVectorME, cbir_featureVectorD);


         vector<vector<double> > fm;
         for (int l=0; l<3; l++) {
             vector<double> fml;
             for (int k=0; k<4; k++) {
                 fml.push_back(cbir_featureVectorME[l][k]);
 //                cout << cbir_featureVectorME[l][k] << " ";
             }
             fm.push_back(fml);
         }
         featureVectorsME.push_back(fm);
 //        cout << endl;

         vector<vector<double> > d;
         for (int l=0; l<3; l++) {
             vector<double> dl;
             for (int k=0; k<6; k++) {
                 dl.push_back(cbir_featureVectorD[l][k]);
 //                cout << cbir_featureVectorD[l][k] << " ";
             }
             for (int k=6; k<12; k++) {
                 dl.push_back(sqrt(cbir_featureVectorD[l][k]));
 //                cout << sqrt(cbir_featureVectorD[l][k]) << " ";
             }
             d.push_back(dl);
         }
         featureVectorsD.push_back(d);
 //        cout << endl;



         (void) jpeg_finish_decompress(&cinfo);
         jpeg_destroy_decompress(&cinfo);

         /* Close files, if we opened them */
         fclose(input_file);

         for (int i=0; i<cinfo.output_height; i++)
             delete bugger[i];
         delete [] bugger;


         /*R /= size;
         G /= size;
         B /= size;*/

//         gettimeofday(&end, NULL);

//         cout << ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) << " us" << endl;

//         cout << name<<" Indexing "<<i<<endl;
         //if (i==0) break;
     }
 }



 void threadindexing(const char* path, vector<string> &fileNames, vector<vector<vector<double> > > &featureVectorsME, vector<vector<vector<double> > > &featureVectorsD) {
     struct timeval start, end;

     QDir dir(path);
     QString entry;
     MyThread thread[THREADS];
     for (int i=0; i<THREADS; i++)
         thread[i].name = QString("t%1").arg(i+1).toLatin1().data();

         /*for (int i=0; i<1000; i++) {
             string s(path);
             s+=separator;
             s+=QString("%1").arg(i).toLatin1().data();
             s+=".jpg";
             if (i<250) thread1.fileNames.push_back(s);
             else if (i<500) thread2.fileNames.push_back(s);
             else if (i<750) thread3.fileNames.push_back(s);
             else thread4.fileNames.push_back(s);
         }*/
     int currThread=0;
     int total=0;
     foreach (entry, dir.entryList(QDir::Files)) {
         string s(path);
         s+=separator;
         s+=entry.toLatin1().data();
         thread[currThread++].fileNames.push_back(s);
         if (currThread == THREADS) currThread=0;
         total++;
     }

     for (int i=0; i<THREADS; i++)
         thread[i].start();

     for (int i=0; i<THREADS; i++)
         thread[i].wait();

     fileNames.reserve(total);
     featureVectorsME.reserve(total);
     featureVectorsD.reserve(total);
     for (int i=0; i<THREADS; i++) {
         fileNames.insert( fileNames.end(), thread[i].fileNames.begin(), thread[i].fileNames.end());
         featureVectorsME.insert( featureVectorsME.end(), thread[i].featureVectorsME.begin(), thread[i].featureVectorsME.end());
         featureVectorsD.insert( featureVectorsD.end(), thread[i].featureVectorsD.begin(), thread[i].featureVectorsD.end());
     }
 }



void fakeindexing(const char* path, vector<string> &fileNames, vector<vector<vector<double> > > &featureVectorsME, vector<vector<vector<double> > > &featureVectorsD) {
    struct timeval start, end;

    QDir dir(path);
    QString entry;
    foreach (entry, dir.entryList(QDir::Files)) {
        string s(path);
        s+=separator;
        s+=entry.toLatin1().data();
        fileNames.push_back(s);
    }
        /*for (int i=0; i<1000; i++) {
            string s(path);
            s+=separator;
            s+=QString("%1").arg(i).toLatin1().data();
            s+=".jpg";
            fileNames.push_back(s);
        }*/

    for (int i=0; i<fileNames.size(); i++) {
        gettimeofday(&start, NULL);
        int R=0, G=0, B=0;
        QImage image(fileNames[i].c_str());
        uchar* data = image.bits();
        int size = image.width() * image.height();
        for (int i=0; i<size; i++) {
            R += *(data++);
            G += *(data++);
            B += *(data++);
//            cout << "R="<<R<<" G="<<G<<" B="<<B<<endl;
        }

        R /= size;
        G /= size;
        B /= size;

        gettimeofday(&end, NULL);

        cout << ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) << " us" << endl;

        cout << "Indexing "<<i<<endl;
//        if (i==0) break;
    }
}

void newindexing(const char* path, vector<string> &fileNames, vector<vector<vector<double> > > &featureVectorsME, vector<vector<vector<double> > > &featureVectorsD) {
    struct timeval start, end;

    QDir dir(path);
    QString entry;
/*    foreach (entry, dir.entryList(QDir::Files)) {
        string s(path);
        s+=separator;
        s+=entry.toLatin1().data();
        fileNames.push_back(s);
    }*/
        for (int i=0; i<1000; i++) {
            string s(path);
            s+=separator;
            s+=QString("%1").arg(i).toLatin1().data();
            s+=".jpg";
            fileNames.push_back(s);
        }

    for (int i=0; i<fileNames.size(); i++) {
        gettimeofday(&start, NULL);
        int R=0, G=0, B=0;
/*        QImage image(fileNames[i].c_str());
        uchar* data = image.bits();
        int size = image.width() * image.height();
        for (int i=0; i<size; i++) {
            R += *(data++);
            G += *(data++);
            B += *(data++);
            //cout << "R="<<R<<" G="<<G<<" B="<<B<<endl;
        }*/

        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE * input_file;
        JDIMENSION num_scanlines;

        /* Initialize the JPEG decompression object with default error handling. */
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        if ((input_file = fopen(fileNames[i].c_str(), "rb")) == NULL) {
          exit(-1);
        }

        /* Specify data source for decompression */
        jpeg_stdio_src(&cinfo, input_file);

        /* Read file header, set default decompression parameters */
        (void) jpeg_read_header(&cinfo, TRUE);

        /* Start decompressor */
        (void) jpeg_start_decompress(&cinfo);
        (void) libjpeg_cbir_clear_stats();

        JSAMPARRAY bugger = new JSAMPROW[cinfo.output_height];
        for (int i=0; i<cinfo.output_height; i++)
            bugger[i] = new JSAMPLE[cinfo.output_width * 3];

        /* Process data */
        while (cinfo.output_scanline < cinfo.output_height) {
          num_scanlines = jpeg_read_scanlines(&cinfo, bugger,
                          cinfo.output_height);
          /* nista */
        }


        double cbir_featureVectorME[3][4];
        double cbir_featureVectorD[3][12];
        (void) libjpeg_cbir_final_stats(cbir_featureVectorME, cbir_featureVectorD);


        vector<vector<double> > fm;
        for (int l=0; l<3; l++) {
            vector<double> fml;
            for (int k=0; k<4; k++) {
                fml.push_back(cbir_featureVectorME[l][k]);
//                cout << cbir_featureVectorME[l][k] << " ";
            }
            fm.push_back(fml);
        }
        featureVectorsME.push_back(fm);
//        cout << endl;

        vector<vector<double> > d;
        for (int l=0; l<3; l++) {
            vector<double> dl;
            for (int k=0; k<6; k++) {
                dl.push_back(cbir_featureVectorD[l][k]);
//                cout << cbir_featureVectorD[l][k] << " ";
            }
            for (int k=6; k<12; k++) {
                dl.push_back(sqrt(cbir_featureVectorD[l][k]));
//                cout << sqrt(cbir_featureVectorD[l][k]) << " ";
            }
            d.push_back(dl);
        }
        featureVectorsD.push_back(d);
//        cout << endl;



        (void) jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        /* Close files, if we opened them */
        fclose(input_file);

        for (int i=0; i<cinfo.output_height; i++)
            delete bugger[i];
        delete [] bugger;


        /*R /= size;
        G /= size;
        B /= size;*/

        gettimeofday(&end, NULL);

        cout << ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) << " us" << endl;

        cout << "Indexing "<<i<<endl;
        //if (i==0) break;
    }
}

void saveIndex(vector<vector<vector<double> > > &featureVectorsME, vector<vector<vector<double> > > &featureVectorsD) {
    /*ofstream file;
    file.open("/home/vedran/cbir/image.vary.index", ios::out | ios::binary);
    for (int i=0; i<9907; i++) {
        for (int l=0; l<3; l++) {
            for (int k=0; k<4; k++) {
                file << featureVectorsME[i][l][k]<<"\n";
            }
        }
        for (int l=0; l<3; l++) {
            for (int k=0; k<12; k++) {
                file << featureVectorsD[i][l][k]<<"\n";
            }
        }
    }
    file.close();*/
}

void loadIndex(vector<vector<vector<double> > > &featureVectorsME, vector<vector<vector<double> > > &featureVectorsD) {
/*    ifstream file;
    file.open("/home/vedran/cbir/image.index", ios::in | ios::binary);
    char c;
    int i=0;
    while (file.good()) {
        vector<vector<double> > fm;
        for (int l=0; l<3; l++) {
            vector<double> fml;
            for (int k=0; k<4; k++) {
                double d;
                file >> d  ;
                fml.push_back(d);
            }
            fm.push_back(fml);
        }
        featureVectorsME.push_back(fm);

        vector<vector<double> > fd;
        for (int l=0; l<3; l++) {
            vector<double> fdl;
            for (int k=0; k<12; k++) {
                double d;
                file >> d  ;
                fdl.push_back(d);
            }
            fd.push_back(fdl);
        }
        featureVectorsD.push_back(fd);

        cout << "Reading "<<i<<endl;
        i++;
    }
    file.close();*/
}

int main(int argc, char** argv) {
    QApplication a(argc, argv);
    QGraphicsScene scene;
    QGraphicsView view(&scene);
    vector<vector <double> > topM, topD, topW;
    vector<vector <int> > topMi, topDi, topWi;

    vector<int> chosens;
    vector<vector<vector<double> > > featureVectorsME;
    vector<vector<vector<double> > > featureVectorsD;
    vector<string> fileNames;

    int chosensPerCategory = 15;

    struct timeval start, end;
    gettimeofday(&start, NULL);
//    clock_t t1 = clock();

    //threadindexing("/home/vedran/cbir/alma", fileNames, featureVectorsME, featureVectorsD);
    threadindexing("/home/vedran/cbir/image.orig", fileNames, featureVectorsME, featureVectorsD);

    gettimeofday(&end, NULL);
    cout << ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec) << " us" << endl;

//    cout << "Indexing total: " << (clock() - t1) / (CLOCKS_PER_SEC / 1000) << " ms"<< endl;
//    saveIndex(featureVectorsME, featureVectorsD);
//    loadIndex(featureVectorsME, featureVectorsD);
    return 0;

/*    for (int i=0; i<10; i++) {
        for (int j=0; j<chosensPerCategory; j++) {
            int chosen;
            bool found=false;
            do {
                chosen = i * 100 + rand()%100;
                found=false;
                for (int k=0; k<5; k++) {
                    if (chosens.size()>k && chosens[chosens.size()-k-1] == chosen) found=true;
                }
            } while(found);

            chosens.push_back(chosen);

            cout << "Chosen: "<<chosen<<endl;

            vector<double> blank;
            topM.push_back(blank); topD.push_back(blank); topW.push_back(blank);
            vector<int> blank2;
            topMi.push_back(blank2); topDi.push_back(blank2); topWi.push_back(blank2);
        }
    }*/


    for (int i=0; i<fileNames.size(); i++) {
        chosens.push_back(i);

        vector<double> blank;
        topM.push_back(blank); topD.push_back(blank); topW.push_back(blank);
        vector<int> blank2;
        topMi.push_back(blank2); topDi.push_back(blank2); topWi.push_back(blank2);

    }

    int P16M=0, P32M=0, P64M=0;
    int P16D=0, P32D=0, P64D=0;
    int P16W=0, P32W=0, P64W=0;


    for (int l=0; l<chosens.size(); l++) {
        for (int i=featureVectorsME.size()-1; i>=0; i--) {
            if (i==chosens[l]) continue;

            double ME=0;
            for (int k=0; k<3; k++) {
                double sum=0;
                for (int j=0; j<4; j++)
                    sum += pow(featureVectorsME[chosens[l]][k][j] - featureVectorsME[i][k][j], 2);
                ME += sqrt(sum);
            }
            //cout << "ME = "<<ME<<endl;
            double D=0;
            for (int k=0; k<3; k++) {
                double sum=0;
                for (int j=0; j<12; j++) {
                    sum += pow(featureVectorsD[chosens[l]][k][j] - featureVectorsD[i][k][j], 2);
                    //cout << "j=" << j << " : " << featureVectorsD[chosens[l]][k][j] << " - " << featureVectorsD[i][k][j] << endl;
                }
                D += sqrt(sum);
            }
            //cout << "D = "<<D<<endl;
//            double W = 0.99999995*ME + 0.00000005*D;
            //double W = pow(D,3)*ME;
            double W = 0.9995*D + 0.005*ME;
            //return 0;

            bool foundM=false, foundD=false, foundW=false;
            for (int j=0; j<topM[l].size(); j++) {
                if (!foundM && ME < topM[l][j]) {
                    topM[l].insert(topM[l].begin()+j, ME);
                    topMi[l].insert(topMi[l].begin()+j, i);
                    foundM=true;
                }
                if (!foundD && D < topD[l][j]) {
                    topD[l].insert(topD[l].begin()+j, D);
                    topDi[l].insert(topDi[l].begin()+j, i);
                    foundD=true;
                }
                if (!foundW && W < topW[l][j]) {
                    topW[l].insert(topW[l].begin()+j, W);
                    topWi[l].insert(topWi[l].begin()+j, i);
                    foundW=true;
                }
            }
            if (!foundM) { topM[l].push_back(ME); topMi[l].push_back(i); }
            if (!foundD) { topD[l].push_back(D); topDi[l].push_back(i); }
            if (!foundW) { topW[l].push_back(W); topWi[l].push_back(i); }
            //cout << "Processing: "<<i<<endl;
        }

        cout << "Searching "<<chosens[l]<<endl;

        int P16=0, P32=0, P64=0;
        for (int i=0; i<64; i++) {
            //cout << "i="<<i<<" t="<<topMi[l][i]<<" ";
            if (topMi[l][i]/100 == chosens[l]/100) {
                if (i<16) { P16++; }
                if (i<32) { P32++; }
                if (i<64) { P64++; }
            }
        }
        /*if (l==0) {
            for (int i=0; i<64; i++)
                cout << "topM " << topMi[l][i] << ",";
            cout << endl;
        }*/
        cout << endl;
        cout << "M: P(16) "<<(double(P16)/16)<<" R(16) "<<(double(P16)/100)<<" P(32) "<<(double(P32)/32)<<" R(32) "<<(double(P32)/100)<<" P(64) "<<(double(P64)/64)<<" R(64) "<<(double(P64)/100)<<endl;
        P16M += P16;
        P32M += P32;
        P64M += P64;

        P16=0; P32=0; P64=0;
        for (int i=0; i<64; i++) {
            if (topDi[l][i]/100 == chosens[l]/100) {
                if (i<16) { P16++; }
                if (i<32) { P32++; }
                if (i<64) { P64++; }
            }
        }
        cout << "D: P(16) "<<(double(P16)/16)<<" R(16) "<<(double(P16)/100)<<" P(32) "<<(double(P32)/32)<<" R(32) "<<(double(P32)/100)<<" P(64) "<<(double(P64)/64)<<" R(64) "<<(double(P64)/100)<<endl;
        P16D += P16;
        P32D += P32;
        P64D += P64;

        P16=0; P32=0; P64=0;
        for (int i=0; i<64; i++) {
            if (topWi[l][i]/100 == chosens[l]/100) {
                if (i<16) { P16++; }
                if (i<32) { P32++; }
                if (i<64) { P64++; }
            }
        }
        cout << "W: P(16) "<<(double(P16)/16)<<" R(16) "<<(double(P16)/100)<<" P(32) "<<(double(P32)/32)<<" R(32) "<<(double(P32)/100)<<" P(64) "<<(double(P64)/64)<<" R(64) "<<(double(P64)/100)<<endl;
        P16W += P16;
        P32W += P32;
        P64W += P64;

        cout << "M: P(16) "<<(double(P16M)/(l+1)/16)<<" R(16) "<<(double(P16M)/(l+1)/100)<<" P(32) "<<(double(P32M)/(l+1)/32)<<" R(32) "<<(double(P32M)/(l+1)/100)<<" P(64) "<<(double(P64M)/(l+1)/64)<<" R(64) "<<(double(P64M)/(l+1)/100)<<endl;
        cout << "D: P(16) "<<(double(P16D)/(l+1)/16)<<" R(16) "<<(double(P16D)/(l+1)/100)<<" P(32) "<<(double(P32D)/(l+1)/32)<<" R(32) "<<(double(P32D)/(l+1)/100)<<" P(64) "<<(double(P64D)/(l+1)/64)<<" R(64) "<<(double(P64D)/(l+1)/100)<<endl;
        cout << "W: P(16) "<<(double(P16W)/(l+1)/16)<<" R(16) "<<(double(P16W)/(l+1)/100)<<" P(32) "<<(double(P32W)/(l+1)/32)<<" R(32) "<<(double(P32W)/(l+1)/100)<<" P(64) "<<(double(P64W)/(l+1)/64)<<" R(64) "<<(double(P64W)/(l+1)/100)<<endl;
        //if (l==5) return 0;
    }


    return 0;

    /*


    cout<<"Top M: ";
    for (int i=0; i<topM.size(); i++)
        cout<<topMi[i]<<",";
    cout<<endl;
    cout<<"Top D: ";
    for (int i=0; i<topD.size(); i++)
        cout<<topDi[i]<<",";
    cout<<endl;
    cout<<"Top W: ";
    for (int i=0; i<topW.size(); i++)
        cout<<topWi[i]<<",";
    cout<<endl;*/


//    QGraphicsPixmapItem item(QPixmap::fromImage(picture.imagePtr->mergedQImage()));
  //  scene.addItem(&item);
//    view.show();
//    return a.exec();
}

/*int main(void)
{
    struct timeval start, end;
    int i,n;
    gettimeofday(&start, NULL);

    FILE* f = fopen("/home/vedran/cbir/alma/DSCN0706.JPG", "r");
    char buff[65536];
    while ( (n=fread(buff, 1, 65536, f)) != 0)

    fclose(f);


    /*QFile file("/home/vedran/cbir/alma/DSCN0706.JPG");
    file.open(QFile::ReadOnly);
    char c;
    while (file.read((char*)&c, sizeof c) != 0);
    file.close();*//*

    gettimeofday(&end, NULL);
    printf ("%ld us\n", (end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);

    printf("Hello World!\n");
    return 0;
}*/


