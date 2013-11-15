/**
 *  @file	VideoCodec.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the VideoCodec class
 *	@date	18.06.09
 */

#ifndef __video_codec_h
#define __video_codec_h

#include <stdint.h>


extern uint64_t global_video_pkt_pts;

#include <string>



struct AVCodecContext;
struct AVCodec;

namespace mw
{
	
	class VideoFormat;
	class VideoStream;
	
	/**
	 *	@class	VideoCodec
	 *	@brief	class handeling a single videocodec
	 */
	class VideoCodec
	{
		
	public:
		
		/**
		 *	default constructor
		 */
		VideoCodec();
		
		/**
		 *	default destructor
		 */
		~VideoCodec();
		
		/**
		 *	loads the codec from the given Video Format
		 *	@param	mFormat the given VideoFromat
		 *	@param	mStreamNumber the given streamnumber
		 *	@return	returns if the codec is supported.
		 */
		bool loadCodec(const VideoFormat * mFormat,
					   unsigned int mStreamNumber = 0);
		
		/**
		 *	loads the codec from the given stream
		 *	@param	mStream stream where codeccontext is located
		 *	@param	mCodecID which codec should be used
		 *	@param	mPixelFormat which pixelformat
		 *	@param	mWidth width of frame
		 *	@param	mHeight height of frame
		 *	@param	mNum part of timebase
		 *	@param	mDen part of timebase
		 *	@param	mInterlaced if the source material is interlaced
		 *	@param	mBitRate the target bitrate
		 *	@return	returns if successfull
		 */
		bool loadCodec(const VideoStream * mStream,
					   unsigned int mCodecID,
					   unsigned int mPixelFormat,
					   int mWidth,
					   int mHeight,
					   int mNum,
					   int mDen,
					   bool mInterlaced,
					   unsigned int mBitRate = 100000);
		
		/**
		 *	provides access to AVCodecContext
		 *	@return	returns a read only pointer to the struct
		 */
		AVCodecContext * getAVCodecContext() const;
		
		/**
		 *	provides access to AVCodec
		 *	@return	returns a read only pointer to the struct
		 */
		AVCodec * getAVCodec() const;
		
		/**
		 *	gets the information about the videocodec.
		 *	@return	returns a string containing all the information
		 */
		std::string getCodecInformation() const;
		
	private:
		
		AVCodecContext	* codecContext_;			/**< codeccontext of the current file. */
		AVCodec			* codec_;					/**< codec of the current file. */
	};
}

#endif //__video_codec_h

