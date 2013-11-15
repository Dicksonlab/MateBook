/**
 *  @file	VideoStream.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the VideoStream class
 *	@date	18.06.09
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include "VideoStream.hpp"
#include "VideoFormat.hpp"

#include <iostream>
#include <cmath>

extern "C"
{
#include <libavformat/avformat.h>
}


namespace mw
{
	VideoStream::VideoStream() : avStream_(NULL)
	{
	}
	
	VideoStream::~VideoStream()
	{
	}
	
	bool VideoStream::initNewStream(VideoFormat * mFormat)
	{
		/**
		 *	creating a stream in the given video format
		 */
        avStream_ = av_new_stream(mFormat->getAVFormatContext(), 0);
		if(avStream_)
		{
			/**
			 *	defaults from ffmpeg.c
			 */
			avStream_->sample_aspect_ratio.num = 1;
			avStream_->sample_aspect_ratio.den = 1;
			avStream_->pts_wrap_bits = 33;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	AVStream * VideoStream::getAVStream() const
	{
		return avStream_;
	}
}
