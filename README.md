# Deep Noise Suppression - NSNet2
&emsp;&emsp;NSNet2 is a deep learning artificial recurrent neural network (RNN) used for background noise reduction in speech audio files. Microsoft originally released NSNet2 as an updated comparision baseline for their annual Deep Noise Suppression (DNS) [challenge](https://github.com/microsoft/DNS-Challenge/tree/master/NSNet2-baseline), but it is inconvient to use and is not suited for real-time processing. Use of the released NSNet2 model requires correct versions of Python and supporting libarires (including PyTorch and [ONNXruntime](https://github.com/microsoft/onnxruntime)) to be installed and properly linked together. These are relatively big software packages to install and configure just to run this one neural network model. Running NSNet2 without any changes also uses massive amounts of uneccessary memory (RAM) that scales with the size of the audio file.

&emsp;&emsp;Currently there are not many wide-range, pre-trained, and effective noise suppressors for speech that can be used easily. Projects like [RNNoise](https://github.com/xiph/rnnoise) have several quirks, but NSNet2 can be the next-step-up for fine-tuning captured speech. NSNet2 just needed to be converted to a version that more people looking for additional audio filters for recorded speech could utilize, which is the major focus of this project. This nameless project is a user friendly conversion of the NSNet2 released by Microsoft Research.
  
&nbsp;

## What Are the Project Goals? (Features)
* Noise Reduction of Speech Audio
* Usable, Fast, and Straightforward versions of NSNet2
* Input wave (.wav) audio file -> Output equivalent noise suppressed audio file
* Offline | Non-Live and Real-time | Live versions
* Low RAM Usage while maintaining the same processing speed as NSNet2 (Offline version)
* Utilize Single-Instruction-Multiple-Data Instructions (AVX2 and FMA) of modern x64 CPUs
* Thorough Explanation (with diagrams) of how NSNet2 performs effective Noise Reduction
* Thorough Explanation (with diagrams) of what the code is doing and why it was written that way
* Simple to Compile and Modify
* No reliance on math or general matrix calculation libraries for any repeating calculations
  
&nbsp;

## Slightly More Background
&emsp;&emsp;This project was created as a starting point into creating open audio noise reduction software. Audio noise suppression research (with and without using neural networks) produces various publications and snippets throughout the web but rarely leads to open (and pre-trained) usable software. Deep learning models get compared in the Microsoft DNS-challenge and while some of the model designs are published (usually with only minimal information) the exact implementations and trained model parameter values are kept private. However, these light publications sometimes give enough information that the network model can be mostly recreated or be useful to synthesize hybrid designs. Models can then be trained with customizeable training file sets (like the one from the DNS-challenge). The final results could then be run through a comparison process against each other and possibly against the DNS-challenge results.

&emsp;&emsp;Since NSNet2 was published with the exact implementation and trained values, converting the model to a more user friendly version was straightforward. The model was published in the open ONNX format, which meant testing if the ONNX runtime software could be used as the main and biggest dependency alongside compileable code. Unfortunately the current ONNX runtime software suffers from the innability to carry over the model's Gated Recurrent Unit (GRU) hidden states from a previous run which is the biggest reason it is unsuitable for real-time versions of NSNet2. The model value data was extracted (and reorganized) from the ONNX file to be used with the converted version of the model.

TO ADD (software / code principles and links to detailed explanation site with examples)
  
&nbsp;

## How To Use It

### Basic Conversion using a Modern Windows x64 Computer
1. Convert a video or audio file to a WAVE (.wav) audio file using [ffmpeg](https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z) and the Command Prompt (or Windows Powershell)  
```ffmpeg.exe -i inputFile -vn -ac 1 output.wav```  
where "-vn" removes any video element and "-ac 1" mixes the audio into one channel (mono)
2. Run (Double-click) the latest [NSNet2Offline.exe](https://github.com/MediaEnhanced/DNS-NSNet2/releases/download/v0.1/NSNet2Offline.zip) executable downloaded from the releases page of this project
3. Navigate to and select the "output.wav" audio file created in step 1
4. The converter will create "output-Enhanced.wav" in the same directory that "output.wav" resides
This process should take about 2-8 seconds for every 60 seconds of audio
5. Listen and compare the resulting noise suppressed file to the original

### Comparison against RNNoise
TO ADD (use ffmpeg)

### Audio Files with "Louder" but Consistent Background Noise
TO ADD (pre-process with Audacity Noise Reduction Effect)
  
&nbsp;

## Current Limitations; Version 0.1
1. Works only with 48kHz 1-Channel (Mono) Wave Audio Files
2. Offline Version Released Only
3. Does not convert 2-Channel (Stereo) Audio
4. Real-time version is RAM memory bound (reads ~23.5MB of data for every 10ms of converted audio)
5. Too basic and minimal WAVE file error checking
6. Only works on Windows OS (Tested with fresh install of Windows 10)
7. Requires a newish x64 CPU with AVX2 and FMA support
8. TO ADD
  
&nbsp;

## Planned Features
1. Live Version
2. 2-Channel Convert; Each channel seperately and mixed stereo to mono then convert
3. Multithreading Capability for Offline Version
4. Adjustable RAM usuage (might increase offline processing speed by a tiny amount)
5. More Code documentation
6. Linux and FreeBSD support
7. TO ADD
  
&nbsp;

## How to Compile It (with Windows)

### Setup and Necessary Libraries
All C code is currently written to be compiled with gcc for Windows using the MinGW-w64 software. Latest versions can be found [here](https://winlibs.com/). The C code will be modified in the future to be compiled with gcc no-matter the operating system.

All x64 Assembly code (currently containing only subroutine functions) were written to be assembled by the [flat assembler](https://flatassembler.net/download.php) (FASM). The assembly code contains the functions that do the main processing and make AVX2 and FMA calls. The assembled object files get linked into the final executable by gcc / ld

The [FFTW](https://www.fftw.org/) library is used for performing the Discrete Fourier Transform (DFT) and its inverse. The single-percision floating point static library version (for Windows) needs to be compiled and will get linked into the final executable by gcc / ld. In the current source of FFTW, [CMAKE](https://cmake.org/) can be used with MinGW-w64 on Windows to create the static libray after a couple of modifications. The memory allocation file needs to be modified before utilizing the CMAKE script and the script needs to specify the following options: (TO ADD)

### Using Make
The Makefile can be used to create the executables found on the release page. MinGW-w64 comes with Make that can process the Makefile to compile the source code once MinGW binaries and FASM binaries are added to the path. Using the Command Prompt (or Windows Powershell) change directory into the root folder of this project and run: ```mingw32-make.exe```

The resulting executable and necessary networkData.bin file can be found in the bin subdirectory. The Makefile directs Make to use both gcc and FASM to create the intermediate object files from the source code which then get linked together with the fftw libaray in the executable by gcc / ld
  
&nbsp;

## What Can Be Modified?
TO ADD
  
&nbsp;

## Other Projects for the Future
1. How to (re)-train the neural network data from the DNS Challenge set
2. Optimized version of RNNoise project with prinicples taken from this project
3. New noise reduction project utilizing neural networks and ideas taken from other projects and published works
4. TO ADD
  
&nbsp;

## How to Reach The Developer
Email me: Jared.Loewenthal@proton.me
