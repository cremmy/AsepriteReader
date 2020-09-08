#include <cstdio>

#include "asepritereader.h"

int main(int, char**)
{
	AsepriteReader reader;
	
	try
	{
		reader.load("test.aseprite");
	}
	catch(...)
	{
		printf("Fail\n");
		return 1;
	}
	
	printf("Tags:\n");
	for(auto& tag: reader.file.tags)
	{
		printf(" - %s\n", tag->name.c_str());
	}
	
	printf("Layers:\n");
	for(auto& layer: reader.file.layers)
	{
		printf(" - %s\n", layer->name.c_str());
	}
	
	printf("Success\n");
	return 0;
};