/*
 * Part III
 * Programming Assignment 3, Csc 360
 * Braden Simpson, V00685500
 */
#include "diskget.h"
int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("Usage: ./diskget [.img file] [filename]\n");
		return 0;
	}
	infile = fopen(argv[1], "r");

	int FileSize, StartPtr, FATCount, FSCount, RootPtr, RootCount, currentSegmentSize, block;
	currentSegmentSize = 2;
	void *currentPtr = malloc(currentSegmentSize);
	fseek(infile, 8, SEEK_CUR);			/* Skip past the name */
	/* block determines which part of the superblock to examine */
	/* Read Necessary parts of the superblock, can make NO assumptions */
	for (block = 0;block < 6;block++) {
		switch(block) {
			case 0:
				currentSegmentSize = 2;
				currentPtr = malloc(currentSegmentSize);
				fread(currentPtr, 1, currentSegmentSize, infile);
				memcpy(&BlockSize, currentPtr, currentSegmentSize);
				BlockSize = ntohs(BlockSize);
				break;
			case 1:
				currentSegmentSize = 4;
				currentPtr = malloc(currentSegmentSize);
				fread(currentPtr, 1, currentSegmentSize, infile);
				memcpy(&FSCount, currentPtr, currentSegmentSize);
				FSCount = ntohl(FSCount);
				break;
			case 2:
				currentPtr = malloc(currentSegmentSize);
				fread(currentPtr, 1, currentSegmentSize, infile);
				memcpy(&StartPtr, currentPtr, currentSegmentSize);
				StartPtr = ntohl(StartPtr);
				break;
			case 3:
				currentPtr = malloc(currentSegmentSize);
				fread(currentPtr, 1, currentSegmentSize, infile);
				memcpy(&FATCount, currentPtr, currentSegmentSize);
				FATCount = ntohl(FATCount);
				break;
			case 4:
				currentPtr = malloc(currentSegmentSize);
				fread(currentPtr, 1, currentSegmentSize, infile);
				memcpy(&RootPtr, currentPtr, currentSegmentSize);
				RootPtr = ntohl(RootPtr);
				break;
			case 5:
				currentPtr = malloc(currentSegmentSize);
				fread(currentPtr, 1, currentSegmentSize, infile);
				memcpy(&RootCount, currentPtr, currentSegmentSize);
				RootCount = ntohl(RootCount);
				break;
		}
		free(currentPtr);
		currentPtr = NULL;
	}

	FatTable = (unsigned int *)malloc(sizeof(unsigned int) * FSCount);
	/* Fill up the FatTable */
	rewind(infile);
	fseek(infile, BlockSize * StartPtr, SEEK_CUR);
	currentPtr = NULL;
	currentPtr = malloc(4);
	int hexVal, bytes, i;
	bytes = 0;
	for (i = 0;i < FSCount;i++) {
		bytes += fread(currentPtr, 1, 4, infile);
		memcpy(&hexVal, currentPtr, 4);
		hexVal = ntohl(hexVal);
		FatTable[i] = hexVal;
		free(currentPtr);
		currentPtr = NULL;
		currentPtr = malloc(4);
	}

	/* Now find the file to copy */
	rewind(infile);
	fseek(infile, RootPtr * BlockSize, SEEK_CUR);					/* Move to the root directory	*/
	currentSegmentSize = 64;								/* size Directory Entries		*/
	currentPtr = malloc(currentSegmentSize);
	/* read in 1 directory entry at a time */
	int bytesRead = fread(currentPtr, 1, currentSegmentSize, infile);
	while(1) {
		if (bytesRead >= RootCount*BlockSize) break;				/* gone through everything in root		*/
		if (IsCorrectNode(currentPtr, argv[2])) {			/* check if the desired node			*/
			currentPtr += 9;
			memcpy(&FileSize, currentPtr, 4);
			FileSize = ntohl(FileSize);
			currentPtr -= 9;
			WriteToLocalFS(currentPtr, argv[2], StartPtr, FileSize);
			return 0;
			break;
		}
		/* read 64 more bytes	*/
		else {
			bytesRead += fread(currentPtr, 1, currentSegmentSize, infile);
		}
	}
	printf("File not found.\n");
	fclose(infile);
	return 0;
}

/* This function reads the memory in <currentPtr> and	*
 * checks the file name for the desired file.			*/
int IsCorrectNode(void *currentPtr, char *inString)
{
	char *filename = (char*)malloc(31);
	/* skip to the filename */
	currentPtr += 27;
	memcpy(filename, currentPtr, 31);
	currentPtr += 37;					/* skip the unused bytes 		*/
	if (strcmp(inString, filename) == 0) {
		currentPtr -= 64;								/* roll back pointer to start			*/
		free(filename);
		return 1;
	}
	free(filename);
	currentPtr -= 64;								/* roll back pointer to start			*/
	return 0;
}

int WriteToLocalFS(void *currentPtr, char *inString, int FatStart, int FileSize)
{
	FILE *outfile = fopen(inString, "w");
	char buf[BlockSize];
	DEntry Entry;
	/* Get the status byte, no need for ntohs because */
	memcpy(&Entry.status, currentPtr, 1);
	currentPtr++;

	if (!CHECK_BIT(Entry.status, 0))						/* checks if the bit in <pos> is 1	*/
		return 1;

	/* Get the starting block	*/
	memcpy(&Entry.startBlock, currentPtr, 4);
	Entry.startBlock = ntohl(Entry.startBlock);
	currentPtr += 4;

	/* rewind the file to the beginning */
	rewind(infile);
	/* Find the start block of our file we want. */
	fseek(infile, BlockSize * Entry.startBlock, SEEK_CUR);
	int bytesRead = 0;
	int currentBlock = Entry.startBlock;
	if (FileSize <= BlockSize) {
		bytesRead = fread(buf, 1, FileSize, infile);
		FileSize = 0;
	}
	else {
		bytesRead = fread(buf, 1, BlockSize, infile);
		FileSize -= BlockSize;
	}
	fwrite(buf, 1, bytesRead, outfile);

	while (1) {
		if (FatTable[currentBlock] == 0xFFFFFFFF) {
			break;
		}
		rewind(infile);
		fseek(infile, BlockSize*FatTable[currentBlock], SEEK_CUR);
		if (FileSize <= BlockSize) {
			bytesRead = fread(buf, 1, FileSize, infile);
			FileSize = 0;
		}
		else {
			bytesRead = fread(buf, 1, BlockSize, infile);
			FileSize -= BlockSize;
		}
		fwrite(buf, 1, bytesRead, outfile);
		currentBlock = FatTable[currentBlock];
	}
	fclose(outfile);
	return 0;
}
