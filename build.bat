g++ -c -O2 -std=c++17 asepritereader.cpp -o asepritereader.o
ar rcs libasepritereader.a asepritereader.o
g++ test.cpp -L . -l asepritereader -l z -o test.exe