/**
 *  @file	VideoOutputFormat.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the VideoOutputFormat class
 *	@date	10.07.09
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include "VideoOutputFormat.hpp"

#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
}


namespace mw
{
	VideoOutputFormat::VideoOutputFormat() : avOutputFormat_(NULL)
	{
	}
	
	VideoOutputFormat::~VideoOutputFormat()
	{
	}
	
	bool VideoOutputFormat::guessOutputFormat(std::string mFileName)
	{
		/**
		 *	guessing the proper output format based on the filename of the future video file
		 */
		avOutputFormat_ = av_guess_format(NULL, mFileName.c_str(), NULL);
		return (avOutputFormat_ != NULL);
	}
	
	AVOutputFormat * VideoOutputFormat::getAVOutputFormat() const
	{
		return avOutputFormat_;
	}
}

