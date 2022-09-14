//MIT License
//Copyright (c) 2022 Jared Loewenthal
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.


//NSNet2 Offline Converter for Wave Audio Files for Windows
//Initial Version 0.1
//To be compiled with gcc using MinGW-w64
// More Details Here in the future

//This define should be placed at the top (before any includes)
//This dictates that Windows should use UNICODE type strings (good for when working with files)
#define UNICODE
#define WIN32_LEAN_AND_MEAN //Excludes uneccsarry includes when using windows.h
#include <windows.h> //Includes win32 functions and helper macros (uses UNICODE define)
#include <Commdlg.h> //Includes win32 command dialog "pop-up" boxes used for choosing an input file
#define _UNICODE
#include <tchar.h> //needed for _tcscat_s

#include <stdint.h>	//Defines Data Types: https://en.wikipedia.org/wiki/C_data_types
#include <stdlib.h>	//Needed for memory operations (memcpy / memset) ...needed? / included in windows.h?
#include <stdio.h>	//Needed for printf "debug" statements and non-Windows file operations

#include "fftw3.h"	//FFT Library Header

#include "Win32Performance.h" //Windows Performance Counter Helper File

#define CONVERSION_SEGMENTS 2000 //20sec converted at a time, can be changed to 1 for quasi-realtime live version
#define EXTRA_TIMING 1

//link in externaly created hannWindow.o "compiled" data file
extern float _binary___hannWindow_data_start[];
static float* hannWindow = _binary___hannWindow_data_start;

//These typedefs will get removed / modified in future versions:
typedef struct threadParameters {
	uint64_t iterations;
	float* Mptr;
	float* Xptr;
	float* Yptr;
} threadParametersT;

typedef struct threadParameters2 {
	uint64_t iterations;
	float* Mptr;
	float* Tptr; //temp storage pointer
	float* hOffset; //GRU hidden state byte offset
	float* Hptr; //GRU hidden state pointer
	float* Xptr;
	float* Yptr;
	float* Eptr; //Exponential calculation constants pointer
} threadParameters2T;

typedef struct threadParameters3 {
	uint64_t iterations;
	float* Mptr;
	float* Hptr; //GRU previous state pointer
	
	float* HTptr; //temp storage pointer
	float* RTptr; //temp storage pointer
	float* ZTptr; //temp storage pointer
	
	float* Yptr;
	
	float* Eptr; //Exponential calculation constants pointer
	float* hOffset; //GRU hidden state byte offset
	
} threadParameters3T;

// Declare the functions made in the assembly file and will be linked with this file with gcc / ld
extern int inData16ToWindowData(uint8_t* inDataPtr, float* adjHannWindowPtr, float* windowDataNextPtr, float* windowDataNowPtr);
extern int windowDataToOutData16(float* windowDataInvPtr, float* adjHannWindowInvPtr, float* windowDataAccumPtr, uint8_t* outDataPtr);
extern int inData32ToWindowData(uint8_t* inDataPtr, float* adjHannWindowPtr, float* windowDataNextPtr, float* windowDataNowPtr);
extern int windowDataToOutData32(float* windowDataInvPtr, float* adjHannWindowInvPtr, float* windowDataAccumPtr, uint8_t* outDataPtr);
extern int computeMAT5Max0asm(uint64_t iterations, float* matrixPtr, float* inputPtr, float* outputPtr);
extern int computeMAT15GRUasm(uint64_t iterations, float* matrixPtr, float* inputPtr, float* outputPtr);
extern int computeMAT15GRUasm2(uint64_t iterations, float* matrixPtr, float* inputPtr, float* outputPtr);
extern int computeMAT8NoMaxasm(uint64_t iterations, float* matrixPtr, float* inputPtr, float* outputPtr);
extern int computeMAT5NoMaxasm(uint64_t iterations, float* matrixPtr, float* inputPtr, float* outputPtr);
extern int computeGRUasm32(threadParameters3T*);
extern int computeMAT1asm(float* matrixPtr, float* inputPtr, uint64_t iterations, float* outputPtr);
extern int absClampLog(float* fftDataPtr, float* fftAbsPtr, float* logConstantPtr, uint64_t conversions);
extern int sigmoidClampMultiply(float* mat4OutputPtr, float* fftDataPtr, float* expConstantPtr, uint64_t conversions);

#define DATA_FILE_LOCATION		L"networkData.bin" //neural network calculation data file name
#define INPUT_FILE_LOCATION		L"input.wav" //initial name of input file
//#define OUTPUT_FILE_LOCATION	L"output.wav"
#define OUTPUT_FILE_APPEND		L"-Enhanced.wav"

#define DATA_FILE_SIZE	24662852 //Expected networkData.bin file size in bytes

#define FRAMES_PER_CONVERSION	480 // 10 ms conversions sections (requires a lookahead of 10ms)
#define FRAMES_IN_WINDOW 960
#define FFT_NUMBER 1024
#define FFT_BINS 513 //(FFT_NUMBER >> 1) + 1; //(n/2 + 1) = 513

//Headers and chunk data used in .WAV file:
struct riffHeader { //RIFF File Header
	uint32_t idCode; //"Magic Number" File ID Code = "RIFF"
	uint32_t size;   //RIFF size in bytes (in most cases should = (file size - 8) bytes)
	uint32_t typeID; //RIFF file type -> should = "WAVE"
};

struct chunkHeader {
	uint32_t idCode;
	uint32_t size;
};

struct fmtChunkData {
	uint16_t formatType;	//Format type; needs to be 1 = Signed Linear Pulse-Code Modulation (LPCM) for the conversion
	uint16_t channelNum;	//Number of channels; can be 1 or 2 (mono or stereo) for the conversion
	uint32_t sampleRate;	//Sample rate; needs to be 48000 (Hz) for the conversion
	uint32_t dataRate;		//Data rate in bytes per second
	uint16_t frameBytes;	//Bytes in a frame (incorporates sample size and number of channels)
	uint16_t sampleBits;	//Bits per sample
};

//Small buffer used for copying between files
#define BUFFER_SIZE 1024
uint8_t buffer[BUFFER_SIZE];

static struct fmtChunkData fmtData;
static uint32_t dataSize;

//Read and Error check input wave file while creating the output wave file
int readInputAudioAndCreateOutput(HANDLE inFileHandle, HANDLE outFileHandle) { //Error 10-31	
	uint32_t riffHeadSize = sizeof(struct riffHeader);
	
	LARGE_INTEGER inFileSizeEx;
	BOOL fileRes = GetFileSizeEx(inFileHandle, &inFileSizeEx);
	if (fileRes == 0) {
		printf("Could not get file size\n");
		return 10;
	}
	LONGLONG inFileSize = inFileSizeEx.QuadPart;
	if (inFileSize < riffHeadSize) {
		printf("File size too big or too small\n"); //Need to add too big testing...
		return 11; //12 for too big
	}
	
	struct riffHeader riffHead;
	DWORD readBytes = 0;
	fileRes = ReadFile(inFileHandle, &riffHead, riffHeadSize, &readBytes, NULL);
	if (fileRes == 0) { //The only time the input file is checked for readability
		printf("Could not read from the file!\n");
		return 13;
	}
	if (readBytes != riffHeadSize) {
		printf("Incorrect number of read bytes!\n");
		return 14;
	}
	
	if (riffHead.idCode != 0x46464952) { //"RIFF"
		printf("Incorrect file ID Code (Magic Number != RIFF)\n");
		return 15;
	}
	
	if (riffHead.size != (inFileSize - 8)) {
		printf("WARNING: RIFF size does not match actual file size!\n");
		//return 16;
	}
	
	if (riffHead.typeID != 0x45564157) { //"WAVE"
		printf("Incorrect WAV ID Code (Magic Number != WAVE)\n");
		return 17;
	}
	
	uint32_t remainingSize = riffHead.size - 4;
	
	DWORD writtenBytes = 0;
	fileRes = WriteFile(outFileHandle, &riffHead, riffHeadSize, &writtenBytes, NULL);
	if (fileRes == 0) { //The only time the output file is checked for writeability
		printf("Could not write to the file!\n");
		return 18;
	}
	if (writtenBytes != riffHeadSize) {
		printf("Incorrect number of written bytes\n");
		return 19;
	}
	
	
	uint32_t chunkHeadSize = sizeof(struct chunkHeader);
	struct chunkHeader chunkHead;
	
	uint32_t fmtChunkSearch = 0;
	do {
		if (remainingSize < chunkHeadSize) {
			printf("Invalid File: Not enough bytes remaining!\n");
			return 20;
		}
		ReadFile(inFileHandle, &chunkHead, chunkHeadSize, &readBytes, NULL);
		if (readBytes != chunkHeadSize) {
			printf("Incorrect number of read bytes!\n");
			return 14;
		}
		remainingSize -= chunkHeadSize;
		
		if (chunkHead.idCode == 0x20746D66) { //"fmt "
			fmtChunkSearch = 1;
		}
		else {
			if (chunkHead.idCode == 0x61746164) { //"data"
				printf("data chunk found before fmt chunk!\n");
				return 21;
			}
			
			if (remainingSize < chunkHead.size) {
				printf("Invalid File: Not enough bytes remaining!\n");
				return 20;
			}
			
			WriteFile(outFileHandle, &chunkHead, chunkHeadSize, &writtenBytes, NULL);
			if (writtenBytes != chunkHeadSize) {
				printf("Incorrect number of written bytes\n");
				return 19;
			}
			
			uint32_t transfers = chunkHead.size / BUFFER_SIZE; //Truncation desired
			for (uint32_t t=0; t<transfers; t++) {
				ReadFile(inFileHandle, buffer, BUFFER_SIZE, &readBytes, NULL);
				if (readBytes != BUFFER_SIZE) {
					printf("Incorrect number of read bytes!\n");
					return 14;
				}
				
				WriteFile(outFileHandle, buffer, BUFFER_SIZE, &writtenBytes, NULL);
				if (writtenBytes != BUFFER_SIZE) {
					printf("Incorrect number of written bytes\n");
					return 19;
				}
			}
			
			uint32_t lastTransferSize = chunkHead.size % BUFFER_SIZE;
			ReadFile(inFileHandle, buffer, lastTransferSize, &readBytes, NULL);
			if (readBytes != lastTransferSize) {
				printf("Incorrect number of read bytes!\n");
				return 14;
			}
			
			WriteFile(outFileHandle, buffer, lastTransferSize, &writtenBytes, NULL);
			if (writtenBytes != lastTransferSize) {
				printf("Incorrect number of written bytes\n");
				return 19;
			}
			
			remainingSize -= chunkHead.size;
		}		
	} while (fmtChunkSearch == 0);
	
	//if (inHeader.fmtCode != "fmt ") {
	//	printf("Incorrect format ID Code (Magic Number != fmt )\n");
	//	return 18;
	//}
	
	if (chunkHead.size != 16) {
		printf("Wave format size != 16\n");
		return 22;
	}
	
	if (remainingSize < chunkHead.size) {
		printf("Invalid File: Not enough bytes remaining!\n");
		return 20;
	}
	
	uint32_t fmtChunkSize = sizeof(struct fmtChunkData);
	//struct fmtChunkData fmtData;
	
	ReadFile(inFileHandle, &fmtData, fmtChunkSize, &readBytes, NULL);
	if (readBytes != fmtChunkSize) {
		printf("Incorrect number of read bytes!\n");
		return 14;
	}
	remainingSize -= chunkHead.size;
	
	if (fmtData.formatType != 1) {
		printf("Wave format type != 1 (WAV format is not Signed LPCM)\n");
		return 23;
	}
	
	if ((fmtData.channelNum < 1) || (fmtData.channelNum > 2)) { //Channel Count Check
		printf("Wave file is not 1 or 2 channels (is not Mono or Stereo)!\n");
		return 24;
	}
	
	if (fmtData.channelNum == 2) {
		printf("Wave file for 2 channels (Stereo) will be supported in the FUTURE!\n");
		return 24;
	}
	
	if (fmtData.sampleRate != 48000) {
		printf("Wave file does not have a 48kHz sample rate (future versions might do resampling)\n");
		return 25;
	}
	
	uint32_t frameBytesModulo = fmtData.dataRate % fmtData.sampleRate;
	if (frameBytesModulo != 0) {
		printf("Invalid wave data rate!\n");
		return 26;
	}
	
	uint32_t frameBytesCalculated = fmtData.dataRate / fmtData.sampleRate;
	if (fmtData.frameBytes != frameBytesCalculated) {
		printf("Wave frame bytes does not match expected value when using the data rate!\n");
		return 27;
	}
	
	if (fmtData.channelNum == 1) {
		if ((fmtData.frameBytes != 2) && (fmtData.frameBytes != 4)) {
			printf("Mono wave frame bytes expected to be 2 or 4: %d\n", fmtData.frameBytes);
			return 28;
		}
	}
	//else if (fmtData.channelNum == 2) {
	else { //Gaurenteed to be 2 here based on earlier check
		if ((fmtData.frameBytes != 4) && (fmtData.frameBytes != 8)) {
			printf("Stereo wave frame bytes expected to be 4 or 8!\n");
			return 29;
		}
	}
	
	uint32_t sampleBitsCalculated = fmtData.frameBytes << (4 - fmtData.channelNum); //works for channel count of 1 and 2
	if (fmtData.sampleBits != sampleBitsCalculated) {
		printf("Wave sample bits does not match expected value when using the frame bytes and channel count!\n");
		return 30;
	}
	
	WriteFile(outFileHandle, &chunkHead, chunkHeadSize, &writtenBytes, NULL);
	if (writtenBytes != chunkHeadSize) {
		printf("Incorrect number of written bytes\n");
		return 19;
	}
	
	WriteFile(outFileHandle, &fmtData, fmtChunkSize, &writtenBytes, NULL);
	if (writtenBytes != fmtChunkSize) {
		printf("Incorrect number of written bytes\n");
		return 19;
	}
	
	
	
	uint32_t dataChunkSearch = 0;
	do {
		if (remainingSize < chunkHeadSize) {
			printf("Invalid File: Not enough bytes remaining!\n");
			return 20;
		}
		ReadFile(inFileHandle, &chunkHead, chunkHeadSize, &readBytes, NULL);
		if (readBytes != chunkHeadSize) {
			printf("Incorrect number of read bytes!\n");
			return 14;
		}
		remainingSize -= chunkHeadSize;
		
		if (chunkHead.idCode == 0x61746164) { //"data"
			dataChunkSearch = 1;
		}
		else {			
			if (remainingSize < chunkHead.size) {
				printf("Invalid File: Not enough bytes remaining!\n");
				return 20;
			}
			
			WriteFile(outFileHandle, &chunkHead, chunkHeadSize, &writtenBytes, NULL);
			if (writtenBytes != chunkHeadSize) {
				printf("Incorrect number of written bytes\n");
				return 19;
			}
			
			uint32_t transfers = chunkHead.size / BUFFER_SIZE; //Truncation desired
			for (uint32_t t=0; t<transfers; t++) {
				ReadFile(inFileHandle, buffer, BUFFER_SIZE, &readBytes, NULL);
				if (readBytes != BUFFER_SIZE) {
					printf("Incorrect number of read bytes!\n");
					return 14;
				}
				
				WriteFile(outFileHandle, buffer, BUFFER_SIZE, &writtenBytes, NULL);
				if (writtenBytes != BUFFER_SIZE) {
					printf("Incorrect number of written bytes\n");
					return 19;
				}
			}
			
			uint32_t lastTransferSize = chunkHead.size % BUFFER_SIZE;
			ReadFile(inFileHandle, buffer, lastTransferSize, &readBytes, NULL);
			if (readBytes != lastTransferSize) {
				printf("Incorrect number of read bytes!\n");
				return 14;
			}
			
			WriteFile(outFileHandle, buffer, lastTransferSize, &writtenBytes, NULL);
			if (writtenBytes != lastTransferSize) {
				printf("Incorrect number of written bytes\n");
				return 19;
			}
			
			remainingSize -= chunkHead.size;
		}		
	} while (dataChunkSearch == 0);
	
	
	if (remainingSize < chunkHead.size) {
		printf("Invalid File: Not enough bytes remaining for wave data!\n");
		return 31;
	}
	
	WriteFile(outFileHandle, &chunkHead, chunkHeadSize, &writtenBytes, NULL);
	if (writtenBytes != chunkHeadSize) {
		printf("Incorrect number of written bytes\n");
		return 19;
	}
	
	dataSize = chunkHead.size;
	
	return 0;
}

//Sizes of matrix and gru stages
#define MATRIX_MULT1_SIZE 600
#define GRU1_HIDDEN_SIZE 600
#define GRU2_HIDDEN_SIZE 600
#define MATRIX_MULT2_SIZE 800
#define MATRIX_MULT3_SIZE 800
#define MATRIX_MULT4_SIZE 513

//Data pointers for loaded in network data
static float* dataMAT1 = NULL;
static float* dataGRU1 = NULL;
static float* dataGRU2 = NULL;
static float* dataMAT2 = NULL;
static float* dataMAT3 = NULL;
static float* dataMAT4 = NULL;

//Load in network data into aligned allocated memory
int readDataFileIntoMemory(HANDLE inFileHandle) { //Error 20-49
	DWORD readInBytes = ((FFT_BINS * MATRIX_MULT1_SIZE) + MATRIX_MULT1_SIZE) * sizeof(float);
	
	dataMAT1 = _aligned_malloc(readInBytes, 64);
	//printf("dataMAT1: %p\n", dataMAT1);
	
	DWORD readBytes = 0;
	BOOL fileRes = ReadFile(inFileHandle, dataMAT1, readInBytes, &readBytes, NULL);
	if (fileRes == 0) {
		printf("Error reading from the file: 0x%lX\n", GetLastError());
		return 22;
	}
	if (readBytes != readInBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}	
	
	readInBytes = ((600 * 600 * 6) + (600 * 4)) * sizeof(float);
	dataGRU1 = _aligned_malloc(readInBytes, 64);
	fileRes = ReadFile(inFileHandle, dataGRU1, readInBytes, &readBytes, NULL);
	if (fileRes == 0) {
		printf("Error reading from the file!\n");
		return 22;
	}
	if (readBytes != readInBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}
	
	readInBytes = ((600 * 600 * 6) + (600 * 4)) * sizeof(float);
	dataGRU2 = _aligned_malloc(readInBytes, 64);
	fileRes = ReadFile(inFileHandle, dataGRU2, readInBytes, &readBytes, NULL);
	if (fileRes == 0) {
		printf("Error reading from the file!\n");
		return 22;
	}
	if (readBytes != readInBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}
	
	readInBytes = ((GRU2_HIDDEN_SIZE * MATRIX_MULT2_SIZE) + MATRIX_MULT2_SIZE) * sizeof(float);
	dataMAT2 = _aligned_malloc(readInBytes, 64);
	fileRes = ReadFile(inFileHandle, dataMAT2, readInBytes, &readBytes, NULL);
	if (fileRes == 0) {
		printf("Error reading from the file!\n");
		return 22;
	}
	if (readBytes != readInBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}
	
	readInBytes = 32040 * 20 * sizeof(float); //((MATRIX_MULT2_SIZE * MATRIX_MULT3_SIZE) + MATRIX_MULT3_SIZE) * sizeof(float);
	dataMAT3 = _aligned_malloc(readInBytes, 64);
	fileRes = ReadFile(inFileHandle, dataMAT3, readInBytes, &readBytes, NULL);
	if (fileRes == 0) {
		printf("Error reading from the file!\n");
		return 22;
	}
	if (readBytes != readInBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}	
	
	readInBytes = ((MATRIX_MULT3_SIZE * MATRIX_MULT4_SIZE) + MATRIX_MULT4_SIZE) * sizeof(float);
	dataMAT4 = _aligned_malloc(readInBytes, 64);
	fileRes = ReadFile(inFileHandle, dataMAT4, readInBytes, &readBytes, NULL);
	if (fileRes == 0) {
		printf("Error reading from the file!\n");
		return 22;
	}
	if (readBytes != readInBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}
	
	return 0;
}

//Used only once...will be changed in future
int getNextData(HANDLE inFileHandle, uint8_t* inData, size_t numBytes) {
	DWORD readBytes = 0;
	BOOL fileRes = ReadFile(inFileHandle, inData, numBytes, &readBytes, NULL);
	if (fileRes == 0) { //The only error checking done for reading the file in this function
		printf("Error reading from the file!\n");
		return 50;
	}
	if (readBytes != numBytes) {
		printf("Incorrect number of read bytes\n");
		return 23;
	}
	
	return 0;
}

//Used to link only necessary functions from the fftw static library
void fftwf_configure_planner(void *plnr) {
    struct solvtab_s { void (*reg)(void *); const char *reg_nam; };
    extern void fftwf_solvtab_exec(const struct solvtab_s s[], void *);

#define DECLARE(name) extern void name(void *);
#define STRINGIZEx(x) #x
#define STRINGIZE(x) STRINGIZEx(x)
#define SOLVTAB(s) { s, STRINGIZE(s) },
#define DO(X) \
    X(fftwf_codelet_hc2cbdft_2)\
    X(fftwf_codelet_hc2cfdft_2)\
    X(fftwf_codelet_n1_8)\
    X(fftwf_codelet_r2cb_2)\
    X(fftwf_codelet_r2cbIII_2)\
    X(fftwf_codelet_r2cf_2)\
    X(fftwf_codelet_r2cfII_2)\
    X(fftwf_codelet_t1sv_2_avx)\
    X(fftwf_codelet_t1sv_32_avx2)\
    X(fftwf_codelet_t2bv_64_avx)\
    X(fftwf_dft_vrank_geq1_register)\
    /* end DO(X) */

    DO(DECLARE)

    const struct solvtab_s s[] = {
        DO(SOLVTAB)
        { 0, 0 }
    };

    fftwf_solvtab_exec(s, plnr);
}

//Perform the Bulk Conversion
//Need to better handle last conversions of data
int performConversion(HANDLE inFileHandle, HANDLE outFileHandle) {
	float* dataIO = _aligned_malloc(28096*2, 64);
	
	uint8_t* inData = (uint8_t*) dataIO;
	float* adjHannWindow = (float*) (inData + (480*4));
	float* windowData1 = adjHannWindow + 960;
	float* windowData2 = windowData1 + (1024);
	float* logConstants = windowData2 + (1024);
	
	float* fftData = logConstants + (16);
	float* fftDataH = fftData + (513+7);
	float* expConstants = fftData + (513+7)*2;
	
	float* windowDataInv = expConstants + (16);
	float* adjHannWindowInv = windowDataInv + (1024);
	float* windowDataAccum = adjHannWindowInv + (960);
	uint8_t* outData = (uint8_t*) (windowDataAccum + 480);
	//uint8_t* nextPointer = outData + (480*4);
	
	//*
	float* fftDataM = _aligned_malloc(4160 * (CONVERSION_SEGMENTS + 0), 64); //2*(513+7)*4
	float* calcDat1 = _aligned_malloc(3200 * (CONVERSION_SEGMENTS + 1), 64);
	float* calcDat2 = _aligned_malloc(3200 * (CONVERSION_SEGMENTS + 1), 64); //800*4
	
	float* ztData = _aligned_malloc((600*4) * (CONVERSION_SEGMENTS), 64);
	float* rtData = _aligned_malloc((600*4) * (CONVERSION_SEGMENTS), 64);
	float* htData = _aligned_malloc((600*4) * (CONVERSION_SEGMENTS), 64);
	//*/
	
	
	memcpy(adjHannWindow, hannWindow, FRAMES_IN_WINDOW * sizeof(float));
	
	memset(windowData1 + (960), 0, (64) * sizeof(float));
	memset(windowData2 + (960), 0, (64) * sizeof(float));
	
	uint32_t* logConstantsInt = (uint32_t*) logConstants;
	logConstants[0] = 1.0;
	logConstantsInt[1] = 0x00400000;
	logConstantsInt[2] = 0x7F800000;
	logConstantsInt[3] = 0x007FFFFF;
	logConstants[4] = 0.1501692;
	logConstants[5] = 3.4226132;
	logConstants[6] = 5.0225057;
	logConstants[7] = 4.1130283;
	logConstants[8] = 3.4813372;
	logConstants[9]  = 7.9034151668e-07;
	logConstants[10] = 3.0102920532e-01;
	logConstants[11] = 0.000000000001; //before log clamp
	
	expConstants[0] = -1.442695041f;
	expConstants[1] = -6.93145752e-1f;
	expConstants[2] = -1.42860677e-6f;
	expConstants[3] = 1.3836846e-3f;
	expConstants[4] = 8.3748158e-3f;
	expConstants[5] = 4.1668226e-2f;
	expConstants[6] = 1.666642e-1f;
	expConstants[7] = 4.9999992e-1f;
	expConstants[8] = +1.0f;
	uint32_t* expConstantConvert = (uint32_t*) &expConstants[9];
	*expConstantConvert = 0x00800000;
	expConstants[10] = -20.0f;
	expConstants[11] = 0.00009999999747378752; //after sigmoid clamp
	
	memcpy(adjHannWindowInv, adjHannWindow, FRAMES_IN_WINDOW * sizeof(float));
	
	memset(windowDataAccum, 0, (480) * sizeof(float)); //Might want to change this...
	
	memset(calcDat1, 0, (600+8) * sizeof(float));
	memset(calcDat2, 0, (600+8) * sizeof(float));
	
	int frameBytes = fmtData.frameBytes >> (fmtData.channelNum - 1);
	int frameBits  = frameBytes << 3; // Sample / Frame Bits (Frame Bytes * 8)
	float frameMultiplier = (float) (1 << (frameBits - 1));
	
	//printf("Frame Bytes: %d\n", frameBytes);
	
	for (uint32_t d=0; d<FRAMES_IN_WINDOW; d++) {
		adjHannWindow[d] /= frameMultiplier;
		adjHannWindowInv[d] *= frameMultiplier / FFT_NUMBER; //Unormalized inverse result so need to divide result by FFT_NUMBER to get proper inverse
	}
	
	float* windowData12 = &windowData1[FRAMES_PER_CONVERSION];
	float* windowData22 = &windowData2[FRAMES_PER_CONVERSION];
	
	float* windowData = windowData1;
	float* windowDataSecond = windowData12;
	float* windowDataCopy = windowData2;
	float* windowDataNext = windowData22;
	float* windowDataTemp = NULL;
	
	fftwf_iodim fftInfo[1];
	fftInfo[0].n = FFT_NUMBER;
	fftInfo[0].is = 1;
	fftInfo[0].os = 1;
	
	//Preserve input and measure is default FFTW_PRESERVE_INPUT | FFTW_MEASURE
	fftwf_plan fftPlan = fftwf_plan_guru_split_dft_r2c(1, fftInfo, 0, NULL, windowData1, fftData, fftDataH, 0); //FFTW_PATIENT FFTW_EXHAUSTIVE
	
	//Unormalized inverse result so need to divide result by FFT_NUMBER to get proper inverse
	fftwf_plan fftInvPlan = fftwf_plan_guru_split_dft_c2r(1, fftInfo, 0, NULL, fftData, fftDataH, windowDataInv, 0);
	
	/*
	char* wisdomStr = fftwf_export_wisdom_to_string();
	printf("FFT Wisdom %s\n", wisdomStr);
	free(wisdomStr);
	//*/
	
	int (*inDataToWindowData)(uint8_t*,float*,float*,float*);
	int (*windowDataToOutData)(float*,float*,float*,uint8_t*);
	
	size_t conversionBytes = frameBytes * FRAMES_PER_CONVERSION;
	getNextData(inFileHandle, inData, conversionBytes);
	
	if (frameBytes == 2) {
		int16_t* inD = (int16_t*) inData;
		for (uint32_t d=0; d<FRAMES_PER_CONVERSION; d++) {
			windowData[d] = (float) inD[d];
			windowData[d] *= adjHannWindow[d];
		}
		inDataToWindowData = inData16ToWindowData;
		windowDataToOutData = windowDataToOutData16;
	}
	else {
		int32_t* inD = (int32_t*) inData;
		for (uint32_t d=0; d<FRAMES_PER_CONVERSION; d++) {
			windowData[d] = (float) inD[d];
			windowData[d] *= adjHannWindow[d];
		}
		inDataToWindowData = inData32ToWindowData;
		windowDataToOutData = windowDataToOutData32;
	}
	
#if EXTRA_TIMING == 1
	LONGLONG totalTime = 0;
	LONGLONG part1Time = 0;
	LONGLONG part2Time = 0;
	LONGLONG part3Time = 0;
	LONGLONG part4Time = 0;
	LONGLONG part5Time = 0;	
	LONGLONG testTime = 0;
#endif
	
	printf("Running Conversion... Please Wait\n");
	
	LONGLONG conversionTime1 = 0;
	Win32PerformanceGetCount(&conversionTime1);
	
	uint32_t numConversions = (dataSize / conversionBytes); // Expecting truncation
	int conversions = CONVERSION_SEGMENTS; //30 second segments
	int segments = (numConversions / conversions) + 1;
	for (int s=1; s<=segments; s++) {

#if EXTRA_TIMING == 1
		LONGLONG startTime = 0;
		Win32PerformanceGetCount(&startTime);
#else
		printf("Converting Segment %d: %d\n", s, conversions);
#endif
		
		float* fftDataT = fftDataM;
		if (s != segments) { //Last Segment
			for (int c=0; c<conversions; c++) {
				ReadFile(inFileHandle, inData, conversionBytes, NULL, NULL);
			
				inDataToWindowData(inData, adjHannWindow, windowDataCopy, windowDataSecond);
				
				fftwf_execute_split_dft_r2c(fftPlan, windowData, fftDataT, fftDataT + 520);
				
				windowDataTemp = windowData;
				windowData = windowDataCopy;
				windowDataCopy = windowDataTemp;
				
				windowDataTemp = windowDataSecond;
				windowDataSecond = windowDataNext;
				windowDataNext = windowDataTemp;
				
				fftDataT += 1040;
			}
		}
		else {
			conversions = numConversions % conversions;
			
			for (int c=0; c<(conversions-1); c++) {
				ReadFile(inFileHandle, inData, conversionBytes, NULL, NULL);
			
				inDataToWindowData(inData, adjHannWindow, windowDataCopy, windowDataSecond);
				
				fftwf_execute_split_dft_r2c(fftPlan, windowData, fftDataT, fftDataT + 520);
				
				windowDataTemp = windowData;
				windowData = windowDataCopy;
				windowDataCopy = windowDataTemp;
				
				windowDataTemp = windowDataSecond;
				windowDataSecond = windowDataNext;
				windowDataNext = windowDataTemp;
				
				fftDataT += 1040;
			}
			memset(inData, 0, conversionBytes);
			inDataToWindowData(inData, adjHannWindow, windowDataCopy, windowDataSecond);
			fftwf_execute_split_dft_r2c(fftPlan, windowData, fftDataT, fftDataT + 520);
		}
		
#if EXTRA_TIMING == 1		
		LONGLONG readTime = 0;
		Win32PerformanceGetCount(&readTime);
#endif
		
		fftDataT = fftDataM;
		float* calcDat1T = calcDat1 + 800;
		absClampLog(fftDataT, calcDat1T, logConstants, conversions);
		
#if EXTRA_TIMING == 1		
		LONGLONG fftAbsTime = 0;
		Win32PerformanceGetCount(&fftAbsTime);
#endif		
		
		threadParametersT mat1WorkData;
		mat1WorkData.iterations = FFT_BINS;
		
		calcDat1T = calcDat1 + 800;
		float* calcDat2T = calcDat2 + 800;
		
		mat1WorkData.Mptr = (float*) dataMAT1;
		
		for (int i=0; i<15; i++) {
			mat1WorkData.Xptr = calcDat1T;
			mat1WorkData.Yptr = calcDat2T;
			
			for (int c=0; c<conversions; c++) {
				computeMAT5Max0asm(mat1WorkData.iterations, mat1WorkData.Mptr, mat1WorkData.Xptr, mat1WorkData.Yptr);
				
				mat1WorkData.Xptr += 800;
				mat1WorkData.Yptr += 800;
			}
			
			mat1WorkData.Mptr += 20560;
			calcDat2T += (5*8);
		}		

#if EXTRA_TIMING == 1		
		LONGLONG mul1Time = 0;
		Win32PerformanceGetCount(&mul1Time);
#endif
		
		//GRU1 from calcDat1 is Output, calcDat2 is Input
		
		threadParametersT gru1WorkDataNew;
		gru1WorkDataNew.iterations = 600;
		
		float* tempGRU[3] = {ztData, rtData, htData};
		
		gru1WorkDataNew.Mptr = (float*) dataGRU1;
		
		for (int t=0; t<3; t++) {
			calcDat1T = tempGRU[t] + 0;
			calcDat2T = calcDat2 + 800;
			
			for (int i=0; i<15; i++) {
				gru1WorkDataNew.Xptr = calcDat2T;
				gru1WorkDataNew.Yptr = calcDat1T;
				
				for (int c=0; c<conversions; c++) {
					computeMAT5NoMaxasm(gru1WorkDataNew.iterations, gru1WorkDataNew.Mptr, gru1WorkDataNew.Xptr, gru1WorkDataNew.Yptr);
					
					gru1WorkDataNew.Xptr += 800;
					gru1WorkDataNew.Yptr += 600;
				}
				
				gru1WorkDataNew.Mptr += 24040; // 600*(5*8) + (5*8)
				calcDat1T += (5*8);
			}
		}		
		
		threadParameters3T gru1WorkData;
		gru1WorkData.iterations = 600;
		gru1WorkData.Eptr = expConstants;
		
		calcDat1T = calcDat1 + 800;
		calcDat2T = calcDat2 + 800;
		gru1WorkData.Hptr = calcDat1T - 800;
		gru1WorkData.HTptr = htData;
		gru1WorkData.RTptr = rtData;
		gru1WorkData.ZTptr = ztData;
		
		gru1WorkData.Mptr = gru1WorkDataNew.Mptr;

#if EXTRA_TIMING == 1		
		LONGLONG test1Time = 0;
		LONGLONG test2Time = 0;
#endif
		
		for (int c=0; c<conversions; c++) {
			gru1WorkData.iterations = 600;
			gru1WorkData.Yptr = calcDat1T;

#if EXTRA_TIMING == 1
			Win32PerformanceGetCount(&test1Time);
#endif
			
			gru1WorkData.Mptr = gru1WorkDataNew.Mptr;
			for (int i=0; i<5; i++) {
				computeMAT15GRUasm(gru1WorkData.iterations, gru1WorkData.Mptr, gru1WorkData.Hptr, gru1WorkData.ZTptr);
				
				gru1WorkData.Mptr += 72000;
				gru1WorkData.ZTptr += 15*8;
			}

#if EXTRA_TIMING == 1			
			Win32PerformanceGetCount(&test2Time);
			testTime += test2Time-test1Time;
#endif
			
			for (int i=0; i<5; i++) {
				computeMAT15GRUasm(gru1WorkData.iterations, gru1WorkData.Mptr, gru1WorkData.Hptr, gru1WorkData.RTptr);
				
				gru1WorkData.Mptr += 72000;
				gru1WorkData.RTptr += 15*8;
			}
			
			for (int i=0; i<5; i++) {
				computeMAT15GRUasm2(gru1WorkData.iterations, gru1WorkData.Mptr, gru1WorkData.Hptr, gru1WorkData.Yptr);
				
				gru1WorkData.Mptr += 72120;
				gru1WorkData.Yptr += 15*8;
			}
			
			gru1WorkData.iterations = 75;
			gru1WorkData.hOffset = gru1WorkData.Hptr;
			gru1WorkData.Yptr = calcDat1T;
			
			gru1WorkData.ZTptr -= 5*8*15;
			gru1WorkData.RTptr -= 5*8*15;

#if EXTRA_TIMING == 1			
			//Win32PerformanceGetCount(&test1Time);
#endif
			
			computeGRUasm32(&gru1WorkData);

#if EXTRA_TIMING == 1		
			//Win32PerformanceGetCount(&test2Time);
			//testTime += test2Time-test1Time;
#endif
			
			gru1WorkData.RTptr += 5*8*15;
			gru1WorkData.ZTptr += 5*8*15;
			gru1WorkData.HTptr += 5*8*15; //600
			
			calcDat1T += 800;
			gru1WorkData.Hptr += 800;
		}
		
		memcpy(calcDat1, gru1WorkData.Hptr, 600 * sizeof(float));

#if EXTRA_TIMING == 1		
		LONGLONG gru1Time = 0;
		Win32PerformanceGetCount(&gru1Time);
#endif
		
		//GRU1 from calcDat2 is Output, calcDat1 is Input
		
		threadParametersT gru2WorkDataNew;
		gru2WorkDataNew.iterations = 600;
		
		gru2WorkDataNew.Mptr = (float*) dataGRU2;
		
		for (int t=0; t<3; t++) {
			calcDat1T = tempGRU[t] + 0;
			calcDat2T = calcDat1 + 800;
			
			for (int i=0; i<15; i++) {
				gru2WorkDataNew.Xptr = calcDat2T;
				gru2WorkDataNew.Yptr = calcDat1T;
				
				for (int c=0; c<conversions; c++) {
					computeMAT5NoMaxasm(gru2WorkDataNew.iterations, gru2WorkDataNew.Mptr, gru2WorkDataNew.Xptr, gru2WorkDataNew.Yptr);
					
					gru2WorkDataNew.Xptr += 800;
					gru2WorkDataNew.Yptr += 600;
				}
				
				gru2WorkDataNew.Mptr += 24040; // 600*(10*8) + (10*8)
				calcDat1T += (5*8);
			}
		}
		
		
		threadParameters3T gru2WorkData;
		gru2WorkData.iterations = 600;
		gru2WorkData.Eptr = expConstants;
		
		calcDat1T = calcDat2 + 800;
		calcDat2T = calcDat1 + 800;
		gru2WorkData.Hptr = calcDat1T - 800;
		gru2WorkData.HTptr = htData;
		gru2WorkData.RTptr = rtData;
		gru2WorkData.ZTptr = ztData;
		
		for (int c=0; c<conversions; c++) {
			gru2WorkData.iterations = 600;
			gru2WorkData.Yptr = calcDat1T;
			
			gru2WorkData.Mptr = gru2WorkDataNew.Mptr;
			for (int i=0; i<5; i++) {
				computeMAT15GRUasm(gru2WorkData.iterations, gru2WorkData.Mptr, gru2WorkData.Hptr, gru2WorkData.ZTptr);
				
				gru2WorkData.Mptr += 72000;
				gru2WorkData.ZTptr += 15*8;
			}
			
			for (int i=0; i<5; i++) {
				computeMAT15GRUasm(gru2WorkData.iterations, gru2WorkData.Mptr, gru2WorkData.Hptr, gru2WorkData.RTptr);
				
				gru2WorkData.Mptr += 72000;
				gru2WorkData.RTptr += 15*8;
			}
			
			for (int i=0; i<5; i++) {
				computeMAT15GRUasm2(gru2WorkData.iterations, gru2WorkData.Mptr, gru2WorkData.Hptr, gru2WorkData.Yptr);
				
				gru2WorkData.Mptr += 72120;
				gru2WorkData.Yptr += 15*8;
			}
			
			gru2WorkData.iterations = 75;
			gru2WorkData.hOffset = gru2WorkData.Hptr;
			gru2WorkData.Yptr = calcDat1T;
			gru2WorkData.RTptr -= 5*8*15;
			gru2WorkData.ZTptr -= 5*8*15;
			
			computeGRUasm32(&gru2WorkData);
			
			gru2WorkData.HTptr += 5*8*15;
			gru2WorkData.RTptr += 5*8*15;
			gru2WorkData.ZTptr += 5*8*15;
			
			calcDat1T += 800;
			gru2WorkData.Hptr += 800;
		}
		
		memcpy(calcDat2, gru2WorkData.Hptr, 600 * sizeof(float));

#if EXTRA_TIMING == 1		
		LONGLONG gru2Time = 0;
		Win32PerformanceGetCount(&gru2Time);
#endif		
		
		threadParametersT mat2WorkData;
		mat2WorkData.iterations = GRU2_HIDDEN_SIZE;
		
		calcDat1T = calcDat1 + 800;
		calcDat2T = calcDat2 + 800;
		
		mat2WorkData.Mptr = (float*) dataMAT2;
		
		for (int i=0; i<20; i++) {
			mat2WorkData.Xptr = calcDat2T;
			mat2WorkData.Yptr = calcDat1T;
			
			for (int c=0; c<conversions; c++) {
				computeMAT5Max0asm(mat2WorkData.iterations, mat2WorkData.Mptr, mat2WorkData.Xptr, mat2WorkData.Yptr);
				
				mat2WorkData.Xptr += 800;
				mat2WorkData.Yptr += 800;
			}
			
			mat2WorkData.Mptr += 24040;
			calcDat1T += (5*8);
		}

#if EXTRA_TIMING == 1		
		LONGLONG mul2Time = 0;
		Win32PerformanceGetCount(&mul2Time);
#endif		
		
		threadParametersT mat3WorkData;
		mat3WorkData.iterations = MATRIX_MULT2_SIZE;
		
		calcDat1T = calcDat1 + 800;
		calcDat2T = calcDat2 + 800;
		
		mat3WorkData.Mptr = (float*) dataMAT3;
		
		for (int i=0; i<20; i++) {
			mat3WorkData.Xptr = calcDat1T;
			mat3WorkData.Yptr = calcDat2T;
			
			for (int c=0; c<conversions; c++) {
				computeMAT5Max0asm(mat3WorkData.iterations, mat3WorkData.Mptr, mat3WorkData.Xptr, mat3WorkData.Yptr);
				
				mat3WorkData.Xptr += 800;
				mat3WorkData.Yptr += 800;
			}
			
			mat3WorkData.Mptr += 32040;
			calcDat2T += (5*8);
		}

#if EXTRA_TIMING == 1		
		LONGLONG mul3Time = 0;
		Win32PerformanceGetCount(&mul3Time);
#endif
		
		threadParametersT mat4WorkData;
		mat4WorkData.iterations = MATRIX_MULT3_SIZE; //800
		
		calcDat1T = calcDat1 + 800;
		calcDat2T = calcDat2 + 800;
		
		mat4WorkData.Mptr = (float*) dataMAT4;
		
		for (int i=0; i<8; i++) {
			mat4WorkData.Xptr = calcDat2T;
			mat4WorkData.Yptr = calcDat1T;
			
			for (int c=0; c<conversions; c++) {
				computeMAT8NoMaxasm(mat4WorkData.iterations, mat4WorkData.Mptr, mat4WorkData.Xptr, mat4WorkData.Yptr);
				
				mat4WorkData.Xptr += 800;
				mat4WorkData.Yptr += 800;
			}
			
			mat4WorkData.Mptr += 51264;
			calcDat1T += (8*8);
		}
		
		mat4WorkData.Xptr = calcDat2T;
		mat4WorkData.Yptr = calcDat1T;
		
		for (int c=0; c<conversions; c++) {
			computeMAT1asm(mat4WorkData.Mptr, mat4WorkData.Xptr, 25, mat4WorkData.Yptr);
			
			mat4WorkData.Xptr += 800;
			mat4WorkData.Yptr += 800;
		}

#if EXTRA_TIMING == 1		
		LONGLONG mul4Time = 0;
		Win32PerformanceGetCount(&mul4Time);
#endif
		
		fftDataT = fftDataM;
		calcDat1T = calcDat1 + 800;
		sigmoidClampMultiply(calcDat1T, fftDataT, expConstants, conversions);
		
		fftDataT = fftDataM;
		for (int c=0; c<conversions; c++) {
			fftwf_execute_split_dft_c2r(fftInvPlan, fftDataT, fftDataT + 520, windowDataInv);
			
			windowDataToOutData(windowDataInv, adjHannWindowInv, windowDataAccum, outData);
			WriteFile(outFileHandle, outData, conversionBytes, NULL, NULL);
			
			fftDataT += 1040;
		}		

#if EXTRA_TIMING == 1		
		LONGLONG stopTime = 0;
		Win32PerformanceGetCount(&stopTime);
		
		Win32PerformanceIncDifference(startTime, stopTime, &totalTime);
		Win32PerformanceIncDifference(startTime, fftAbsTime, &part1Time);
		//part1Time += (readTime-startTime);
		Win32PerformanceIncDifference(fftAbsTime, mul1Time, &part2Time);
		Win32PerformanceIncDifference(mul1Time, gru2Time, &part3Time);
		Win32PerformanceIncDifference(mul2Time, mul4Time, &part4Time);
		Win32PerformanceIncDifference(mul4Time, stopTime, &part5Time);
		
		//Win32PerformanceIncDifference(mul1Time, test1Time, &testTime);
#endif
	}
	
	size_t remainingData = dataSize % conversionBytes;
	memset(outData, 0, remainingData);
	WriteFile(outFileHandle, outData, remainingData, NULL, NULL);
	
	LONGLONG conversionTime2 = 0;
	Win32PerformanceGetCount(&conversionTime2);
	LONGLONG conversionTimeMs = (conversionTime2 - conversionTime1) / (1000 * win32PerformanceCountDivider);
	printf("Finished Conversion in %lldms\n", conversionTimeMs);
	
#if EXTRA_TIMING == 1	
	printf("Conversion Average Itera Time: %lldus\n", totalTime / numConversions);
	printf("Conversion Average Part1 Time: %lldus\n", part1Time / numConversions);
	printf("Conversion Average Part2 Time: %lldus\n", part2Time / numConversions);
	printf("Conversion Average Part3 Time: %lldus\n", part3Time / numConversions);
	printf("Conversion Average Part4 Time: %lldus\n", part4Time / numConversions);
	printf("Conversion Average Part5 Time: %lldus\n", part5Time / numConversions);
	printf("Conversion TEST Time: %lldus\n", testTime / (numConversions * win32PerformanceCountDivider));
#endif		
	
	//Destroy fft plans
	fftwf_destroy_plan(fftInvPlan);
	fftwf_destroy_plan(fftPlan);
	
	//Free Large Allocated Memory
	//*
	_aligned_free(dataIO);
	
	_aligned_free(fftDataM);
	_aligned_free(calcDat1);
	_aligned_free(calcDat2);
	
	_aligned_free(ztData);
	_aligned_free(rtData);
	_aligned_free(htData);
	
	
	//*/
	
	return 0;
}

//Main C Entry Function
int main(int argc, char* argv[]) {
	int error = 0;
	// get console output handle to be used with WriteConsole which would replace printf... in the future (strHelper)
	//HANDLE conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	printf("\nNSNet2 Offline Audio File Conversion Program Started\n");
	
	
	// Setup the performance counter and get an initial reading
	Win32PerformanceInitialize();
	LONGLONG startTime = 0;
	Win32PerformanceGetCount(&startTime);
	
	
	TCHAR directoryBuffer[200];
	DWORD dirRes = GetCurrentDirectory(200, directoryBuffer);
	printf("Current Directory: %ls\n", directoryBuffer);
	
	
	HANDLE dataFileHandle = CreateFile(DATA_FILE_LOCATION, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dataFileHandle == NULL) {
		printf("Problem opening input data file\n");
		return 1;
	}
	
	error = readDataFileIntoMemory(dataFileHandle);
	if (error != 0) {
		CloseHandle(dataFileHandle);
		return 2;
	}
	
	LONGLONG fileReadEndTime = 0;
	Win32PerformanceGetCount(&fileReadEndTime);
	LONGLONG fileReadDiffTime = 0;
	Win32PerformanceGetDifference(startTime, fileReadEndTime, &fileReadDiffTime);
	printf("Network Data Load-In Time: %lldus\n\n", fileReadDiffTime);
	
	CloseHandle(dataFileHandle);
	
	
	TCHAR fileNameStr[200] = INPUT_FILE_LOCATION;
	
	printf("Select an input audio wave (.wav) file\n");
	OPENFILENAME openFileInfo = {0};
	openFileInfo.lStructSize = sizeof(OPENFILENAME);
	openFileInfo.hwndOwner = NULL;
	//openFileInfo.hInstance = NULL;
	openFileInfo.lpstrFilter = NULL; //String File Name Filter
	openFileInfo.lpstrCustomFilter = NULL; //Not Needed
	openFileInfo.nMaxCustFilter = 0; //Ignored when lpstrCustomFilter is NULL
	openFileInfo.nFilterIndex = 0;
	openFileInfo.lpstrFile = fileNameStr;
	openFileInfo.nMaxFile = 200;
	openFileInfo.lpstrFileTitle = NULL;
	openFileInfo.nMaxFileTitle = 0;
	openFileInfo.lpstrInitialDir = NULL;
	openFileInfo.lpstrTitle = L"Open Wav To Convert";
	openFileInfo.Flags = 0; //Might need to look through these
	//openFileInfo.nFileOffset = 0;
	//openFileInfo.nFileExtension = 0;
	//openFileInfo.lpstrDefExt = 0;
	//More parameters...
	
	BOOL openFileResult = 1;
	//openFileResult = GetOpenFileName(&openFileInfo);
	if (openFileResult == 0) {
		printf("Conversion Canceled\n");
		return 3;
	}
	
	LONGLONG fileChosenTime = 0;
	Win32PerformanceGetCount(&fileChosenTime);
	
	//Open Input and Output Audio Files (Synchronous Operations For Now)
	HANDLE inFileHandle = CreateFile(fileNameStr, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (inFileHandle == NULL) {
		printf("Problem Opening Input File\n");
		return 4;
	}
	
	printf("%ls -> ", fileNameStr);
	
	fileNameStr[openFileInfo.nFileExtension-1] = 0;
	_tcscat_s(fileNameStr, 200, OUTPUT_FILE_APPEND);
	
	printf("%ls\n\n", fileNameStr);
	
	HANDLE outFileHandle = CreateFile(fileNameStr, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
	if (outFileHandle == NULL) {
		printf("Problem Creating Output File\n");
		return 5;
	}
	
	error = readInputAudioAndCreateOutput(inFileHandle, outFileHandle);
	if (error == 0) {
		error = performConversion(inFileHandle, outFileHandle);
		if (error != 0) {
			printf("Error during conversion!\n");
		}
	}
	
	CloseHandle(outFileHandle);
	CloseHandle(inFileHandle);
	
	_aligned_free(dataMAT1);
	_aligned_free(dataGRU1);
	_aligned_free(dataGRU2);
	_aligned_free(dataMAT2);
	_aligned_free(dataMAT3);
	_aligned_free(dataMAT4);
	
	// Get the final reading of the performance counter and display total run time (in us)
	LONGLONG stopTime = 0;
	Win32PerformanceGetCount(&stopTime);
	LONGLONG diffTime = 0;
	Win32PerformanceGetDifference(startTime, stopTime, &diffTime);
	printf("\nTotal Program Run Time: %lldms\n", diffTime / 1000);
	
	//Exit and print error if necessary
	if (error != 0) {
		printf("Program Ended with Error: 0x%X\n\n", error);
		return error;
	}
	printf("Program Ended Successfully\n\n");
	return 0;
}

