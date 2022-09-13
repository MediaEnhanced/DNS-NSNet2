
default_target: WindowsOfflineExecutable

./bin/obj/main.o: ./src/main.c ./src/Win32Performance.h ./src/fftw3.h
	gcc -Wall -m64 -mconsole -O2 -std=c11 -c -o ./bin/obj/main.o ./src/main.c

./bin/obj/asmFunctions.o: ./src/asmFunctions.asm
	FASM ./src/asmFunctions.asm ./bin/obj/asmFunctions.o

./bin/CreateHannWindowData.exe: ./src/createHannWindowData.c
	gcc -Wall -m64 -mconsole -O2 -std=c11 -s -o ./bin/CreateHannWindowData.exe ./src/createHannWindowData.c

./bin/obj/hannWindow.data: ./bin/CreateHannWindowData.exe
	./bin/CreateHannWindowData.exe ./bin/obj/hannWindow.data

./bin/obj/hannWindow.o: ./bin/obj/hannWindow.data
	cd ./bin/obj/ &&	objcopy -I binary -O elf64-x86-64 ./hannWindow.data ./hannWindow.o

./bin/NSNet2Offline.exe: ./bin/obj/main.o ./bin/obj/asmFunctions.o ./bin/obj/hannWindow.o
	gcc -Wall -m64 -mconsole -O2 -s -o ./bin/NSNet2Offline.exe ./bin/obj/main.o -lcomdlg32 ./bin/obj/asmFunctions.o ./bin/obj/hannWindow.o -L./bin/lib -lfftw3f

WindowsOfflineExecutable: ./bin/NSNet2Offline.exe

clean:
	rm ./bin/obj/main.o
	rm ./bin/obj/asmFunctions.o
	rm ./bin/obj/hannWindow.data
	rm ./bin/obj/hannWindow.o
	rm ./bin/CreateHannWindowData.exe
	rm ./bin/NSNet2Offline.exe
