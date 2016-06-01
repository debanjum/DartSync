// datacompress.c
//
// Created by Dongjun Suh on 05/31/16.
//
// This file is used to compress and decompress files that are sent to and received from peers
// in our DartSync project.
//
// NOTE: Referred to http://www.zlib.net/manual.html to learn more about compression and decompression
// functions and how to use them properly.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

char* compressString(char* source, unsigned long int sourceLen, unsigned long int *destLen) {
	// compressBound is called before compress() to allocate the destination buffer
	*destLen = compressBound(sourceLen);

	// allocate memory for the destination buffer
	char *dest = calloc(*destLen, sizeof(char));

	// we then call compress() to compress the source buffer into the destination buffer
	int code = compress((unsigned char*) dest, destLen, (unsigned char*) source, sourceLen);

	// if the compression was not successful, check to see which error occurred (Z_MEM_ERROR/Z_BUF_ERROR)
	if (code != Z_OK) {
		if (code == Z_MEM_ERROR) {
			perror("Not enough memory");
		}
		else if (code == Z_BUF_ERROR) {
			perror("Not enough room in the output buffer");
		}
		else {
			perror("An error occurred");
		}

		// free the destination buffer and exit upon failure
		free(dest);
		exit(0);
	}

	dest = (char*)realloc(dest, *destLen);
	return dest;	// return the compressed string upon success
}

char* decompressString(char *source, unsigned long int sourceLen, unsigned long int *destLen) {
	// allocate memory for the destination buffer
	char *dest = calloc(*destLen, sizeof(char));

	// we then call uncompress to decompress the source buffer into the destination buffer
	int code = uncompress((unsigned char*) dest, destLen, (unsigned char*) source, sourceLen);

	// if the decompression was not successful, check to see which error occurred
	if (code != Z_OK && code != -3) {
		if (code == Z_MEM_ERROR) {
			perror("Not enough memory");
		}
		else if (code == Z_BUF_ERROR) {
			perror("Not enough room in the output buffer");
		}
		else {
			perror("An error occurred");
		}

		// free the destination buffer and exit upon failure
		free(dest);
		exit(0);
	}

	dest = (char*)realloc(dest, *destLen);
	return dest;	// return the decompressed string upon success
}

