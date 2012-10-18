#include "cbir.h"

#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <byteswap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEBUGLEVEL 0

#define MAX_COMPONENTS 3
#define BUFFER_SIZE 65536

struct timeval start, end;
long idctTime, huffmanTime;

const char* error_messages[7] = {
    "This is not a JPEG file",
    "0xFF not found where expected",
    "Hierarchical JPEG not supported yet",
    "Precision other then 8 bits not supported yet",
    "Only Y'CbCr supported for now",
    "Can't find Huffman tables specified in the SOF header",
    "Number of components in SOF and SOS header doesn't match"

};


/* Huffman table definition */

struct Huffman_Table {
    unsigned char table_id, table_class;

    int codes[256];
    int code_lengths[256];
    unsigned char lookup_table[65536];
};

struct Huffman_Table* huffman_tables[16];
int huffman_tables_size;
int previous_dc[MAX_COMPONENTS];


/* Quantization table definition */

struct QuantizationTable {
    unsigned char table_id;
    int data[8][8];
};


/* Component definition */

struct Component {
    unsigned char id;
    int quantization_table, huff_table_dc, huff_table_ac, hfactor, vfactor;
};

struct Component components[MAX_COMPONENTS];


/* CBIR statistics */
double colorFeatures[MAX_COMPONENTS][4];
int colorFeaturesLast1000[MAX_COMPONENTS][4];
int colorFeaturesCounters[MAX_COMPONENTS];
int colorFeaturesDoubleCounters[MAX_COMPONENTS];

double specificBlocks[MAX_COMPONENTS][6];
int specificBlocksLast1000[MAX_COMPONENTS][6];
double specificBlocksSquares[MAX_COMPONENTS][6];
int specificBlocksSquaresLast1000[MAX_COMPONENTS][6];


/* Globals for stuff in buffer */
int end_of_file_marker, restart_marker, end_of_file;


/* Functions */


/* Huffman helper functions */

int read_file_if_neccessary(FILE* file, unsigned char* buff, int* bufpos, int* bufbitpos, int* buflen) {
    int i, j;


    /* Normalize bits */
    while (*bufbitpos >= 8) {
        (*bufpos)++;
        (*bufbitpos) -= 8;
    }
#if DEBUGLEVEL>1
    printf ("bufpos %d bufbitpos %d\n", *bufpos, *bufbitpos);
#endif

    if (*bufpos == end_of_file_marker - 1) {
#if DEBUGLEVEL>1
        printf ("End of image marker!\n");
#endif
        /* Allow AC block reading to read any remaining bits */
        end_of_file = 1;
        return 1;
    }

    if (*bufpos == restart_marker) {
#if DEBUGLEVEL>1
        printf ("Restart marker!\n");
#endif
        for (i=0; i<cbir_image_data.components; i++)
            previous_dc[i] = 0;
        (*bufpos) += 2;
        (*bufbitpos) = 0;
    }

    /* Is there a room for another int? */
    int x = *bufpos;
    int y = *buflen;
    y -= sizeof(short); /* y can be negative, while *buflen - sizeof(short) will cast to unsigned long */
    if (x >= y) {
        /* Move remaining bytes from end of buffer to beginning */
        int copy_bytes = *buflen - *bufpos;
        for (i=0; i<copy_bytes; i++)
            buff[i] = *(bufpos+i);
        *bufpos = 0;
#if DEBUGLEVEL>1
        if (copy_bytes > 0)
            printf ("Copied %d bytes to buffer start\n", copy_bytes);
#endif

        *buflen = copy_bytes + fread(buff+copy_bytes, sizeof(char), BUFFER_SIZE - copy_bytes - 1, file);
#if DEBUGLEVEL>1
        printf ("Read %d bytes from file\n", (*buflen) - copy_bytes);
#endif
        if ((*buflen) - copy_bytes == 0) return 0;

        /* Detect various special codes in buffer */
        end_of_file_marker = restart_marker = -1;
        for (i=0; i<*buflen-1; i++) {
            if (buff[i] == 0xff && buff[i+1] == 0x00) {
#if DEBUGLEVEL>1
                printf ("Removing stuffing at index %d\n", i);
#endif
                for (j=i+1; j<*buflen-1; j++)
                    buff[j] = buff[j+1];
                (*buflen)--;
            }
            else if (buff[i] == 0xff && buff[i+1] == 0xd9) {
                end_of_file_marker = i;
#if DEBUGLEVEL>1
                printf ("End of file at index %d\n", i);
#endif
            }
            else if (buff[i] == 0xff && (buff[i+1] >= 0xd0 && buff[i+1] <= 0xd7)) {
                restart_marker = i;
#if DEBUGLEVEL>1
                printf ("Restart marker at index %d\n", i);
#endif
            }
        }

    }


    return 1;
}


void cbir_final_stats() {
    int i,j;

    for (j=0; j<cbir_image_data.components; j++) {
//        cout << "Component: "<<j<<endl;
        for (i=0; i<4; i++) {
            double last1000Average = ((double)colorFeaturesLast1000[j][i]) / 1024;
            colorFeatures[j][i] *= colorFeaturesDoubleCounters[j];
            colorFeatures[j][i] += last1000Average;
//            printf ("AVERAGE %lf %lf\n", last1000Average, colorFeatures[currentComponent][i]);
            double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
            colorFeatures[j][i] /= divisor;
//            cout << "  M=" << colorFeatures[j][i];
            cbir_featureVectorME[j][i] = colorFeatures[j][i];
//            printf ("cme %d %d = %lf\n", j, i, cbir_featureVectorME[j][i]);
        }
//        cout << endl;
        for (i=0; i<6; i++) {
            double last1000Average = ((double)specificBlocksLast1000[j][i]) / 1024;
            specificBlocks[j][i] *= colorFeaturesDoubleCounters[j];
            specificBlocks[j][i] += last1000Average;
            double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
            specificBlocks[j][i] /= divisor;
//            cout << "  D=" << specificBlocks[j][i];
            cbir_featureVectorD[j][i] = specificBlocks[j][i];
        }
        for (i=0; i<6; i++) {
            double last1000Average = ((double)specificBlocksSquaresLast1000[j][i]) / 1024;
            specificBlocksSquares[j][i] *= colorFeaturesDoubleCounters[j];
            specificBlocksSquares[j][i] += last1000Average;
            double divisor = ((double)colorFeaturesCounters[j]) / 1024 + colorFeaturesDoubleCounters[j];
            specificBlocksSquares[j][i] /= divisor;

            // Std dev = square root of (mean of squares minus square of mean)
            cbir_featureVectorD[j][i+6] = sqrt(fabs(specificBlocksSquares[j][i] - pow(specificBlocks[j][i],2)));
//            cout << "  D=" << featureVectorD[j][i+6];
        }
    }

}


void cbir_process_data_block(int* row, int currentComponent) {
    int i;

    int M11 = (row[0] + row[1] + row[2] + row[4]) >> 4;
    int M12 = (row[0] + row[1] - row[2] - row[4]) >> 4;
    int M21 = (row[0] - row[1] + row[2] - row[4]) >> 4;
    int M22 = (row[0] - row[1] - row[2] + row[4]) >> 4;

    if (currentComponent == 0 && colorFeaturesLast1000[currentComponent][0]==0) {
        for (i=0; i<64; i++)
            fprintf(stderr, "%d ",row[i]);
        fprintf(stderr, "\n");
    }

    colorFeaturesLast1000[currentComponent][0] += M11;
    colorFeaturesLast1000[currentComponent][1] += M12;
    colorFeaturesLast1000[currentComponent][2] += M21;
    colorFeaturesLast1000[currentComponent][3] += M22;

    colorFeaturesCounters[currentComponent]++;

#if DEBUGLEVEL>1
    printf ("row[0]=%d row[1]=%d row[2]=%d row[3]=%d\n", row[0], row[1], row[2], row[3]);
    printf ("M11=%d M12=%d M21=%d M22=%d\n", M11, M12, M21, M22);
    printf ("DEBUG (%d,%d): %d %d %d %d\n", currentComponent, colorFeaturesCounters[currentComponent], colorFeaturesLast1000[currentComponent][0], colorFeaturesLast1000[currentComponent][1], colorFeaturesLast1000[currentComponent][2], colorFeaturesLast1000[currentComponent][3]);
#endif

    // Coefficients displayed on Figure 3 from paper
    int texture1 = row[0];
    int texture2 = row[1] + row[2] + row[3] + row[4] + row[5];
    int texture3 = 0;
    for (i=6; i<15; i++) texture3 += row[i];

    int texture4 = /*row[0] +*/ row[2] + row[3] + row[9] + row[10] + row[20] + row[21] + row[35]; /* Error in paper */
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
        for (i=0; i<4; i++) {
            double last1000Average = ((double)colorFeaturesLast1000[currentComponent][i]) / 1024;
            colorFeatures[currentComponent][i] *= colorFeaturesDoubleCounters[currentComponent];
            colorFeatures[currentComponent][i] += last1000Average;
            colorFeatures[currentComponent][i] /= (colorFeaturesDoubleCounters[currentComponent] + 1);
            //colorFeaturesLast1000[currentComponent][i] = 0;
#if DEBUGLEVEL>1
            printf ("AVERAGE %lf %lf\n", last1000Average, colorFeatures[currentComponent][i]);
#endif
        }
        colorFeaturesDoubleCounters[currentComponent]++;
        colorFeaturesCounters[currentComponent] = 0;

        for (i=0; i<6; i++) {
            double last1000Average = ((double)specificBlocksLast1000[currentComponent][i]) / 1024;
            specificBlocks[currentComponent][i] *= colorFeaturesDoubleCounters[currentComponent];
            specificBlocks[currentComponent][i] += last1000Average;
            specificBlocks[currentComponent][i] /= (colorFeaturesDoubleCounters[currentComponent] + 1);
            //specificBlocksLast1000[currentComponent][i] = 0;
        }
        for (i=0; i<6; i++) {
            double last1000Average = ((double)specificBlocksSquaresLast1000[currentComponent][i]) / 1024;
            specificBlocksSquares[currentComponent][i] *= colorFeaturesDoubleCounters[currentComponent];
            specificBlocksSquares[currentComponent][i] += last1000Average;
            specificBlocksSquares[currentComponent][i] /= (colorFeaturesDoubleCounters[currentComponent] + 1);
            //specificBlocksSquaresLast1000[currentComponent][i] = 0;
        }
    }

}


/* Returns top 16 bits in buffer */
unsigned short get_current_value(unsigned char* buff, int bufpos, int bufbitpos) {
    unsigned int current_value = *((unsigned int*)(buff+bufpos));
#if DEBUGLEVEL>1
    printf ("buff %x ", current_value);
#endif
    current_value = __bswap_32 ( current_value );
#if DEBUGLEVEL>1
    printf ("swap %x ", current_value);
#endif
    current_value =  current_value << bufbitpos;
    current_value =  current_value >> 16;
#if DEBUGLEVEL>1
    printf ("shift %x s2 %x", current_value, current_value>>bufbitpos);
#endif
#if DEBUGLEVEL>1
    printf ("\n");
#endif
    return current_value;
}


/* Main Huffman decoding function */
void cbir_read_image_data(FILE* file) {
    int i, k;

    /* Variables used in Huffman decoding */
    for (i=0; i<cbir_image_data.components; i++)
        previous_dc[i] = 0;

    unsigned char buff[BUFFER_SIZE];
    int bufpos, bufbitpos, buflen;
    bufpos = bufbitpos = buflen = 0;

    int data_block[64];
    int data_block_pos = 0;
    memset ((char*)data_block, 0, sizeof(int)*64);

    enum { AC, DC } ACDC = DC;
    int ACcount = 64; /*  int ACcount = zigZagEnd - zigZagStart; */

    unsigned int masks[16];
    masks[0] = pow(2,15);
    for (i=1; i<16; i++)
        masks[i] = masks[i-1] + pow(2, 15-i);

    unsigned short current_value = 0;
    int element = -1, bits=0;
    unsigned short current_component = 0;
    int counter2 = 1; /* Counter for chroma subsampling */
    int blkNo=0;

    end_of_file = 0; /* Global */


    struct Huffman_Table* htable = huffman_tables[components[0].huff_table_dc];

#if DEBUGLEVEL>0
    printf ("--- Huffman read block 0 component 0\n");
#endif


    /* Main loop */

    do {
        if (read_file_if_neccessary(file, buff, &bufpos, &bufbitpos, &buflen) == 0)
            break; /* End of file */

        /* Look for code in huffman table (must convert to little endian) */
        current_value = get_current_value(buff, bufpos, bufbitpos);

        element = -1;
        for (bits=1; bits<=16; bits++) {
            int idx = current_value & masks[bits-1];
#if DEBUGLEVEL>1
            printf ("Looking for code %x in LUT (%x, %x, %d)\n", idx, current_value, masks[bits-1], bits);
#endif
            if (blkNo == 312142)
            printf ("Looking for code %x in LUT (%x, %x, %d)\n", idx, current_value, masks[bits-1], bits);
            if ((element = htable->lookup_table[idx]) != -1) {
                if (element<0 || element>255) printf("element: %d\n", element);
                if (htable->code_lengths[element] == bits)
                    break;
                element = -1;
            }
        }

        /* Code not found! wtf */
        if (element == -1) {
#if DEBUGLEVEL>0
            printf ("Code not found!\n");
#endif
            bufbitpos++;
        }

        else {
#if DEBUGLEVEL>1
            printf ("Found data %x len %d at index %d\n", htable->codes[element], bits, element);
#endif
            if (blkNo == 312142)
            printf ("Found data %x len %d at index %d\n", htable->codes[element], bits, element);
            bufbitpos += bits;

            if (ACDC == DC) {
                /* Reading of DC coefficients */

                if (read_file_if_neccessary(file, buff, &bufpos, &bufbitpos, &buflen) == 0)
                    break; /* End of file */

                /* Further 'element' bits is DC coefficient value */
                int DC = get_current_value(buff, bufpos, bufbitpos);
                DC = DC >> (16 - element);
                bufbitpos += element;

                /* If MSB in DC coefficient starts with 0, then substract value of DC with 2^bitlength+1 */
                if ( element != 0 && ( DC >> (element-1) ) == 0 ) {
                    DC = DC - (2 << (element-1)) + 1;
                }

                /* Differential coding of DC coefficients */
                data_block[data_block_pos] = DC + previous_dc[current_component];
#if DEBUGLEVEL>1
                printf ("DC is %d\n", DC + previous_dc[current_component]);
#endif
                previous_dc[current_component] += DC;
                data_block_pos++;

                /* Now we read AC */
                ACDC = AC;
                htable = huffman_tables[components[current_component].huff_table_ac];


            } else {
                /* Reading of AC coefficients */


                /* Every AC component is composite of 4 bits (RRRRSSSS). R bits tells us relative position of
                   non zero element from the previous non zero element (number of zeros between two non zero elements)
                   SSSS bits tels us magnitude range of AC element
                   Two special values:
                      00 is END OF BLOCK (all AC elements are zeros)
                      F0 is 16 zeroes */

                if (element == 0x00) {
                    for (; data_block_pos<ACcount; data_block_pos++)
                        data_block[data_block_pos] = 0;
                }

                else if (element == 0xF0) {
                    for (k=0;k<16;k++) {
                        data_block[data_block_pos] = 0;
                        data_block_pos++;
                        if (data_block_pos >= ACcount) {
                            break;
                        }
                    }
                }
                else {

                    /* If AC element is 0xAB for example, then we have to separate it in two nibbles
                       First nible is RRRR bits, second are SSSS bits
                       RRRR bits told us how many zero elements are before this element
                       SSSS bits told us how many binary digits our AC element has (if 1001 then we have to read next 9 elements from file) */

                    unsigned char Rbits = element >> 4;
                    unsigned char Sbits = element & 0x0F;

                    /* Before our element there is Rbits zero elements */
                    for (k=0; k<Rbits; k++) {
                        if (data_block_pos >= ACcount) {
                            break;
                        }
                        data_block[data_block_pos] = 0;
                        data_block_pos++;
                    }

                    /* In case of error, reading file will just do more errors so skip this */
                    if (data_block_pos < ACcount) {
                        if (read_file_if_neccessary(file, buff, &bufpos, &bufbitpos, &buflen) == 0)
                            break; /* End of file */

                        /* Read out AC coefficient */
                        int AC = get_current_value(buff, bufpos, bufbitpos);
                        AC = AC >> (16 - Sbits);
                        bufbitpos += Sbits;

                        // If MSB in AC coefficient starts with 0, then substract value of AC with 2^bitLength+1
                        if ( Sbits != 0 && (AC>>(Sbits-1)) == 0 ) {
                            AC = AC - (2 << (Sbits-1)) + 1;
                        }
                        data_block[data_block_pos] = AC;
                        data_block_pos++;
                    }
                }

                // End of block
                if (data_block_pos >= ACcount) {
                    cbir_process_data_block(data_block, current_component);
                    ACDC = DC;
                    data_block_pos = 0;

                    /* Which is the next component? Depends on chroma subsampling */
                    if (counter2 < components[current_component].hfactor * components[current_component].vfactor)
                        counter2++;
                    else {
                        counter2 = 1;
                        current_component++;
                        if (current_component == cbir_image_data.components) current_component=0;
                    }
                    htable = huffman_tables[components[current_component].huff_table_dc];
#if DEBUGLEVEL>0
                    printf ("--- Huffman read block %d component %d\n", ++blkNo, current_component);
#endif
                }

            }
        }

    } while (!end_of_file);

    if (data_block_pos > 0) {
        for (; data_block_pos<ACcount; data_block_pos++)
            data_block[data_block_pos] = 0;
        cbir_process_data_block(data_block, current_component);
    }
    cbir_parse_successful = 1;

}


void cbir_read_scan_header(FILE* file) {
    /* Header length */
    int i, j, header_length = fgetc(file)*256 + fgetc(file);

    /* Taking information for number of components */
    if ((int)fgetc(file) != cbir_image_data.components) {
        cbir_last_error_message = (char*)error_messages[6];
        for (i=0; i<header_length-8; i++)
            fgetc(file);
        return;
    }

    if (huffman_tables_size == 0) {
        /* No Huffman tables read, can't proceed */
        for (i=0; i<header_length-3; i++)
            fgetc(file);
        return;
    }

    /* Read huffman tables for components (we ignore other data) */
    for (i=0; i<cbir_image_data.components; i++) {
        fgetc(file); /* Component ID - should always match order in SOF header */

        unsigned char table_id = fgetc(file);
        unsigned char table_dc = table_id >> 4;
        unsigned char table_ac = table_id & 0x0f;

        components[i].huff_table_dc = components[i].huff_table_ac = 0;

        for (j=0; j<huffman_tables_size; j++) {
            if (huffman_tables[j]->table_id == table_dc && huffman_tables[j]->table_class == 0)
                components[i].huff_table_dc = j;
            if (huffman_tables[j]->table_id == table_ac && huffman_tables[j]->table_class == 1)
                components[i].huff_table_ac = j;
        }

        /* Table not found */
        if (components[i].huff_table_dc == 0 || components[i].huff_table_ac == 0) {
            cbir_last_error_message = (char*)error_messages[5];
            /* Things could still work though */
        }
    }

    /* Read data for progressive JPEG which we don't support */
    fgetc(file);
    fgetc(file);
    fgetc(file);
}



void cbir_read_huffman_tables(FILE* file) {
    /* Header length */
    int header_length = fgetc(file)*256 + fgetc(file);
    int i=2, j, k;

    while (i < header_length) {
        char opts = fgetc(file);
        i++;

        unsigned char table_id, table_class;
        table_id = opts & 0x0f;
        table_class = opts >> 4;

        /* Find this table in the list */
        struct Huffman_Table* table = 0;
        for (j=0; j<huffman_tables_size; j++) {
            if (huffman_tables[j]->table_id == table_id && huffman_tables[j]->table_class == table_class) {
                table = huffman_tables[j];
                break;
            }
        }

        /* Table not found, create a new one */
        if (table == 0) {
            table = (struct Huffman_Table*)malloc(sizeof(struct Huffman_Table));
            table->table_id = opts & 0x0f;
            table->table_class = opts >> 4;
            for (j=0; j<256; j++)
                table->codes[j] = table->code_lengths[j] = 0;
            for (j=0; j<65536; j++)
                table->lookup_table[j] = -1;
            huffman_tables[huffman_tables_size++] = table;
        }

        /* List of code lengths */
        unsigned char code_lengths[16];
        for (j=0; j<16; j++)
            code_lengths[j] = fgetc(file);
        i += 16;

        unsigned int code=0, element=0;

        /* For each length... */
        for (j=0; j<16; j++) {
            /* If code length is 0, we continue because no element is coded wiht "j" bits */
            if (code_lengths[j] == 0) {
                /*  When tree depth changes, we add additional bit, and shift code one place left */
                code *= 2;
                continue;
            }

            /* If there is at least one element coded with "i" bits, then we have to assign code to it */
            for (k=0; k<code_lengths[j]; k++) {
                element = fgetc(file);
                i++;

                table->codes[element]        = code;
                table->code_lengths[element] = j+1;

                /* Add right-aligned code to the code lookup table */
                int rightaligned = (code << (15-j));
                table->lookup_table[rightaligned] = element;

                /* Elements on the same tree depth have code incremented by one */
                code++;
            }

            /*  When tree depth changes, we add additional bit, and shift code one place left */
            code *= 2;
        }

#if DEBUGLEVEL>0
        printf ("Read Huffman table ID: %d class %d\n", table_id, table_class);
#endif

    }
}


/* Skip unneccessary header fileds */
void cbir_skip_header(FILE* file) {
    int i, len = fgetc(file)*256 + fgetc(file);
    for (i=0; i<len-2; i++) /* 2 bytes for len are included in len */
        fgetc(file);
}



void cbir_read_frame_header(FILE* file) {
    /* Header length */
    int i, header_length = fgetc(file)*256 + fgetc(file);

    /* Precision */
    if ((int)fgetc(file) != 8) {
        cbir_last_error_message = (char*)error_messages[3];
        for (i=0; i<header_length-3; i++)
            fgetc(file);
        return;
    }

    /* Taking picture height and width. These informations are stored in two bytes for each dimension */
    cbir_image_data.height = fgetc(file)*256 + fgetc(file);
    cbir_image_data.width = fgetc(file)*256 + fgetc(file);

    /* Taking information for number of components and components data */
    if ((int)fgetc(file) != 3) {
        cbir_last_error_message = (char*)error_messages[4];
        for (i=0; i<header_length-8; i++)
            fgetc(file);
        return;
    }

    cbir_image_data.components = 3;

    /* Every component has 4 information. ComponentID, Horizontal sampling, Vertical Sampling, and Quantization table ID */
    for (i=0; i<cbir_image_data.components; i++) {
        components[i].id = fgetc(file);

        unsigned char c = fgetc(file); /* subsampling */
        components[i].hfactor = c >> 4;
        components[i].vfactor = c & 0x0f;

        components[i].quantization_table = fgetc(file);
    }

#if DEBUGLEVEL>0
    printf("Image height is: %d and image width is: %d\n", cbir_image_data.height, cbir_image_data.width);
#endif
}



/* Main loop for parsing a JPEG file */
void cbir_parse_jpeg(FILE* file) {
    int i;

    if (fgetc(file) != 0xff || fgetc(file) != 0xd8) {
        cbir_last_error_message = (char*)error_messages[0];
        return;
    }

    while (!feof(file)) {
        if (fgetc(file) != 0xff) {
            cbir_last_error_message = (char*)error_messages[1];
#if DEBUGLEVEL>0
            printf("Error: 0xFF expected\n");
#endif
            continue; /* Keep looking for 0xff */
        }

        unsigned char marker = fgetc(file);
#if DEBUGLEVEL>1
        printf("Found marker %x\n", marker);
#endif

        switch (marker) {
            case 0xd8: /* SOI = Start Of Image */
                break;

            case 0xd9: /* EOI = End Of Image */
                break;

            case 0xe0: /* 0xe0 - 0xef : APPn = Application specific segments */
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
                cbir_skip_header(file);
                break;

            case 0xc0: /* SOF = Start Of Frame */
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
                cbir_read_frame_header(file);
                /* readFrameHeader(file, marker); */
                break;

            case 0xc4: /* DHT = Define Huffman Tables */
                gettimeofday(&start, NULL);
                cbir_read_huffman_tables(file);
                /* readHuffmanTables(file); */
                gettimeofday(&end, NULL);
                idctTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
                break;

            case 0xcc: /* DAC = Define Arithmetic Coding */
                cbir_skip_header(file);
                break;

            case 0xdb: /* DQT = Define Quantization Tables */
                cbir_skip_header(file); /* We will not quantize */
                /* readQuantizationTables(file); */
                break;

            case 0xdc: /* DNL = Define Number of Lines */
                /* DNL is always 4 bytes long
                   It is only used if a scan has different number of lines from previous scan (progressive?)
                   [ ITU-T T.81, chapter B.2.5 ]
                   If you find a progressive JPEG with this feature, please implement */
                for (i=0; i<4; i++)
                    fgetc(file);
                break;

            case 0xdd: /* DRI = Define Restart Interval */
                /* DRI is always 4 bytes long
                   We are currently just ignoring restart markers */
                for (i=0; i<4; i++)
                    fgetc(file);
                break;

            case 0xde: /* DHP = Define Hierarchical Progression */
            case 0xdf: /* EXP = Expand Reference Components */
                cbir_last_error_message = (char*)error_messages[2];
                return;

            case 0xfe: /* COM = Comment */
                cbir_skip_header(file);
                break;

            case 0xda: /* SOS = Start Of Scan */
                cbir_read_scan_header(file);
                /*readScanHeader(file);*/

                /* Scan starts immediately after header */
                gettimeofday(&start, NULL);
                cbir_read_image_data(file);
                /*readImageData(file);*/
                gettimeofday(&end, NULL);
                huffmanTime += ((end.tv_sec  - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
                return;
                break;

            default:
                /* Unknown marker - lets ignore and hope for the best... */
                break;

        }

    }
}


void cbir_init_stats() {
    int i,j;
    for (i=0; i<MAX_COMPONENTS; i++) {
        colorFeaturesCounters[i] = colorFeaturesDoubleCounters[i] = 0;

        for (j=0; j<4; j++) {
            colorFeatures[i][j] = colorFeaturesLast1000[i][j] = 0;
            cbir_featureVectorME[i][j] = 0;
        }
        for (j=0; j<6; j++) {
            specificBlocks[i][j] = specificBlocksLast1000[i][j] = 0;
            specificBlocksSquares[i][j] = specificBlocksSquaresLast1000[i][j] = 0;
            cbir_featureVectorD[i][j] = cbir_featureVectorD[i][j+6] = 0;
        }
    }
}


/* Main function */
void cbir_decode(const char* filename) {
    /* Initialize */
    cbir_init_stats();
    cbir_parse_successful = 0;
    cbir_image_data.width = cbir_image_data.height = 0;
    huffman_tables_size = 0;
    end_of_file = restart_marker = -1;

    //FILE* file = fopen("/home/vedran/mms/etfshop/branches/interface_change/etfshop/samples/test.jpg", "r");
    FILE* file = fopen(filename, "r");
    if (file) {
        cbir_parse_jpeg(file);
        fclose(file);
        cbir_final_stats();
    }
}
