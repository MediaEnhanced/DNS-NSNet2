# Deep Noise Suppression - NSNet2
&emsp;&emsp;NSNet2 is a deep learning artificial recurrent neural network (RNN) used for "background" noise reduction in speech audio files. Microsoft originally released NSNet2 as an updated baseline for their annual Deep Noise Suppression (DNS) [challenge](https://github.com/microsoft/DNS-Challenge/tree/master/NSNet2-baseline), but it is inconvient to use and is not suited for real-time conversion. The correct versions of Python and supporting libarires (including PyTorch and ONNXruntime) needed to be installed and properly linked for functional use and when NSNet2 is run without any changes, massive amounts of memory (RAM) are allocated when uneccessary. Currently there are not many wide-range effective open noise suppressors that can be used easily, projects like [RNNoise](https://github.com/xiph/rnnoise) have their quirks (though can be used more easily within the ffmpeg project), but NSNet2 is an effective "next-step-up!" NSNet2 just needed to be converted to a version more people looking to fine-tune captured speech could utilize which is the major focus of this project.

## What Are the Project Goals? (Features)
1. Noise Reduction of Speech Audio
2. Usable, Fast, and Simple Versions of NSNet2
3. Input wave (.wav) audio file -> Output equivalent audio file where the raw audio data has been noise suppressed
4. Real-Time (Live) and Offline (Non-Live) Versions
5. Low RAM Usage for the same processing speed as the original NSNet2 (for Offline Version)
6. Utilize Single Instruction Multiple Data Instructions (AVX2 and FMA) of modern x64 CPUs
7. Simple to Modify and Compile
8. Thorough Explanation (with diagrams) of how NSNet2 Works towards creating Effective Noise Reduction
9. Thorough Explanation (with diagrams) of what the code is doing and why it was written that way
10. No reliance on math or general matrix calculation libraries for any repeating calculations

## How To Use It
### Very Basic Conversion using Command Prompt (Windows Powershell) on Windows
1. Convert a video or audio file to a WAVE (.wav) audio file using [ffmpeg](https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z)
```
ffmpeg.exe -i inputFile -vn -ac 1 output.wav
```
2. Run (Double-click) the latest NSNet2Offline.exe executable downloaded from the releases page of this project
3. Navigate to and select the output.wav audio file created in step 1
4. The converter will create output-Enhanced.wav in the same directory that output.wav resides
5. Listen and compare the resulting noise suppressed file to the original

## Current Limitations; Version 0.1
1. Works only with 48kHz 1-Channel (Mono) Wave Audio Files
2. Offline Version Released Only
3. Does not convert 2-Channel (Stereo) Audio
4. Real-time version is RAM memory bound (operates on ~23.5MB of data for every 10ms of converted audio)
5. Too basic and minimal WAVE file error checking
6. Only works on Windows OS (Tested with fresh install of Windows 10)
7. Requires a newish x64 CPU with AVX2 and FMA support
8. TO ADD

## Planned Features
1. Live Version
2. 2-Channel Convert; Each channel seperately and mixed stereo to mono then convert
3. Multithreading Capability
4. Adjustable RAM usuage (might increase offline processing speed by a tiny amount)
5. More Code documentation
6. Linux and FreeBSD support
7. TO ADD

## How to Compile It
### Setup and Necessary Libraries
All C code is currently written to be compiled with gcc for Windows using the MinGW-w64 software. Latest versions can be found [here](https://winlibs.com/). The C code will be modified in the future to be compiled with gcc no-matter the operating system.

All x64 Assembly code (currently containing only subroutine functions) were written to be assembled by the [flat assembler](https://flatassembler.net/download.php) (FASM). The assembly code contains the functions that do the main processing and make AVX2 and FMA calls. The assembled object files get linked into the final executable by gcc / ld

The [FFTW](https://www.fftw.org/) library is used for performing the Discrete Fourier Transform (DFT) and its inverse. The single-percision floating point static library version (for Windows) needs to be compiled and will get linked into the final executable by gcc / ld. In the current source of FFTW, [CMAKE](https://cmake.org/) can be used with MinGW-w64 on Windows to create the static libray after a couple of modifications. The memory allocation file needs to be modified before utilizing the CMAKE script and the script needs to specify the following options.

### Compiling the Assembly
FASM makes it easy to assemble the x64 assembly code simply by running:
```
fasm.exe asmFile.asm
```
This will create an asmFile.o file in the same directory that can be used with gcc

### Creating the Hann Window Object File
TO ADD

### Compile the main C source files (including main.c) and linking in the other object files
TO ADD

## What Should Be Modified First?
TO ADD

## Other Projects for the Future
1. How to (re)-train the neural network data from the DNS Challenge set
2. Optimized version of RNNoise project with prinicples taken from this project
3. New noise reduction project utilizing neural networks and ideas taken from other projects and published works
4. TO ADD
