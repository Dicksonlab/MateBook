/**
 *  @file	VideoStream.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the VideoStream class
 *	@date	10.07.09
 */

#ifndef __video_stream_h
#define __video_stream_h

struct AVStream;

namespace mw
{
	
	class VideoFormat;
	
	/**
	 *	@class	VideoStream
	 *	@brief	class handeling a single videoStream
	 */
	class VideoStream
		{
			
		public:
			
			/**
			 *	default constructor.
			 */
			VideoStream();
			
			/**
			 *	default destructor.
			 */
			virtual ~VideoStream();
			
			/**				
			 *	initialises a new video stream
			 *	@param	mOutputFormat the format of the stream
			 *	@return	returns if success
			 */
			bool initNewStream(VideoFormat * mFormat);
			
			/**
			 *	access to AVStream struct
			 *	@return	returns pointer to the struct
			 */
			AVStream * getAVStream() const;
			
		private:
			
			AVStream		* avStream_;				/**< current stream. */
			
		};
}

#endif //__video_h

