// datacompress.h
//
// Created by Dongjun Suh on 05/31/16.

#ifndef datacompress_h
#define datacompress_h

// compresses the source buffer into the destination buffer
char *compressString(char *source, unsigned long int sourceLen, unsigned long int *destLen);

// decompresses the source buffer into the destination buffer
char *decompressString(char *source, unsigned long int sourceLen, unsigned long int *destLen);

#endif // datacompress_h