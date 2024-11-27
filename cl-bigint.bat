set BINPATH=D:\Program Files\MinGW64\bin

path=%BINPATH%;%path%

del .\BigInt.exe

g++ -c BigInt.cpp -std=c++17

g++.exe BigInt.o -o .\BigInt.exe -m64


del .\BigInt.o

pause