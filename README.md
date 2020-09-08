# AsepriteReader
Reader of .aseprite files written in plain C++, only dependent on zlib.

Created with the help of https://github.com/WeAthFoLD/MetaSprite

Supports everything I needed for my project d;

# How to use
Build asepritereader.cpp file into a library, link to your project (don't forget to also add zlib)

```
g++ -c asepritereader.cpp -o asepritereader.o
ar rcs libasepritereader.a asepritereader.o
```

OR

Add asepritereader.h and .cpp to your project.

test.cpp contains a simple test and an example on how to use the read file.

# zlib
Reader depends on zlib https://zlib.net/ . If you wan't to use anything else, or don't use it at all, find and change/remove a call to "uncompress".
