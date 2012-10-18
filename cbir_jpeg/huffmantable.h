#ifndef HUFFMANTABLE_H
#define HUFFMANTABLE_H

#include <vector>

using namespace std;

class HuffmanTable {

public:
/*    //Huffman table codes
    vector <unsigned int> luminanceDChuffmanCode;
    vector <unsigned int> luminanceAChuffmanCode;
    vector <unsigned int>chrominanceDChuffmanCode;
    vector <unsigned int>chrominanceAChuffmanCode;

    //Huffman table code lengths
    vector <unsigned int>  luminanceDCHuffmanCodeLength;
    vector <unsigned int> luminanceAChuffmanCodeLength;
    vector <unsigned int> chrominanceDChuffmanCodeLength;
    vector <unsigned int> chrominanceAChuffmanCodeLength;*/
    unsigned char tableID, tableClass;
    vector <unsigned int> codes, codeLengths;
    HuffmanTable();
//    static void deleteUnnecessaryData();
};


struct HuffmanNode {
    unsigned int codeValue, codeLength, dataValue;
    HuffmanNode* left;
    HuffmanNode* right;
};


class HuffmanTree {
private:
    HuffmanNode* root;
    void deleteNode(HuffmanNode* node);
    void addNode(HuffmanNode* &node, unsigned int codeValue, unsigned int codeLength, unsigned int dataValue);
    HuffmanNode* getNode(HuffmanNode* node, unsigned int codeValue, unsigned int codeLength);


public:
    unsigned char tableID, tableClass;

    // We use lookup table for shorter codes (under 8 bits)
    // To account for code length, an additional binary one is added to beginning
    HuffmanNode* huffmanShortLUT[65536];
    HuffmanNode* huffmanShortLUT2[65536];
    int codes[256];
    int codeLengths[256];

    HuffmanTree() : root(0) {
        for (int i=0; i<256; i++) codes[i] = codeLengths[i] = 0;
    }
    ~HuffmanTree() {
        deleteNode(root);
        for (int i=0; i<65536; i++)
            if (huffmanShortLUT[i] != 0) delete huffmanShortLUT[i];
    }

    void addNode(unsigned int codeValue, unsigned int codeLength, unsigned int dataValue) {
        addNode(root, codeValue, codeLength, dataValue);
    }

    HuffmanNode* getNode(unsigned int codeValue, unsigned int codeLength) {
        return getNode(root, codeValue, codeLength);
    }
};

#endif // HUFFMANTABLE_H
