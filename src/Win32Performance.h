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

//Include Header file for helper functions to use the Windows Performance Counter to Get Accurate Code Timings
#ifndef WIN32_PERFORMANCE_H
#define WIN32_PERFORMANCE_H

#include <windows.h>

//#define WIN32_PERFORMANCE_ERROR
//#define WIN32_PERFORMANCE_PRINT

#ifdef WIN32_PERFORMANCE_PRINT
#include <stdio.h>
#endif

#define WIN32_PERFORMANCE_TARGET_FREQUENCY 1000000 //To be used to convert results to microseconds (1us = 1e6 Hz)

static LONGLONG win32PerformanceFrequency = 0;
static LONGLONG win32PerformanceCountDivider = 0;

static int Win32PerformanceInitialize() {
	LARGE_INTEGER performanceCounter;
	BOOL result = QueryPerformanceFrequency(&performanceCounter);
	if (result == 0) {
		win32PerformanceFrequency = 0;
		#ifdef WIN32_PERFORMANCE_PRINT
		printf("No high-resolution performance counter supported\n");
		#endif
		return 1;
	}
	win32PerformanceFrequency = performanceCounter.QuadPart;
	
	if (win32PerformanceFrequency < WIN32_PERFORMANCE_TARGET_FREQUENCY) {
		win32PerformanceCountDivider = 0;
		#ifdef WIN32_PERFORMANCE_PRINT
		printf("Performance Counter does not meet target measurement frequency!\n");
		#endif
		return 2;
	}
	win32PerformanceCountDivider = win32PerformanceFrequency / WIN32_PERFORMANCE_TARGET_FREQUENCY;
	
	if ((win32PerformanceFrequency % WIN32_PERFORMANCE_TARGET_FREQUENCY) != 0) {
		#ifdef WIN32_PERFORMANCE_PRINT
		printf("Performance Frequency does not divide evenly for target frequency\n");
		#endif
		return 3;
	}
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Counter Properly Initialized\n");
	#endif
	return 0;
}

#ifdef WIN32_PERFORMANCE_ERROR
static int Win32PerformanceGetCount(LONGLONG* count) {
	if (win32PerformanceFrequency == 0) {
		return 4;
	}
	LARGE_INTEGER performanceCounter;
	QueryPerformanceCounter(&performanceCounter);
	*count = performanceCounter.QuadPart;
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Count: %lld\n", *count);
	#endif
	return 0;
}
#else
inline static void Win32PerformanceGetCount(LONGLONG* count) {
	LARGE_INTEGER performanceCounter;
	QueryPerformanceCounter(&performanceCounter);
	*count = performanceCounter.QuadPart;
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Count: %lld\n", *count);
	#endif
}
#endif

#ifdef WIN32_PERFORMANCE_ERROR
static int Win32PerformanceGetDifference(LONGLONG start, LONGLONG stop, LONGLONG* difference) {
	if (win32PerformanceCountDivider == 0) {
		return 5;
	}
	*difference = (stop - start) / win32PerformanceCountDivider; //Truncates difference (always rounds down)
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Difference: %lldus\n", *difference);
	#endif
	return 0;
}
#else
inline static void Win32PerformanceGetDifference(LONGLONG start, LONGLONG stop, LONGLONG* difference) {
	*difference = (stop - start) / win32PerformanceCountDivider; //Truncates difference (always rounds down)
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Difference: %lldus\n", *difference);
	#endif
}
#endif

#ifdef WIN32_PERFORMANCE_ERROR
static int Win32PerformanceIncDifference(LONGLONG start, LONGLONG stop, LONGLONG* difference) {
	if (win32PerformanceCountDivider == 0) {
		return 5;
	}
	*difference += (stop - start) / win32PerformanceCountDivider; //Truncates difference (always rounds down)
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Inc Difference: %lldus\n", *difference);
	#endif
	return 0;
}
#else
inline static void Win32PerformanceIncDifference(LONGLONG start, LONGLONG stop, LONGLONG* difference) {
	*difference += (stop - start) / win32PerformanceCountDivider; //Truncates difference (always rounds down)
	#ifdef WIN32_PERFORMANCE_PRINT
	printf("Performance Inc Difference: %lldus\n", *difference);
	#endif
}
#endif

#endif
