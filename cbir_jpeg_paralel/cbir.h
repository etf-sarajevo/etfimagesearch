#ifndef CBIR_H
#define CBIR_H

void cbir_decode(const char* filename);

int cbir_parse_successful;
char* cbir_last_error_message;

struct {
    int width, height;
    int components;
} cbir_image_data;

double cbir_featureVectorME[3][4];
double cbir_featureVectorD[3][12];

#endif
