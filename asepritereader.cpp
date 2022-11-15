/*
 * asepritereader.cpp
 *
 *  Created on: 21 apr 2020
 *      Author: crm
 */

#include "asepritereader.h"

#include <fstream>
#include <map>
#include <sstream>
#include <zlib.h>


using namespace Aseprite;

// Additional constants, internal to Aseprite's file format
namespace AsepriteFileConstants
	{
	const uint16_t ASEPRITE_MAGIC_NUMBER_FILE=0xA5E0;
	const uint16_t ASEPRITE_MAGIC_NUMBER_FRAME=0xF1FA;

	enum ChunkType
		{
		CHUNK_LAYER=0x2004,
		CHUNK_CEL=0x2005,
		CHUNK_CELEXTRA=0x2006,
		CHUNK_FRAME_TAGS=0x2018,
		CHUNK_PALETTE=0x2019,
		CHUNK_USERDATA=0x2020,
		CHUNK_SLICE=0x2022,
		};

	enum LayerType
		{
		LAYER_NORMAL=0,
		LAYER_GROUP=1,
		};

	enum Flag
		{
		FLAG_LAYER_VISIBLE=0x1<<0,

		FLAG_USERDATA_TEXT=0x1<<0,
		FLAG_USERDATA_COLOR=0x1<<1,

		FLAG_SLICE_9SLICES=0x1<<0,
		FLAG_SLICE_PIVOT=0x1<<1,
		};

	enum CelType
		{
		CEL_RAW=0,
		CEL_LINKED=1,
		CEL_COMPRESSED=2,
		};
	}


AsepriteFilePtr AsepriteReader::load(const std::string& path)
	{
	if(auto it=resources.find(path); it!=resources.end())
		{
		//if(!it->second.expired())
			return it->second;// .lock();
		}

	auto ptr=loadInternal(path);

	resources[path]=ptr;

	return ptr;
	}

AsepriteFilePtr AsepriteReader::loadInternal(const std::string& path)
	{
	using namespace Aseprite;
	using namespace AsepriteFileConstants;

	/*****************************************************************************/
	/* Helpers *******************************************************************/
	/*****************************************************************************/
	auto readUInt8=[&path](std::fstream& in)->uint8_t
		{
		uint8_t tmp;
		in.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
		if(!in.good())
			throw ResourceLoadException("Unexpected end of file");
		return tmp;
		};
	auto readInt16=[&path](std::fstream& in)->int16_t
		{
		int16_t tmp;
		in.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
		if(!in.good())
			throw ResourceLoadException("Unexpected end of file");
		return tmp;
		};
	auto readUInt16=[&path](std::fstream& in)->uint16_t
		{
		uint16_t tmp;
		in.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
		if(!in.good())
			throw ResourceLoadException("Unexpected end of file");
		return tmp;
		};
	auto readUInt32=[&path](std::fstream& in)->uint32_t
		{
		uint32_t tmp;
		in.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
		if(!in.good())
			throw ResourceLoadException("Unexpected end of file");
		return tmp;
		};
	auto readString=[&path](std::fstream& in, unsigned short length)->std::string
		{
		std::string buff;
		buff.resize(length); // Resize adds \0 at the end
		in.read(&buff[0], length);
		return buff;
		};
	auto readBytes=[&path](std::fstream& in, unsigned short length)->uint8_t*
		{
		uint8_t* data=new uint8_t[length+1];
		in.read(reinterpret_cast<char*>(data), length);
		data[length]=0;
		return data;
		};
	auto skipBytes=[&path](std::fstream& in, int count)->void
		{
		in.seekg(count, std::ios::cur);
		if(!in.good())
			throw ResourceLoadException("Unexpected end of file");
		};

	/*****************************************************************************/
	/*****************************************************************************/
	/*****************************************************************************/
	auto ptr=std::make_shared<File>();
	auto& file=*ptr;

	file.path=path;

	std::fstream in;
	in.open(path.c_str(), std::ios::in|std::ios::binary);

	if(!in.is_open())
		{
		throw ResourceLoadException("Could not open the file");
		}

	// Load file
	skipBytes(in, 4); // File size

	if(readUInt16(in)!=ASEPRITE_MAGIC_NUMBER_FILE)
		{
		throw ResourceLoadException("Invalid Aseprite file (magic number -> file)");
		}

	const unsigned short FRAME_COUNT=readUInt16(in);
	file.width=readUInt16(in);
	file.height=readUInt16(in);
	const unsigned short COLOR_DEPTH=readUInt16(in);

	if(COLOR_DEPTH!=32)
		{
		throw ResourceLoadException("Not supported color depth: "+std::to_string(COLOR_DEPTH)+"bpp");
		}

	skipBytes(in, 4); // File flags
	skipBytes(in, 2); // Deprecated speed
	skipBytes(in, 108);

	UserData* userDataAcceptor=nullptr;
	std::map<int, Layer*> layerIndex;

	for(unsigned idxFrame=0u; idxFrame<FRAME_COUNT; ++idxFrame)
		{
		//const unsigned int frameSize=readUInt32(in);
		skipBytes(in, 4);

		if(readUInt16(in)!=ASEPRITE_MAGIC_NUMBER_FRAME)
			{
			throw ResourceLoadException("Invalid Aseprite file (magic number -> frame)");
			}

		file.frames.push_back(std::make_unique<Frame>());
		Frame* frame=file.frames.back().get();

		const unsigned short CHUNK_COUNT=readUInt16(in);
		frame->duration=readUInt16(in);
		skipBytes(in, 6);

		//LOG_DEBUG(" - Frame {:3d}/{:3d} [at 0x{:08X}]", idxFrame, FRAME_COUNT, in.tellg());

		for(unsigned idxChunk=0u; idxChunk<CHUNK_COUNT; ++idxChunk)
			{
			const unsigned int CHUNK_SIZE=readUInt32(in);
			const unsigned short CHUNK_TYPE=readUInt16(in);

			//LOG_DEBUG("   - Chunk {:3d}/{:3d} [0x{:08X} size {}][type {}]", idxChunk, CHUNK_COUNT, in.tellg(), CHUNK_SIZE, CHUNK_TYPE);

			switch(CHUNK_TYPE)
				{
				case CHUNK_LAYER:
					{
					file.layers.push_back(std::make_unique<Layer>());
					Layer* layer=file.layers.back().get();

					const unsigned short LAYER_FLAGS=readUInt16(in);
					const unsigned short LAYER_TYPE=readUInt16(in);

					layer->isVisible=LAYER_FLAGS&FLAG_LAYER_VISIBLE;
					layer->isGroup=LAYER_TYPE==LAYER_GROUP;

					const unsigned short LAYER_CHILD_LEVEL=readUInt16(in);
					skipBytes(in, 4);

					layer->blendMode=static_cast<BlendMode>(readUInt16(in));
					layer->opacity=readUInt32(in);
					layer->name=readString(in, readUInt16(in));

					if(LAYER_CHILD_LEVEL==0)
						{
						// Highest level layer
						}
					else
						{
						layer->layerParent=layerIndex[LAYER_CHILD_LEVEL-1];
						layer->layerParent->layerChildren.push_back(layer);

						if(!layer->layerParent->isVisible)
							layer->isVisible=false;
						}

					layerIndex[LAYER_CHILD_LEVEL]=layer;
					userDataAcceptor=layer;
					}
				break;

				case CHUNK_CEL:
					{
					file.cels.push_back(std::make_unique<Cel>());
					Cel* cel=file.cels.back().get();

					const unsigned short LAYER_INDEX=readUInt16(in);
					if(frame->cels.size()<=LAYER_INDEX)
						frame->cels.resize(LAYER_INDEX+1, nullptr);

					cel->position.x=readInt16(in);
					cel->position.y=readInt16(in);
					cel->opacity=readUInt8(in);
					cel->frame=frame;
					cel->layer=file.layers[LAYER_INDEX].get();

					const unsigned short CEL_TYPE=readUInt16(in);
					skipBytes(in, 7);

					switch(CEL_TYPE)
						{
						case CEL_RAW:
							{
							cel->size.x=readUInt16(in);
							cel->size.y=readUInt16(in);

							const unsigned int CEL_DATA_LENGTH=CHUNK_SIZE-26;
							if(cel->layer->isVisible)
								{
								cel->pixels=std::shared_ptr<uint8_t[]>(readBytes(in, CEL_DATA_LENGTH));
								}
							else
								{
								skipBytes(in, CEL_DATA_LENGTH);
								}
							}
						break;

						case CEL_LINKED:
							{
							const unsigned short CEL_LINK=readUInt16(in);

							cel->pixels=file.frames[CEL_LINK]->cels[LAYER_INDEX]->pixels;
							cel->size=file.frames[CEL_LINK]->cels[LAYER_INDEX]->size;
							}
						break;

						case CEL_COMPRESSED:
							{
							cel->size.x=readUInt16(in);
							cel->size.y=readUInt16(in);

							const unsigned int CEL_DATA_LENGTH=CHUNK_SIZE-26;
							if(cel->layer->isVisible)
								{
								std::unique_ptr<uint8_t[]> cpixels(readBytes(in, CEL_DATA_LENGTH));

								unsigned long int celDataLengthUncompressed=cel->size.x*cel->size.y*4;
								cel->pixels=std::shared_ptr<uint8_t[]>(new uint8_t[celDataLengthUncompressed]);

								int ret=uncompress(cel->pixels.get(), &celDataLengthUncompressed, cpixels.get(), CEL_DATA_LENGTH);

								if(ret!=Z_OK)
									throw ResourceLoadException("Could not extract the data (zlib err: "+std::to_string(ret)+")");
								}
							else
								{
								skipBytes(in, CEL_DATA_LENGTH);
								}
							}
						break;


						default:
							throw ResourceLoadException("Invalid Aseprite file (unsupported cel type "+std::to_string(CEL_TYPE)+")");
						break;
						}

					userDataAcceptor=cel;
					frame->cels[LAYER_INDEX]=cel;
					}
				break;

				case CHUNK_CELEXTRA:
					skipBytes(in, CHUNK_SIZE-4-2);
				break;

				case CHUNK_FRAME_TAGS:
					{
					const unsigned short TAG_COUNT=readUInt16(in);
					skipBytes(in, 8);

					for(unsigned idxTag=0u; idxTag<TAG_COUNT; ++idxTag)
						{
						file.tags.push_back(std::make_unique<FrameTag>());
						FrameTag* tag=file.tags.back().get();

						tag->frameFrom=readUInt16(in);
						tag->frameTo=readUInt16(in);
						tag->direction=static_cast<AnimationDirection>(readUInt16(in));
						skipBytes(in, 7);

						const unsigned int TAG_COLOR=readUInt32(in);
						tag->color.r=(TAG_COLOR>> 0)&0xFF;
						tag->color.g=(TAG_COLOR>> 8)&0xFF;
						tag->color.b=(TAG_COLOR>>16)&0xFF;
						tag->color.a=(TAG_COLOR>>24)&0xFF;
						tag->name=readString(in, readUInt16(in));
						}
					}
				break;

				case CHUNK_PALETTE:
					skipBytes(in, CHUNK_SIZE-4-2);
				break;

				case CHUNK_USERDATA:
					{
					const unsigned int flags=readUInt32(in);
					if(flags&FLAG_USERDATA_TEXT)
						{
						const std::string udataText=readString(in, readUInt16(in));

						if(userDataAcceptor)
							{
							userDataAcceptor->text=udataText;
							}
						}
					if(flags&FLAG_USERDATA_COLOR)
						{
						const unsigned int udataColor=readUInt32(in);

						if(userDataAcceptor)
							{
							userDataAcceptor->color.r=(udataColor>> 0)&0xFF;
							userDataAcceptor->color.g=(udataColor>> 8)&0xFF;
							userDataAcceptor->color.b=(udataColor>>16)&0xFF;
							userDataAcceptor->color.a=(udataColor>>24)&0xFF;
							}
						}
					}
				break;

				case CHUNK_SLICE:
					{
					file.slices.push_back(std::make_unique<Slice>());
					Slice* slice=file.slices.back().get();

					skipBytes(in, 4);
					const unsigned int SLICE_FLAGS=readUInt32(in);
					skipBytes(in, 4);
					slice->name=readString(in, readUInt16(in));
					skipBytes(in, 4);
					slice->position.x=readUInt32(in);
					slice->position.y=readUInt32(in);
					slice->size.x=readUInt32(in);
					slice->size.y=readUInt32(in);

					if(SLICE_FLAGS&FLAG_SLICE_9SLICES)
						{
						slice->has9Slice=true;
						slice->position9slice.x=readUInt32(in);
						slice->position9slice.y=readUInt32(in);
						slice->size9slice.x=readUInt32(in);
						slice->size9slice.y=readUInt32(in);
						}
					if(SLICE_FLAGS&FLAG_SLICE_PIVOT)
						{
						slice->hasPivot=true;
						slice->pivot.x=readUInt32(in);
						slice->pivot.y=readUInt32(in);
						}

					userDataAcceptor=slice;
					}
				break;

				default:
					//LOG_DEBUG(" - Unsupported: {}", chunkType);
					skipBytes(in, CHUNK_SIZE-4-2);
				break;
				}
			}

		frame->cels.resize(file.layers.size(), nullptr);
		}

	// Tag frames
	for(auto& tag: file.tags)
		{
		for(int i=tag->frameFrom; i<=tag->frameTo; ++i)
			{
			tag->frames.push_back(file.frames[i].get());
			file.frames[i]->tags.push_back(tag.get());
			}
		}

	in.close();

	return ptr;
	}
