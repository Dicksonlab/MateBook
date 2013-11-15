#ifndef __output_video_h
#define __output_video_h

#include "Video.hpp"

#include <vector>
#include <string>

namespace mw
{
	class VideoFormat;
	class VideoCodec;
	class VideoFrame;
	class VideoOutputFormat;
	class VideoStream;
	
	/**
	 *	@class	OutputVideo
	 *	@brief	class handling an output video derived from Video
	 */
	class OutputVideo {
	public:
		
		/**
		 *	constructor
		 *	codec, bitrate and pixelformat are inferred from the file extension
		 *	@param	mFileName the file name of the file to be created
		 *	@param	mWidth the width of the video in pixels
		 *	@param	mHeight the height of the video in pixels
		 *	@param	mFrameRate the framerate
		 */
		OutputVideo(char * mFileName,
					int mWidth,
					int mHeight,
					double mFrameRate);
		
		/**
		 *	constructor
		 *	@param	mFileName the file name of the file to be created
		 *	@param	mWidth the width of the video in pixels
		 *	@param	mHeight the height of the video in pixels
		 *	@param	mFrameRate the framerate
		 *	@param	mCodecID the ffmpeg codec id of the codec to be used
		 *	@param	mBitRate the bitrate of the new video
		 *	@param	mPixelFormat the pixelformat to be used
		 */
		OutputVideo(char * mFileName,
					int mWidth,
					int mHeight,
					double mFrameRate,
					unsigned int mCodecID,
					unsigned int mBitRate,
					unsigned int mPixelFormat);
		
		/**
		 *	destructor
		 */
		virtual ~OutputVideo();

		/**
		 *	returns the pointer to the video format
		 *	@return	returns the pointer to the member
		 */
		VideoFormat * getVideoFormat() const;
		
		/**
		 *	returns the pointer to the video codec
		 *	@return	returns the pointer to the member
		 */
		VideoCodec * getVideoCodec() const;
		
		/**
		 *	returns a string containing some video information
		 *	@return	returns a string containing format and codec information
		 */
		std::string getVideoInformation() const;
		
		/**
		 *	gets the number of frames in the video, calculated using fps and video duration
		 *	@return	returns the number of frames
		 */
		int64_t getNumberOfFrames();
		
		/**
		 *	returns the frame rate
		 *	@return	returns the videos fps
		 */
		float getFrameRate();
		
		/**
		 *	accessor for width of video
		 *	@return	returns the width
		 */
		int getFrameWidth();
		
		/**
		 *	accessor for height of video
		 *	@return	returns the height
		 */
		int getFrameHeight();
		
		/**
		 *	returns the size of the group of pictures
		 *	@return	returns the number of frames in a group of pictures
		 */
		int getGopSize();
		
		/**
		 *	returns the current frame number the video state points to
		 *	@return	returns the number of the last frame read within the video
		 */
		unsigned int getCurrentFrameNumber();
		
		/**
		 *	gets the video stream object
		 *	@result	the video stream
		 */
		VideoStream * getVideoStream() const;
		
		/**
		 *	appends the video frame to the video
		 *	@param	mFrameData the raw data of the frame
		 *	@param	mPixelFormat the pixelformat
		 *	@return	returns if success
		 */
		bool appendVideoFrame(unsigned char * mFrameData,
							  unsigned int mPixelFormat);
		
		/**
		 *	closes the video and writes trailer
		 */
		void close();

	private:
		std::string fileName_;
		VideoFormat			* videoFormat_;			/**< pointer to the format of the current video. */
		VideoCodec			* videoCodec_;			/**< pointer to the codec of the current video.*/
		int64_t				  numFrames_;			/**< number of frames found in the current video. */
		float				  fps_;					/**< frames per second of video. */
		int					  width_;				/**< width of frame. */
		int					  height_;				/**< height of frame. */
		int					  gopSize_;				/**< size of the gop. */
		int64_t				  curFrame_;			/**< number of current frame. this is where we are in the video. */
		int64_t				  lastFrame_;			/**< the frame that will be returned when getVideoFrame is called. */
		std::vector<audioInfo>	audioInfos_;		/**< the informations for each audio stream. */

		VideoOutputFormat	* outputFormat_;		/**< output format. Just needed for encoding. */
		VideoStream			* outputStream_;		/**< output stream. Just needed for encoding. */
		
		unsigned int		  codecID_;				/**< the id of the codec. */
		unsigned int		  bitRate_;				/**< the bitrate of the codec. */
		unsigned int		  pixelFormat_;			/**< the pixelformat of the codec. */
	
		unsigned int		  bitBufferSize_;		/**< the size of the bit buffer used for encoding. */
		uint8_t				* bitBuffer_;			/**< the bitbuffer used for encoding. */
	};
}

#endif //__output_video_h
