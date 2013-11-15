/**
 *  @file	VideoFrame.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the VideoFrame class
 *	@date	22.06.09
 */

#ifndef __video_frame_h
#define __video_frame_h

#include <string>
#ifdef WIN32
#include <stdint.h>
#endif

#include "ColorFormat.hpp"

struct AVFrame;
struct AVCodecContext;
struct SwsContext;

namespace mw
{
	
	class VideoFormat;
	class VideoCodec;
	class VideoStream;
	
	/**
	 *	@class	VideoFrame
	 *	@brief	class handeling a single videoframe
	 */
	class VideoFrame
	{
		
	public:
		
		/**
		 *	default constructor
		 */
		VideoFrame();
		
		/**
		 *	constructor with given frame data
		 *	@param	mWidth the width of the frame
		 *	@param	mHeight the height of the frame
		 *	@param	mFrameData the raw frame
		 *	@param	mPixelFormat the pixelformat of the frame given
		 */
		VideoFrame(int mWidth,
				   int mHeight,
				   unsigned char * mFrameData,
				   unsigned int mPixelFormat,
				   const VideoCodec * mCodec);

		/**
		 *	default destructor
		 */
		~VideoFrame();
								
		/**
		 *	provides readonly access to the AVFrame struct
		 *	@return	returns a readonly pointer to the struct
		 */
		const AVFrame * getAVFrame() const;
		
		/**
		 *	gets the information about the videoframe.
		 *	@return	returns a string containing all the information
		 */
		std::string getFrameInformation() const;
		
		/**
		 *	saves the given Frame to disc.
		 *	the saved image is saved in a ppm format.
		 *	@param	mFrame	the frame to save
		 *	@param	mWidth	width of frame
		 *	@param	mHeight	height of frame
		 *	@param	mNumFrame number of the frame
		 */
		void saveFrame(int mNumFrame); 
		
		/**
		 *	returns the presentation time of the frame
		 *	@return	pts
		 */
		int64_t getFramePts() const;
		
		/**
		 *	returns the number of bytes needed for storing the image
		 *	@return	returns numBytes_
		 */
		int getNumBytes() const;
		
		/**
		 *	returns a read only pointer to the raw image data
		 *	@return	returns a pointer to imageBuffer_
		 */
		uint8_t * getRawBufferPtr();
		
		/**
		 *	converts the current frame to a given pixelformat.
		 *	the frameData_ array is not used any more and the 
		 *	frameRGB_ pointer stores a frame in the given pixel format.
		 *	@param	mPixelFormat the pixelformat of the new frame.
		 */
		void convertRGBToPixelFormat(unsigned int mPixelFormat);
		
		/**
		 *	converts the current frame to a given pixelformat.
		 *	the frameData_ array is not used any more and the 
		 *	frameRGB_ pointer stores a frame in the given pixel format.
		 *	@param	mSourcePixelFormat the pixelformat that is assumed the current frame is
		 *	@param	mTargetPixelFormat the pixelformat of the new frame.
		 */
		void convertToPixelFormat(unsigned int mSourcePixelFormat,
								  unsigned int mTargetPixelFormat);
		
		/**
		 *	encodes and writes the frame to the final video
		 *	@param	mFormat VideoFormat used for encoding
		 *	@param	mCodec VideoCodec used for encoding
		 *	@param	mStream VideoStream of the output
		 *	@param	mBitBufferSize size of buffer used for encoding
		 *	@param	mBitBuffer buffer used for encoding
		 *	@return	returns if success
		 */
		bool encodeAndWriteFrame(VideoFormat * mFormat,
								 VideoCodec * mCodec,
								 VideoStream * mStream,
								 unsigned int mBitBufferSize,
								 uint8_t * mBitBuffer);
		
		/**
		 *	reads the number of frames before the first key frame.
		 *	it resets the video after
		 *	@param	mFormat	the current VideoFormat
		 *	@param	mCodec	the current VideoCodec
		 *	@param	mInterlaced output variable, checks if frames are interleaved
		 *	@param	mFramesBeforeKey output variable, the number of frames before the first keyframe
		 *	@param	mFps output variable, the approximate fps of the video 
		 *	@param	mGop the gop size
		 *	@param	mStreamIndex the index of the video stream to load
		 */
		void getFrameStats(const VideoFormat * mFormat,
						   const VideoCodec * mCodec,
						   bool * mInterlaced,
						   int * mFramesBeforeKey,
						   float * mFps,
						   int * mGop,
						   unsigned int mStreamIndex = 0);
		
		/**
		 *	loads the Frame from the given VideoFormat
		 *	@param	mFormat	the current VideoFormat
		 *	@param	mCodec	the current VideoCodec
		 *	@param	mScaleContext the software scale context
		 *	@param	mColorConversionContext the contest used for color conversion
		 *	@param	mCurFrameNumber the framenumber where we are in the video
		 *	@param	mTargetFrameNumber the framenumber where we want to be
		 *	@param	mStreamIndex the index of the video stream to load
		 *	@param	mConvert if the frame should be converted to rgb
		 *	@param	mLoadKeyFrame if the next keyFrame instead of the next frame should be loaded
		 *	@return	returns if success
		 */
		bool loadFrame(const VideoFormat * mFormat,
					   const VideoCodec * mCodec,
					   SwsContext * mScaleContext,
					   SwsContext * mColorConversionContext,
					   int64_t mCurFrameNumber,
					   int64_t mTargetFrameNumber,
					   unsigned int mStreamIndex = 0,
					   bool mConvert = true,
					   bool mLoadKeyFrame = false);
		
	private:
		
		/**
		 *	converts the given AVFrame into the rgb frame stored by the class instance
		 *	@param	mTempFrame the frame with the immediate data stored in YUV
		 *	@param	mTempBuffer reference to the array where mTempFrame stores its data
		 *	@param	mCodec Codec used for the current stream
		 *	@param	mScaleContext the software scale context
		 *	@param	mColorConversionContext the context used for color conversion
		 */
		void convertFrame(AVFrame * & mTempFrame,
						  uint8_t * & mTempBuffer,
						  const VideoCodec * mCodec,
						  SwsContext * mScaleContext,
						  SwsContext * mColorConversionContext);
		
		/**
		 *	prepares the current frame for filling with data
		 *	@param	mCodec codec used for decoding
		 *	@param	mWidth the with of the data, defaults to 0, takes from codec then
		 *	@param	mHeight the height of the data, defaults to 0, takes from codec then
		 *	@param	mPixelFormat the pixelformat of the frame
		 */
		void prepareFrame(const VideoCodec * mCodec,
						  int mWidth = 0,
						  int mHeight = 0,
						  unsigned int mPixelFormat = PIX_FMT_RGB24);
		
		/**
		 *	deinterlaces the current frame by using the skip field technique.
		 *	only to be called on the rgb frame!
		 */
		void deinterlaceSkipField();
		
		AVFrame		* frameRGB_;			/**< AVFrame pointer storing the single frame in RGB. The format conversion is already done after loading. */
		int			  numBytes_;			/**< number of bytes that the imageBuffer_ actually has. */
		uint8_t		* imageBuffer_;			/**< Buffer where the actual image data is stored. It is linked with the AVFrame struct. */
		
		AVFrame		* decodedFrame_;		/**< AVFrame pointer storing the single frame after decoding. */
		int			  decodedNumBytes_;		/**< number of bytes that the decodedImageBuffer_ actually has. */
		uint8_t		* decodedImageBuffer_;	/**< Buffer where the actual image data is stored. It is linked with the AVFrame struct. */
		
		AVFrame		* deinterlacedFrame_;		/**< AVFrame pointer storing the single frame after deinterlacing. */
		int			  deinterlacedNumBytes_;	/**< number of bytes that the deinterlacedImageBuffer_ actually has. */
		uint8_t		* deinterlacedImageBuffer_;	/**< Buffer where the actual image data is stored. It is linked with the AVFrame struct. */
		
		AVFrame		* fullSizeFrame_;			/**< AVFrame pointer storing the single frame after sizing up. */
		int			  fullSizeNumBytes_;		/**< number of bytes that the fullSizeImageBuffer_ actually has. */
		uint8_t		* fullSizeImageBuffer_;		/**< Buffer where the actual image data is stored. It is linked with the AVFrame struct. */
		
		int			  height_;				/**< height of frame. */
		int			  width_;				/**< width of frame. */
		double		  pts_;					/**< presentation time of frame. */
	};
}
#endif //__video_frame_h
