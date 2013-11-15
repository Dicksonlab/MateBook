/**
 *  @file	mediawrapper.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation of the wrapper methods that can be called by the dylib
 *	@date	26.06.09
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include <iostream>
#include <limits>

#include "mediawrapper.hpp"

#include "VideoFrame.hpp"

#include "Video.hpp"
#include "InputVideo.hpp"
#include "OutputVideo.hpp"
#include "VideoFrame.hpp"

#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
}

namespace mw
{
	void initialize()
	{
		/**
		 *	initializes ffmpeg with all available codecs
		 *	we also set, that we don't want ffmpeg to report unless there is a 
		 *	serious error
		 */
		av_register_all();
	#ifndef _DEBUG
		av_log_set_level(AV_LOG_QUIET);
	#else
		av_log_set_level(AV_LOG_QUIET);
		//av_log_set_level(AV_LOG_VERBOSE);
	#endif
	}
}
