#include "huffmantable.h"

/*HuffmanTable::HuffmanTable():luminanceDChuffmanCode(256,0xFFFFFFFF),luminanceAChuffmanCode(256,0xFFFFFFFF),chrominanceDChuffmanCode(256,0xFFFFFFFF),chrominanceAChuffmanCode(256,0xFFFFFFFF),luminanceDCHuffmanCodeLength(256,1000),luminanceAChuffmanCodeLength(256,1000), chrominanceDChuffmanCodeLength(256,1000),chrominanceAChuffmanCodeLength(256,1000)
{
        //Initial value of 1000 for code length is logic. Why?
        //Because code length can NEVER reach value of 1000. If we put number<0 sometimes this if statement can be true, and we can have problems
        //  if (currentDataLength>=huffmanTable.luminanceDCHuffmanCodeLength[i] && huffmanTable.luminanceDChuffmanCode[i] == data >> (currentDataLength-huffmanTable.luminanceDCHuffmanCodeLength[i])) {
}*/

HuffmanTable::HuffmanTable(): codes(256,0xFFFFFFFF), codeLengths(256,1000)
{
        //Initial value of 1000 for code length is logic. Why?
        //Because code length can NEVER reach value of 1000. If we put number<0 sometimes this if statement can be true, and we can have problems
        //  if (currentDataLength>=huffmanTable.luminanceDCHuffmanCodeLength[i] && huffmanTable.luminanceDChuffmanCode[i] == data >> (currentDataLength-huffmanTable.luminanceDCHuffmanCodeLength[i])) {
}


/*void HuffmanTable::deleteUnnecessaryData() {
    for (int i=0; i<tables.size(); i++)
        delete tables[i];
    tables.clear();
}*/

void HuffmanTree::deleteNode(HuffmanNode* node) {
    if (node==0) return;
    deleteNode(node->left);
    deleteNode(node->right);
    delete node;
}

void HuffmanTree::addNode(HuffmanNode* &node, unsigned int codeValue, unsigned int codeLength, unsigned int dataValue) {
    if (node==0) {
        node = new HuffmanNode;
        node->codeValue = codeValue;
        node->codeLength = codeLength;
        node->dataValue = dataValue;
        node->left = node->right = 0;
        return;
    }

    if (codeLength < node->codeLength) {
        HuffmanNode *tmpnode = new HuffmanNode;
        tmpnode->codeValue = codeValue;
        tmpnode->codeLength = codeLength;
        tmpnode->dataValue = dataValue;

        int nodePartValue = node->codeValue >> (node->codeLength - codeLength);
        if (nodePartValue < codeValue) {
            tmpnode->left = node;
            tmpnode->right = 0;
            node = tmpnode;
        } else {
            tmpnode->right = node;
            tmpnode->left = 0;
            node = tmpnode;
        }

    } else {
        int partValue = codeValue >> (codeLength - node->codeLength);
        if (partValue < node->codeValue)
            addNode(node->left, codeValue, codeLength, dataValue);
        else
            addNode(node->right, codeValue, codeLength, dataValue);
    }
}

HuffmanNode* HuffmanTree::getNode(HuffmanNode* node, unsigned int currentData, unsigned int currentDataLength) {
    if (node==0) return 0;

    // If current data is shorter than node, it means we didn't find it
    // because the shortest code is at root and longer ones are below
    if (currentDataLength < node->codeLength) return 0;

    int partValue = currentData >> (currentDataLength - node->codeLength);

    if (partValue == node->codeValue) return node;
    if (partValue < node->codeValue) return getNode(node->left, currentData, currentDataLength);
    return getNode(node->right, currentData, currentDataLength);
}
