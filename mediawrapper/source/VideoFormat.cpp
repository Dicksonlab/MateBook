/**
 *  @file	VideoFormat.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the VideoFormat class
 *	@date	18.06.09
 */

#define __STDC_CONSTANT_MACROS
#include <stdint.h>

#include "VideoFormat.hpp"
#include "VideoOutputFormat.hpp"

#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
}


namespace mw
{
	VideoFormat::VideoFormat() :
	avFormatContext_(NULL),
	isInputFile_(false),
	fps_(0.0),
	firstFramePts_(0.0)
	{
	}
	
	VideoFormat::~VideoFormat()
	{
		/**
		 *	depending on if we are dealing with an input or output
		 *	file we need to close the avformat with a different 
		 *	function
		 */
		if(avFormatContext_)
		{
			if(isInputFile_)
			{
				avformat_close_input(& avFormatContext_);
			}
			else
			{
				if(avFormatContext_->pb)
				{
					avio_close(avFormatContext_->pb);
				}
			}
			avFormatContext_ = NULL;
		}
	}
	
	bool VideoFormat::loadAVFormatContext(std::string mFileName)
	{
		/**
		 *	we open the video format for an input file
		 */
		if(avformat_open_input(& avFormatContext_, mFileName.c_str(), NULL, NULL) != 0)
		{
			std::cerr << "Could not open input file!" << std::endl;
			return false;
		}
		/**
		 *	query information about the streams found in the video
		 */
		AVDictionary **options = NULL;
		if(avformat_find_stream_info(avFormatContext_, options) < 0)
		{
			std::cerr << "Could not find the stream info!" << std::endl;
			return false;
		}
#ifdef _DEBUG
		av_dump_format(avFormatContext_, 0, mFileName.c_str(), 0);
#endif		
		isInputFile_ = true;
		return true;
	}
	
	bool VideoFormat::allocAVFormatContext()
	{
		/**
		 *	since we are not dealing with an input file, we create the
		 *	format context ourselves
		 */
		avFormatContext_ = avformat_alloc_context();

		if(avFormatContext_)
		{
			isInputFile_ = false;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void VideoFormat::addOutputFormat(VideoOutputFormat * mOutputFormat)
	{
		/**
		 *	we add the output format to the context
		 */
		avFormatContext_->oformat = mOutputFormat->getAVOutputFormat();
	}
	
	bool VideoFormat::openOutputFile(std::string mFileName)
	{
		/**
		 *	for output files we need to tell ffmpeg to use our created
		 *	format context as an output file with the given path
		 */
		if(avio_open(& (avFormatContext_->pb), mFileName.c_str(), AVIO_FLAG_WRITE) < 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	
	bool VideoFormat::writeHeader()
	{
		if(avformat_write_header(avFormatContext_, NULL))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	bool VideoFormat::writeTrailer()
	{
		if(av_write_trailer(avFormatContext_))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	AVFormatContext * VideoFormat::getAVFormatContext() const
	{
		return avFormatContext_;
	}
	
	unsigned int VideoFormat::getStreamIndexForVideoStream(unsigned int mVideoIndex/* = 0*/)
	{
		/**
		 *	we want the index of the mVideoIndex-th video stream in the array
		 *	of streams (which can also store audio streams)
		 */
		unsigned int result = -1;
		
		for(unsigned int i = 0; i < avFormatContext_->nb_streams; i++)
		{
			if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
			{
				result++;
				
			}
			if(result == mVideoIndex)
			{
				return i;
			}
		}
		
		return result;
	}
	
	unsigned int VideoFormat::getStreamIndexForAudioStream(unsigned int mAudioIndex/* = 0*/)
	{
		/**
		 *	we want the index of the mAudioIndex-th audio stream in the array
		 *	of streams (which can also store audio streams)
		 */
		unsigned int result = -1;
		
		for(unsigned int i = 0; i < avFormatContext_->nb_streams; i++)
		{
			if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) 
			{
				result++;
				
			}
			if(result == mAudioIndex)
			{
				return i;
			}
		}
		
		return result;
	}
	
	std::vector<unsigned int> VideoFormat::getStreamIndicesForAudioStreams()
	{
		/**
		 *	we return the indices for all the audio streams in the file
		 */
		std::vector<unsigned int> result;
		
		for(unsigned int i = 0; i < avFormatContext_->nb_streams; i++)
		{
			if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) 
			{
				result.push_back(i);
			}
		}
		
		return result;
	}
	
	int64_t VideoFormat::frameNumberToTimeStamp(uint64_t mFrameNumber, bool mInterlaced, unsigned int mStreamIndex/* = 0*/) const
	{
		/**
		 *	the frame number is converted into a time stamp in seconds based on the stored fps information
		 */
		float timestampSeconds = ((float) (mFrameNumber)) / fps_;
		
		/**
		 *	converts the time stamp in seconds to the standard ffmpeg time base
		 */
		int64_t timestamp = (int64_t)(timestampSeconds * AV_TIME_BASE);
		
		/**
		 *	converts the time stamp in ffmpegs time base to the time base the stream given with mStreamIndex
		 *	uses for describing time
		 */
		AVRational timeBase = {1, AV_TIME_BASE};	// AV_TIME_BASE_Q macro doesn't compile on windows
		timestamp = av_rescale_q(timestamp, timeBase, avFormatContext_->streams[mStreamIndex]->time_base);
		return timestamp;
	}
	
	void VideoFormat::setFps(float mFps)
	{
		fps_ = mFps;
	}
	
	float VideoFormat::getFps() const
	{
		return fps_;
	}
	
	void VideoFormat::setFirstFramePts(float mFirstFramePts)
	{
		firstFramePts_ = mFirstFramePts;
	}
	
	float VideoFormat::getFirstFramePts() const
	{
		return firstFramePts_;
	}
}

