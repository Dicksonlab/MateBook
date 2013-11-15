/**
 *  @file	OutputVideo.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the OutputVideo class
 *	@date	13.05.10
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include "OutputVideo.hpp"

#include "VideoFormat.hpp"
#include "VideoCodec.hpp"
#include "VideoFrame.hpp"
#include "VideoOutputFormat.hpp"
#include "VideoStream.hpp"

#include <iostream>
#include <cmath>
#include <stdexcept>

extern "C"
{
#include <libavformat/avformat.h>
}

namespace mw
{
	OutputVideo::OutputVideo(char * mFileName, int mWidth, int mHeight, double mFrameRate) :
		fileName_(mFileName),
		videoFormat_(NULL),
		videoCodec_(NULL),
		numFrames_(0),
		fps_(mFrameRate),
		width_(mWidth),
		height_(mHeight),
		gopSize_(0),
		curFrame_(0),
		lastFrame_(0),
		audioInfos_(),
		outputFormat_(0),
		outputStream_(0),
		bitBufferSize_(0),
		bitBuffer_(0),
		codecID_(CODEC_ID_MPEG4),
		bitRate_(mWidth * mHeight * 0.048 * mFrameRate),	// width * height * frameRate * 3 colors * 8bits / 500 
		pixelFormat_(PIX_FMT_YUV420P)
	{
		/**
		 *	we create the new video format from default values
		 */
		videoFormat_ = new VideoFormat();
		videoFormat_->allocAVFormatContext();
		
		/**
		 *	since we are writing we also need the output format, that will be created based on the 
		 *	file ending of the output file
		 */
		outputFormat_ = new VideoOutputFormat();
		if (!outputFormat_->guessOutputFormat(fileName_)) {
			throw std::runtime_error("Failed to guess output format.");
		}
		
		/**
		 *	we add the output format to the videoFormat, so we can use the ffmpeg functions
		 */
		videoFormat_->addOutputFormat(outputFormat_);
		if (!videoFormat_->openOutputFile(fileName_)) {
			throw std::runtime_error("Failed to open output file.");
		}
		
		/**
		 *	given our video information we need to create an output stream
		 *	for writing the frames
		 */
		outputStream_ = new VideoStream();
		if (!outputStream_->initNewStream(videoFormat_)) {
			throw std::runtime_error("Failed to init new stream.");
		}
		/**
		 *	we do this so our video codecs match and we don't try to encode mismatching codecs
		 */
		codecID_ = outputFormat_->getAVOutputFormat()->video_codec;

	}
	
	OutputVideo::OutputVideo(char * mFileName, int mWidth, int mHeight, double mFrameRate, unsigned int mCodecID, unsigned int mBitRate, unsigned int mPixelFormat) :
		fileName_(mFileName),
		videoFormat_(NULL),
		videoCodec_(NULL),
		numFrames_(0),
		fps_(mFrameRate),
		width_(mWidth),
		height_(mHeight),
		gopSize_(0),
		curFrame_(0),
		lastFrame_(0),
		audioInfos_(),
		outputFormat_(0),
		outputStream_(0),
		bitBufferSize_(0),
		bitBuffer_(0),
		codecID_(mCodecID),
		bitRate_(mBitRate),
		pixelFormat_(mPixelFormat)
	{
		/**
		 *	we create the new video format from default values
		 */
		videoFormat_ = new VideoFormat();
		videoFormat_->allocAVFormatContext();
		
		/**
		 *	since we are writing we also need the output format, that will be created based on the
		 *	file ending of the output file
		 */
		outputFormat_ = new VideoOutputFormat();
		if (!outputFormat_->guessOutputFormat(fileName_)) {
			throw std::runtime_error("Failed to guess output format.");
		}
		
		/**
		 *	we add the output format to the videoFormat, so we can use the ffmpeg functions
		 */
		videoFormat_->addOutputFormat(outputFormat_);
		if (!videoFormat_->openOutputFile(fileName_)) {
			throw std::runtime_error("Failed to open output file.");
		}
		
		/**
		 *	given our video information we need to create an output stream
		 *	for writing the frames
		 */
		outputStream_ = new VideoStream();
		if (!outputStream_->initNewStream(videoFormat_)) {
			throw std::runtime_error("Failed to init new stream.");
		}
	}

	VideoFormat * OutputVideo::getVideoFormat() const
	{
		return videoFormat_;
	}
	
	VideoCodec * OutputVideo::getVideoCodec() const
	{
		return videoCodec_;
	}
	
	std::string OutputVideo::getVideoInformation() const
	{
		std::string result;
		result += videoCodec_->getCodecInformation();
		
		return result;
	}
	
	int64_t OutputVideo::getNumberOfFrames()
	{
		return numFrames_;
	}
	
	float OutputVideo::getFrameRate()
	{
		return fps_;
	}
	
	int OutputVideo::getFrameWidth()
	{
		return width_;
	}
	
	int OutputVideo::getFrameHeight()
	{
		return height_;
	}
	
	int OutputVideo::getGopSize()
	{
		return gopSize_;
	}
			
	unsigned int OutputVideo::getCurrentFrameNumber()
	{
		return lastFrame_;
	}

	OutputVideo::~OutputVideo()
	{
		if(bitBuffer_)
		{
			this->close();
		}
		if(outputFormat_)
		{
			delete outputFormat_;
			outputFormat_ = 0;
		}
		
		if(outputStream_)
		{
			delete outputStream_;
			outputStream_ = 0;
		}

		if(videoCodec_)
		{
			delete videoCodec_;
			videoCodec_ = NULL;
		}
		
		if(videoFormat_)
		{
			delete videoFormat_;
			videoFormat_ = NULL;
		}
	}
	
	VideoStream * OutputVideo::getVideoStream() const
	{
		return outputStream_;
	}
	
	bool OutputVideo::appendVideoFrame(unsigned char * mFrameData, unsigned int mPixelFormat)
	{
		/**
		 *	lazy instanciation of the video codec
		 *	based on the information we have from the format - dealing structures
		 */
		if(!videoCodec_)
		{
			videoCodec_ = new VideoCodec();
			if(!videoCodec_->loadCodec(outputStream_,
									   codecID_,
									   pixelFormat_,
									   width_,
									   height_,
									   (unsigned int) fps_,
									   1,
									   false,
									   bitRate_))
			{
				return false;
			}
			
			/**
			 *	starts writing the video here
			 */
			videoFormat_->writeHeader();

			/**
			 *	temporary buffer for encoding
			 */
			bitBufferSize_ = width_ * height_ * 3;
			bitBuffer_ = new uint8_t[bitBufferSize_];
		}
		
		/**
		 *	creating temporary video frame. if needed our incoming frame data is converted
		 *	to the pixel format of our video
		 */
		VideoFrame * newFrame;
		newFrame = new VideoFrame(width_, height_, mFrameData, mPixelFormat, videoCodec_);
		if(mPixelFormat != videoCodec_->getAVCodecContext()->pix_fmt)
		{
			newFrame->convertToPixelFormat(mPixelFormat, videoCodec_->getAVCodecContext()->pix_fmt);
		}
		/**
		 *	here the frame is actually appended to the video
		 */
		bool result = newFrame->encodeAndWriteFrame(videoFormat_, videoCodec_, outputStream_, bitBufferSize_, bitBuffer_);
		
		delete newFrame;
		
		return result;
	}
	
	void OutputVideo::close()
	{
		/**
		 *	writes the end of the video
		 */
		videoFormat_->writeTrailer();
		delete bitBuffer_;
		bitBuffer_ = 0;
		bitBufferSize_ = 0;
	}
}