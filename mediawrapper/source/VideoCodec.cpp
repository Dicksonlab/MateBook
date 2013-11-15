/**
 *  @file	VideoCodec.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the VideoCodec class
 *	@date	18.06.09
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include "VideoCodec.hpp"
#include "VideoFormat.hpp"
#include "VideoFrame.hpp"
#include "VideoStream.hpp"

#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/**
 *	this part here of the code is injected into the video decoding code from ffmpeg
 *	it produces proper video pts, since the default methods from ffmpeg don't work
 *	in some cases
 */
uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
int our_get_buffer(struct AVCodecContext *c, AVFrame *pic) {
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));
	*pts = global_video_pkt_pts;
	pic->opaque = pts;
	
//	std::cout << "getbuffer " << *pts << " ";
	return ret;
}
void our_release_buffer(struct AVCodecContext *c, AVFrame *pic) {
	if(pic) av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}

namespace mw
{
	VideoCodec::VideoCodec() : codec_(NULL)
	{
	}
	
	VideoCodec::~VideoCodec()
	{
		if(codec_)
		{
			/* ===========================================================================
			 since the codec_ is just a pointer to an AVFormatContext member we MUST NOT 
			 delete it! Otherwise we would be freeing memory twice.
			 ========================================================================hg= */
			codec_ = NULL;
		}
		
		if(codecContext_)
		{
			/* ===========================================================================
			 since the codecContext_ is just a pointer to an AVFormatContext member we 
			 MUST NOT delete it! Otherwise we would be freeing memory twice.
			 ========================================================================hg= */
			avcodec_close(codecContext_);
			codecContext_ = NULL;
		}
	}
	
	bool VideoCodec::loadCodec(const VideoFormat * mFormat,
							   unsigned int mStreamNumber/* = 0*/)
	{
		/**
		 *	loads a codec for decoding videos
		 */
		/**
		 *	given a videoformat we load the codec from it.
		 */
		codecContext_ = mFormat->getAVFormatContext()->streams[mStreamNumber]->codec;

		/**
		 *	we get the width and height of a single video frame
		 */
		int width = codecContext_->width;
		int height = codecContext_->height;
		
		/**
		 *	here an actual codec is loaded
		 */
		codec_ = avcodec_find_decoder(codecContext_->codec_id);
		if(codec_ == NULL) 
		{
			std::cerr << "Unsupported codec!" << std::endl;
			return false;
		}	
		
		/**
		 *	based on the information of before, we open the codec
		 */
		AVDictionary ** options = NULL;
		if(avcodec_open2(codecContext_, codec_, options) < 0)
		{
			std::cerr << "Could not open codec!" << std::endl;
			return false;
		}

		/**
		 *	writing the proper size information into our new codec context
		 */
		codecContext_->width = width;
		codecContext_->height = height;
		
		/**
		 *	injecting the custom read functions for decoding
		 *	into the codec context
		 */
		codecContext_->get_buffer = our_get_buffer;
		codecContext_->release_buffer = our_release_buffer;
		
		return true;
	}
	
	bool VideoCodec::loadCodec(const VideoStream * mStream,
							   unsigned int mCodecID,
							   unsigned int mPixelFormat,
							   int mWidth,
							   int mHeight,
							   int mNum,
							   int mDen,
							   bool mInterlaced,
							   unsigned int mBitRate/* = 100000*/)
	{
		/**
		 *	loads a codec for encoding videos
		 */
		
		/**
		 *	most of the values here are taken from ffmpeg.c, the ffmpeg executable
		 *	that also provides encoding information
		 */
		codecContext_ = mStream->getAVStream()->codec;
		
		/**
		 *	basic codec information
		 */
		codecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
		codecContext_->codec_id = AVCodecID(mCodecID);
		codecContext_->bit_rate = mBitRate;
		codecContext_->bit_rate_tolerance = 4000000;
		codecContext_->width = mWidth;
		codecContext_->height = mHeight;
		
		/**
		 *	information about fps
		 */
		codecContext_->time_base.num = 1;
		codecContext_->time_base.den = (int)(0.5 +((float) mNum) / ((float) mDen));
		if(mInterlaced)
		{
			codecContext_->time_base.den /= 2;
		}
		
		/**
		 *	encoding specific details
		 */
		codecContext_->gop_size = 12;
		codecContext_->keyint_min = 25;
		codecContext_->sample_aspect_ratio.num = 1;
		codecContext_->sample_aspect_ratio.den = 1;
		
		codecContext_->b_frame_strategy = 0;
		codecContext_->max_b_frames=0;
		
		codecContext_->pix_fmt = PixelFormat(mPixelFormat);
		codecContext_->rc_max_rate = 0;
		codecContext_->refs = 1;
		
		// Defaults from ffmpeg.c
		codecContext_->qblur = 0.5f;
		codecContext_->qcompress = 0.5f;
		codecContext_->b_quant_offset = 1.25f;
		codecContext_->b_quant_factor = 1.25f;
		codecContext_->i_quant_offset = 0.0f;
		codecContext_->i_quant_factor = -0.71f;
		
		codecContext_->qmax = 31;
		codecContext_->qmin = 2;
		codecContext_->max_qdiff = 3;
		codecContext_->qcompress = 0.5f;
		codecContext_->me_range = 0;
		
		codecContext_->coder_type = 0;

		/**
		 *	based on the values entered above we try to find a codec that is
		 *	able to deal with these values
		 */
		codec_ = avcodec_find_encoder(codecContext_->codec_id);
		if(!codec_)
		{
			std::cerr << "No matching codec found!" << std::endl;
			return false;
		}
		/**
		 *	open the corresponding codec context
		 */
		AVDictionary ** options = NULL;
		if (avcodec_open2(codecContext_, codec_, options) < 0)
		{
			return false;
		}
		
		return true;
	}
	
	AVCodecContext * VideoCodec::getAVCodecContext() const
	{
		return codecContext_;
	}
	
	AVCodec * VideoCodec::getAVCodec() const
	{
		return codec_;
	}
	
	std::string VideoCodec::getCodecInformation() const
	{
		std::string result = "";
		
		result += "name: ";
		result += codec_->long_name;
	
		result += "\nsample_rate: ";
		result += codecContext_->sample_rate;
		
		result += "\nchannels: ";
		result += codecContext_->channels;
		
		result += "\n";
		
		return result;
	}
}

