/**
 *  @file	mediawrapper.h
 *	@author	Herbert Grasberger
 *  @brief	contains the method definitions that can be used from the dylib
 *	@date	26.06.09
 */

#ifndef __media_wrapper_h
#define __media_wrapper_h

#include "ColorFormat.hpp"
#include "InputVideo.hpp"
#include "OutputVideo.hpp"

namespace mw
{
	/**
	 *	function that needs to be called before the media wrapper classes can be
	 *	used. It initializes ffmpeg
	 */
	void initialize();
}

#endif //__media_wrapper_h
