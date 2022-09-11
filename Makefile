
default_target: WindowsOfflineExecutable

./bin/obj/main.o: ./src/main.c ./src/Win32Performance.h ./src/fftw3.h
	gcc -Wall -m64 -mconsole -O2 -std=c11 -c -o ./bin/obj/main.o ./src/main.c

./bin/obj/asmFunctions.o: ./src/asmFunctions.asm
	FASM ./src/asmFunctions.asm ./bin/obj/asmFunctions.o

./bin/NSNet2Offline.exe: ./bin/obj/main.o ./bin/obj/asmFunctions.o ./bin/obj/hannWindow.o
	gcc -Wall -m64 -mconsole -O2 -s -o ./bin/NSNet2Offline.exe ./bin/obj/main.o -lcomdlg32 ./bin/obj/asmFunctions.o ./bin/obj/hannWindow.o -L./bin/lib -lfftw3f

WindowsOfflineExecutable: ./bin/NSNet2Offline.exe

clean:
	rm ./bin/obj/main.o
	rm ./bin/obj/asmFunctions.o
	rm ./bin/NSNet2Offline.exe
