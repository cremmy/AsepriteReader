#include <cstdio>

#include "asepritereader.h"

int main(int, char**)
	{
	Aseprite::AsepriteFilePtr file;
	
	try
		{
		// If you don't need manager...
		//file=Aseprite::AsepriteReader::getInstance()->load("test.aseprite");
		
		// When you want to use single file multiple times, but passing it around is not possible, use manager:
		auto manager=Aseprite::AsepriteReader::getInstance();
		file=manager->load("test.aseprite");
		}
	catch(...)
		{
		printf("Fail\n");
		return 1;
		}
	
	printf("Tags:\n");
	for(auto& tag: file->tags)
		{
		printf(" - %s\n", tag->name.c_str());
		}
	
	printf("Layers:\n");
	for(auto& layer: file->layers)
		{
		printf(" - %s\n", layer->name.c_str());
		}
	
	printf("Success\n");
	return 0;
};