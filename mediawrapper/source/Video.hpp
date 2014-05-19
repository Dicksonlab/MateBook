/**
 *  @file	Video.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the Video class
 *	@date	18.06.09
 */

#ifndef __video_h
#define __video_h

#include <string>
#if defined(WIN32) || defined(LINUX)
#include <stdint.h>
#endif

#include <vector>

namespace mw
{
	class VideoFormat;
	class VideoCodec;
	class VideoFrame;
	
	/**
	 *	@brief	structure storing information about an audio stream. 
	 */
	typedef struct 
	{
		VideoCodec *		  audioCodec_;			/**< the codec for the audio stream. */
		int					  numChannels_;			/**< the number of channels. */		
		int					  sampleRate_;			/**< the sample rate of the audio. */
		int					  sampleCount_;			/**< the number of samples. */	
	}
	audioInfo;
}

#endif //__video_h

