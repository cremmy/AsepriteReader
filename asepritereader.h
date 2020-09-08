/*
 * asepritereader.h
 *
 *  Created on: 21 kwi 2020
 *      Author: crm
 */

#pragma once

#include <exception>
#include <memory>
#include <string>
#include <vector>

class AsepriteReader final
	{
	public:
		// TODO Check if values are correct
		enum class BlendMode
			{
			NORMAL=0,

			DARKEN=1,
			MULTIPLY=2,
			COLOR_BURN=3,

			LIGHTEN=4,
			SCREEN=5,
			COLOR_DODGE=6,
			ADDITION=7,

			OVERLAY=8,
			SOFT_LIGHT=9,
			HARD_LIGHT=10,

			DIFFERENCE=11,
			EXCLUSION=12,
			SUBTRACT=13,
			DIVIDE=14,

			HUE=15,
			SATURATION=16,
			COLOR=17,
			LUMINOSITY=18,
			};

		enum class AnimationDirection
			{
			FORWARD=0,
			REVERSE=1,
			PINGPONG=2,
			};

	protected:
		struct Vector
			{
			int x;
			int y;
			};
		struct Color
			{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
			};
			
		struct UserData
			{
			std::string text;
			Color color;
			};
			
		class ResourceLoadException: public std::exception
			{
			protected:
				std::string message;

			public:
				ResourceLoadException(const std::string& message): exception(), message(message) {}

				virtual const char* what() const noexcept override {return message.c_str();}
			};

	public:
		struct Frame;
		struct FrameTag;
		struct Layer;
		struct Cel;
		struct Slice;

		struct AsepriteFile
			{
			int width;
			int height;

			std::vector<std::unique_ptr<Frame>> frames;
			std::vector<std::unique_ptr<FrameTag>> tags;
			std::vector<std::unique_ptr<Layer>> layers;
			std::vector<std::unique_ptr<Cel>> cels;
			std::vector<std::unique_ptr<Slice>> slices;
			};

		struct Frame
			{
			int duration=0; // ms

			std::vector<Cel*> cels;
			std::vector<FrameTag*> tags;
			};

		struct FrameTag: public UserData
			{
			std::string name;

			int frameFrom=0;
			int frameTo=0;

			AnimationDirection direction; // forward, reverse, pingpong

			std::vector<Frame*> frames;
			};

		struct Layer: public UserData
			{
			std::string name;

			bool isGroup=false;
			bool isVisible=true;
			int opacity=0; // 0-255
			BlendMode blendMode;

			Layer* layerParent=nullptr;
			std::vector<Layer*> layerChildren;
			};

		struct Cel: public UserData
			{
			Vector position;
			Vector size;
			int opacity=0;

			std::shared_ptr<uint8_t> pixels; // RGBA

			Frame* frame=nullptr;
			Layer* layer=nullptr;
			};

		struct Slice: public UserData
			{
			std::string name;

			Vector position;
			Vector size;

			bool has9Slice=false;
			Vector position9slice;
			Vector size9slice;

			bool hasPivot=false;
			Vector pivot;
			};

	public:
		AsepriteFile file;

	public:
		AsepriteReader()=default;
		~AsepriteReader()=default;

		void load(const std::string& path);
	};
