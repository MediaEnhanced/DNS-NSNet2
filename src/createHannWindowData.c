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


//Helper program that creates the data of a Hann Window and exports it to a file
//This data file gets converted to an object file by the objcopy program
//gcc will link this in with the final program
//The hann window will not need to be re-generated on each start-up of the main program


#include <stdint.h>	//Defines Data Types: https://en.wikipedia.org/wiki/C_data_types
#include <stdlib.h>	//Needed for dynamic memory operations malloc & free
#include <stdio.h>	//Needed for printf statements and general file operations

#define _USE_MATH_DEFINES	//Needed for pi constant: M_PI
#include <math.h> //Needed for math sine, cosine, and square root functions: sin, cos, sqrt

#define WINDOW_SIZE 960 //Window Size 

int main(int argc, char* argv[]) {
  int error = 0;
	printf("\nCreate Hann Window Data Program Started\n");
	
	printf("Creating Hann window data...\n");
	size_t windowBytes = WINDOW_SIZE * sizeof(float);
	float* window = malloc(windowBytes); //allocate memory array for data
	
	
	//Process in double-precision (double) and then convert to single precision (float)
	//double pi2 = M_PI * 2.0;
	//double cosFactor = pi2 / ((double) WINDOW_SIZE);
	//double halfFactor = 0.5;
	
	for (uint64_t n=0; n<WINDOW_SIZE; n++) {
		//double cosResult = cos(n * cosFactor);
		//double result = sqrt(halfFactor - (cosResult * halfFactor));
		double result = sin((M_PI * n) / WINDOW_SIZE);
		window[n] = (float) result;
	}
	
	printf("Hann Window data created!\n");
	
	
	printf("Checking invertability of half overlapped windows...\n");
	uint64_t halfWindow = WINDOW_SIZE >> 1; //WINDOW_SIZE / 2 (truncated)
	
	float avgRes = 0.0;
	float minRes = 100000.0;
	float maxRes = 0.000001;
	
	for (uint64_t i=0; i<halfWindow; i++) {
		float part1 = window[i];
		float part2 = window[i + halfWindow];
		float result = part1*part1 + part2*part2;
		avgRes += result;
		if (result < minRes) {
			minRes = result;
		}
		else if (result > maxRes) {
			maxRes = result;
		}
	}
	
	printf("Average Result: %.8f\n", avgRes / halfWindow);
	printf("Minimum Result: %.8f\n", minRes);
	printf("Maximum Result: %.8f\n", maxRes);
	
	float tolerance = 0.00001;
	int invertability = 0;
	if (minRes < 1.0) {
		if ((1.0 - minRes) > tolerance) {
			invertability++;
		}
	}
	else {
		if ((minRes - 1.0) > tolerance) {
			invertability++;
		}
	}
	if (maxRes > 1.0) {
		if ((maxRes - 1.0) > tolerance) {
			invertability++;
		}
	}
	else {
		if ((1.0 - maxRes) > tolerance) {
			invertability++;
		}
	}
	
	if (invertability > 0) {
		printf("Invertability of overlapped windows not correct!\n");
		error = 1;
	}
	
	
	printf("Exporting Hann Window data to file...\n");
	char* fileLocation = "hannWindow.data"; //Default location
	if (argc >= 2) {
		fileLocation = argv[1];
	}
	
	FILE* dataFile = fopen(fileLocation, "wb");
	if (dataFile == NULL) {
		printf("File could not be opened, or overwritten!\n");
		error = -1;
	}
	else {
		size_t elementsWritten = fwrite(window, sizeof(float), WINDOW_SIZE, dataFile);
		if (elementsWritten != WINDOW_SIZE) {
			printf("Data could not be completely written to file!\n");
			error = -2;
		}
		fclose(dataFile);
		printf("Hann Window data saved to file: %s\n", fileLocation);
	}
	
	
	free(window);
	
	printf("Program Ended\n");
  return error;
}