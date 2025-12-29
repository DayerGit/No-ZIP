#pragma once
#include <stdint.h>
#include <fstream>

#define _sizeof(a, b) (sizeof(a) - b*sizeof(uint8_t*))

#pragma pack(push, 1)

struct DataDescriptor {
	uint32_t signature;
	uint32_t CRC32;
	uint32_t compressSize;
	uint32_t unCompressSize;
};

struct LocalFileHeader {
	uint32_t signature;
	uint16_t versionNeededToExtract;
	uint16_t generalPurposeBitFlag;
	uint16_t compressionMethod;
	uint16_t lastModTime;
	uint16_t lastModDate;
	uint32_t CRC32;
	uint32_t compressSize;
	uint32_t unCompressSize;
	uint16_t lenOfFileName;
	uint16_t lenOfExtraField;

	uint8_t* fileName;
	uint8_t* extraField;
};

struct CentralDirectory {
	uint32_t signature;
	uint16_t versionMadeBy;
	uint16_t versionNeededToExtract;
	uint16_t generalPurposeBitFlag;
	uint16_t compressionMethod;
	uint16_t lastModTime;
	uint16_t lastModDate;
	uint32_t CRC32;
	uint32_t compressSize;
	uint32_t unCompressSize;
	uint16_t lenOfFileName;
	uint16_t lenOfExtraField;
	uint16_t lenOfComment;
	uint16_t numOfDisk;
	uint16_t internalAttributes;
	uint32_t externalAttributes;
	uint32_t offsetOfLocalHeader;

	uint8_t* fileName;
	uint8_t* extraField;
	uint8_t* comment;
};

struct EndOfCentralDirectory {
	uint32_t signature;
	uint16_t numOfDisk;
	uint16_t numOfDiskWhereStartCD;
	uint16_t numOfCDRecordsOnThisDisk;
	uint16_t totalNumOfCDRecords;
	uint32_t sizeOfCD;
	uint32_t offsetOfCD;
	uint16_t lenOfComment;

	uint8_t* comment;
};

#pragma pack(push)

typedef struct {
	FILE* file;
	CentralDirectory* CD;
	EndOfCentralDirectory endCD;
} ZipFile;