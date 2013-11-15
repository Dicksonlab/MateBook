/**
 *  @file	VideoFrame.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the VideoFrame class
 *	@date	22.06.09
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include <limits>

#include "VideoFrame.hpp"
#include "VideoFormat.hpp"
#include "VideoCodec.hpp"
#include "VideoStream.hpp"

#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


namespace mw
{
	
	VideoFrame::VideoFrame()
	: frameRGB_(NULL),
	numBytes_(0),
	imageBuffer_(NULL),
	decodedFrame_(NULL),
	decodedNumBytes_(0),
	decodedImageBuffer_(NULL),
	deinterlacedFrame_(NULL),
	deinterlacedNumBytes_(0),
	deinterlacedImageBuffer_(NULL),
	fullSizeFrame_(NULL),
	fullSizeNumBytes_(0),
	fullSizeImageBuffer_(NULL),
	height_(0),
	width_(0),
	pts_(0)
	{
	}
	
	VideoFrame::VideoFrame(int mWidth,
						   int mHeight,
						   unsigned char * mFrameData,
						   unsigned int mPixelFormat,
						   const VideoCodec * mVideoCodec)
	: frameRGB_(NULL),
	numBytes_(0),
	imageBuffer_(NULL),
	decodedFrame_(NULL),
	decodedNumBytes_(0),
	decodedImageBuffer_(NULL),
	deinterlacedFrame_(NULL),
	deinterlacedNumBytes_(0),
	deinterlacedImageBuffer_(NULL),
	fullSizeFrame_(NULL),
	fullSizeNumBytes_(0),
	fullSizeImageBuffer_(NULL),
	height_(mHeight),
	width_(mWidth),
	pts_(0)
	{
		this->prepareFrame(mVideoCodec,
						   width_,
						   height_,
						   mPixelFormat);
		memcpy(imageBuffer_,
			   mFrameData,
			   numBytes_);
	}
	
	VideoFrame::~VideoFrame()
	{
		// rgb frame raw data & additional
		if(imageBuffer_)
		{
			delete imageBuffer_;
			imageBuffer_ = NULL;
		}
		
		if(frameRGB_)
		{
			av_free(frameRGB_);
			frameRGB_ = NULL;
		}
		
		// decoded frame raw data & additional
		if(decodedImageBuffer_)
		{
			delete decodedImageBuffer_;
			decodedImageBuffer_ = NULL;
		}
		
		if(decodedFrame_)
		{
			av_free(decodedFrame_);
			decodedFrame_ = NULL;
		}
		
		// deinterlaced frame raw data & additional
		if(deinterlacedImageBuffer_)
		{
			delete deinterlacedImageBuffer_;
			deinterlacedImageBuffer_ = NULL;
		}
		
		if(deinterlacedFrame_)
		{
			av_free(deinterlacedFrame_);
			deinterlacedFrame_ = NULL;
		}
		
		// fullSize frame
		if(fullSizeImageBuffer_)
		{
			delete fullSizeImageBuffer_;
			fullSizeImageBuffer_ = NULL;
		}
		
		if(fullSizeFrame_)
		{
			av_free(fullSizeFrame_);
			fullSizeFrame_ = NULL;
		}
	}
	
	const AVFrame * VideoFrame::getAVFrame() const
	{
		return frameRGB_;
	}
	
	void VideoFrame::saveFrame(int mNumFrame)
	{
		/**
		 *	saves a single frame uncompressed as a ppm file
		 *	it needs to be stored in RGB for the frame
		 *	to export and display properly in other applications
		 */
		FILE * pFile;
		char szFilename[32];
		
#ifdef WIN32
		sprintf_s(szFilename,
				  "frame%d.ppm",
				  mNumFrame);
		fopen_s(& pFile,
				szFilename,
				"wb");
#else
		sprintf(szFilename,
				"frame%d.ppm",
				mNumFrame);
		pFile = fopen(szFilename,
					  "wb");
#endif
		if(pFile == NULL)
		{
			return;
		}
		
		fprintf(pFile,
				"P6\n%d %d\n255\n",
				width_,
				height_);
		
		for(int y = 0; y < height_; y++)
		{
			fwrite(frameRGB_->data[0] + y * frameRGB_->linesize[0],
				   1,
				   width_ * 3,
				   pFile);
		}
		
		fclose(pFile);
	}
	
	int64_t VideoFrame::getFramePts() const
	{
		return pts_;
	}
	
	int VideoFrame::getNumBytes() const
	{
		return numBytes_;
	}
	
	uint8_t * VideoFrame::getRawBufferPtr()
	{
		return imageBuffer_;
	}
	
	void VideoFrame::convertRGBToPixelFormat(unsigned int mPixelFormat)
	{
		/**
		 *	convenience function for converting from RGB into other formats
		 *	is there for legacy purposes, since we only used to have this call in earlier versions
		 *	of mediawrapper
		 */
		this->convertToPixelFormat(PIX_FMT_RGB24,
								   mPixelFormat);
	}
	
	void VideoFrame::convertToPixelFormat(unsigned int mSourcePixelFormat,
										  unsigned int mTargetPixelFormat)
	{
		/**
		 *	function to convert between pixel formats
		 *	it uses the frame data stored in frameRGB_ and
		 *	replaces frameRGB_ and the buffer with the new data
		 */
		
		/**
		 *	creating a new frame to store the results
		 *	with a separate buffer array too
		 */
		AVFrame * newFrame = avcodec_alloc_frame();
		
		int newSize = avpicture_fill(
									 (AVPicture *) newFrame,
									 NULL,
									 PixelFormat(mTargetPixelFormat),
									 width_,
									 height_
									 );
		uint8_t * newBuffer = new uint8_t[newSize / sizeof(uint8_t)];
		avpicture_fill(
					   (AVPicture *) newFrame,
					   newBuffer,
					   PixelFormat(mTargetPixelFormat),
					   width_,
					   height_
					   );
		
		/**
		 *	creates the "scale" context for the pixel format conversion
		 */
		SwsContext * img_convert_ctx;
		
		img_convert_ctx = sws_getContext(
										 width_,
										 height_,
										 PixelFormat(mSourcePixelFormat),
										 width_,
										 height_,
										 PixelFormat(mTargetPixelFormat),
										 SWS_BICUBIC,
										 NULL,
										 NULL,
										 NULL
										 );
		
		/**
		 *	converts the image
		 */
		sws_scale(
				  img_convert_ctx,
				  frameRGB_->data,
				  frameRGB_->linesize,
				  0,
				  height_,
				  newFrame->data,
				  newFrame->linesize
				  );
		
		/**
		 *	cleans up the data, and swaps the temp image with the final resulting image
		 *	to delete the memory not needed any more
		 */
		sws_freeContext(img_convert_ctx);
		
		if(imageBuffer_)
		{
			delete imageBuffer_;
			imageBuffer_ = NULL;
		}
		imageBuffer_ = newBuffer;
		numBytes_ = newSize;
		
		if(frameRGB_)
		{
			av_free(frameRGB_);
			frameRGB_ = NULL;
		}
		
		frameRGB_ = newFrame;
	}
	
	bool VideoFrame::encodeAndWriteFrame(VideoFormat * mFormat,
										 VideoCodec * mCodec,
										 VideoStream * mStream,
										 unsigned int mBitBufferSize,
										 uint8_t * mBitBuffer)
	{
		/**
		 *	encodes the given video into a buffer
		 */
		int out_size = avcodec_encode_video(mCodec->getAVCodecContext(),
											mBitBuffer,
											mBitBufferSize,
											frameRGB_);
		
		/**
		 *	if the encoding was successfull we have an out_size of a positive number of bytes
		 */
		if (out_size > 0)
		{
			/**
			 *	creating a packet containing the encoded frame
			 */
			AVPacket outpkt;
			av_init_packet(& outpkt);
			
			outpkt.data = mBitBuffer;
			outpkt.size = out_size;
			outpkt.stream_index = 0;
			outpkt.dts = outpkt.pts = mCodec->getAVCodecContext()->coded_frame->pts;
			outpkt.flags |= (mCodec->getAVCodecContext()->coded_frame->key_frame) ? AV_PKT_FLAG_KEY : 0;
			
			/**
			 *	writing the packet to the actual video file
			 */
			if (av_write_frame (mFormat->getAVFormatContext(),
								& outpkt) < 0)
			{
				return false;
			}
		}
		else if(out_size < 0)
		{
			/**
			 *	we couldn't encode, thus something is wrong
			 */
			return false;
		}
		
		return true;
	}
	
#define USE_FFMPEG_DEINTERLACE
	
	void VideoFrame::convertFrame(AVFrame * & mTempFrame,
								  uint8_t * & mTempBuffer,
								  const VideoCodec * mCodec,
								  SwsContext * mScaleContext,
								  SwsContext * mColorConversionContext)
	{
		/**
		 *	taking a freshly encoded frame to the necessary steps until we have them in full size
		 *	and rgb
		 */
		
		/**
		 *	we start of with deinterlacing the frame, if necessary.
		 *	assuming we don't have to deinterlace we directly scale the TempFrame
		 */
		AVFrame * toScaleFrame = mTempFrame;
		uint8_t * toScaleData = mTempBuffer;
#if defined(USE_FFMPEG_DEINTERLACE)
		if(mTempFrame->interlaced_frame)
		{
			/**
			 *	in case we deinterlace the TempFrame, we store it in deinterlaceFrame_
			 *	this is now the one we use for scaling
			 */
			avpicture_deinterlace(
								  (AVPicture *) deinterlacedFrame_,
								  (AVPicture *) mTempFrame,
								  mCodec->getAVCodecContext()->pix_fmt,
								  mCodec->getAVCodecContext()->width,
								  mCodec->getAVCodecContext()->height
								  );
			
			toScaleFrame = deinterlacedFrame_;
			toScaleData = deinterlacedImageBuffer_;
			
		}
#endif
		
		/**
		 *	we scale our frame, in case the width and height are not the desired ones
		 */
		if(mCodec->getAVCodecContext()->width != width_ ||
		   mCodec->getAVCodecContext()->height != height_)
		{
			sws_scale(
					  mScaleContext,
					  toScaleFrame->data,
					  toScaleFrame->linesize,
					  0,
					  height_,
					  fullSizeFrame_->data,
					  fullSizeFrame_->linesize
					  );
			
			toScaleFrame = fullSizeFrame_;
			toScaleData = fullSizeImageBuffer_;
		}
		
		/**
		 *	we do the color conversion here last, 
		 *	since this is the order that seems to be fastest
		 */
		sws_scale(
				  mColorConversionContext,
				  toScaleFrame->data,
				  toScaleFrame->linesize,
				  0,
				  height_,
				  frameRGB_->data,
				  frameRGB_->linesize
				  );
		
#if !defined(USE_FFMPEG_DEINTERLACE)
		if(mTempFrame->interlaced_frame)
		{
			this->deinterlaceSkipField();
		}
#endif
	}
	
	void VideoFrame::getFrameStats(const VideoFormat * mFormat,
								   const VideoCodec * mCodec,
								   bool * mInterlaced,
								   int * mFramesBeforeKey,
								   float * mFps,
								   int * mGop,
								   unsigned int mStreamIndex/* = 0*/)
	{
		/**
		 *	preparing the values needed for calculating our frame stats
		 */
		*mFramesBeforeKey = 0;
		bool framesBeforeKeyDone = false;
		size_t keyFrameCount = 0;
		size_t betweenKeyFrames = 0;
		size_t gop_size = 0;
		
		double lastPts = 0;
		double difference = 0;
		int numFrames = 0;
		bool firstframe = true;
		double firstPts = 0;
		
		/**
		 *	we prepare the frame. this means, allocating the memory, if not already done
		 */
		this->prepareFrame(mCodec);
		
		/**
		 *	temporary packet to store the decoded data
		 */
		int frameFinished;
		AVPacket packet;
		
		/**
		 *	we read packets out of the video
		 */
		while(av_read_frame(mFormat->getAVFormatContext(),
							& packet) >= 0)
		{
			/**
			 *	if the packet matches the stream we are interested in
			 */
			if(packet.stream_index == (int)mStreamIndex)
			{
				global_video_pkt_pts = packet.pts;
				
				/**
				 *	we decode the packet into (part of) a frame
				 */
				avcodec_decode_video2(
									  mCodec->getAVCodecContext(),
									  decodedFrame_,
									  & frameFinished,
									  & packet
									  );
				
				/**
				 * in case we are done with one frame
				 */
				if(frameFinished)
				{
					/**
					 *	increment the frame counter, to make sure we know how many frames
					 *	are between keyframes. Sometimes the number reported by the frames and the gop size from
					 *	the video codec doesn't match
					 */
					betweenKeyFrames++;
					
					/**
					 *	calculation of the frame pts
					 */
					if(packet.dts == AV_NOPTS_VALUE &&
					   decodedFrame_->opaque &&
					   *(uint64_t*)decodedFrame_->opaque != AV_NOPTS_VALUE)
					{
						pts_ = *(uint64_t *)decodedFrame_->opaque;
					}
					else if(packet.dts != AV_NOPTS_VALUE)
					{
						pts_ = packet.dts;
					}
					else
					{
						pts_ = 0;
					}
					pts_ *= av_q2d(mFormat->getAVFormatContext()->streams[mStreamIndex]->time_base);
					
					/**
					 *	in case it is our first frame decoded we want to know what it's pts is.
					 *	if a recording was split into multiple videos by the camera, the first frame
					 *	can have a pts != 0, since the video is part of a series of videos
					 */
					if(firstframe)
					{
						firstframe = false;
						firstPts = pts_;
						lastPts = pts_;
						
						const_cast<VideoFormat *>(mFormat)->setFirstFramePts(pts_);
					}
					
					/**
					 *	calculating the pts difference, to get an estimate of the fps within the video
					 *	sometimes it is not the one reported
					 */
					difference += pts_ - lastPts;
					lastPts = pts_;
					
					/**
					 *	once we encounter a frame to deinterlace we set the flag
					 */
					if(decodedFrame_->interlaced_frame)
					{
						*mInterlaced = true;
					}
					/**
					 *	if we passed a key frame we know we have at least found the first key frame
					 *	and know how many frames in the video are before the first keyframe.
					 */
					if(decodedFrame_->key_frame)
					{
						framesBeforeKeyDone = true;
						keyFrameCount ++;
						gop_size = betweenKeyFrames;
						betweenKeyFrames = 0;
						
					}
					/**
					 * if we haven't found a keyframe yet, we increas the number of frames before the first key frame
					 */
					if(!framesBeforeKeyDone)
					{
						
						(*mFramesBeforeKey)++;
					}
					
					/**
					 *	once we decoded 2 seconds of video, we average the fps from the first two seconds to
					 *	set the videos fps
					 */
					numFrames++;
					if(((pts_ - firstPts) >= 2.0))
					{
						difference /= (numFrames - 1);
						*mFps = 1.0f / difference;
						
						/**
						 *	resetting the video to the start
						 */
						avcodec_flush_buffers(mCodec->getAVCodecContext());
						if(mCodec->getAVCodec()->id == CODEC_ID_H264)
						{
							if(av_seek_frame(mFormat->getAVFormatContext(),
											 -1,
											 0,
											 AVSEEK_FLAG_BACKWARD) < 0)
							{
								std::cerr << "Resetting Video failed!" << std::endl;
							}
						}
						else
						{
							if(av_seek_frame(mFormat->getAVFormatContext(),
											 -1,
											 0,
											 AVSEEK_FLAG_ANY) < 0)
							{
								std::cerr << "Resetting Video failed!" << std::endl;
							}
						}
						*mGop = gop_size;
						
						/**
						 *	cleaning packet
						 */
						av_free_packet(& packet);
						
						return;
					}
				}
			}
			/**
			 *	cleaning packet
			 */
			av_free_packet(& packet);
		}
		/**
		 *	cleaning packet
		 */
		av_free_packet(& packet);
		
		std::cerr << "Reached End of Stream!" << std::endl;
	}
	
	//#define DEBUG_SEEK
	
	bool VideoFrame::loadFrame(const VideoFormat * mFormat,
							   const VideoCodec * mCodec,
							   SwsContext * mScaleContext,
							   SwsContext * mColorConversionContext,
							   int64_t mCurFrameNumber,
							   int64_t mTargetFrameNumber,
							   unsigned int mStreamIndex/* = 0*/,
							   bool mConvert/* = true*/,
							   bool mLoadKeyFrame/* = false*/)
	{
		/**
		 *	calculating how many frames we need to read, given the frame number where the video was seeked to
		 *	and the desired frame
		 */
		int frameDiff = mTargetFrameNumber - mCurFrameNumber + 1;
		
		/**
		 *	the pts of the needed frame
		 */
		float targetPtsSeconds = (((float) mTargetFrameNumber) / mFormat->getFps() +
								  mFormat->getFirstFramePts() -
								  1.0 / (10 * mFormat->getFps()));
		
		/**
		 *	testing if we plainly read the next finished frame or if more than one
		 *	frame needs to be decoded
		 */
		bool seekAndRead = mTargetFrameNumber != mCurFrameNumber;
		
		int readFrames = 0;
		bool passedKeyFrame = false;
		
		/**
		 *	preparing the frame with the given codec. if this has been done alreay, 
		 *	nothing will be done in here
		 */
		this->prepareFrame(mCodec);
		
		int frameFinished;
		AVPacket packet;
		
#ifdef DEBUG_SEEK
		std::cerr.precision(8);
		std::cerr << std::endl << "loadFrame: targetNumber: " << mTargetFrameNumber << " start read for targetPtsSeconds : " << std::fixed << targetPtsSeconds << "; " ;
#endif
		/**
		 *	we read the next frame into our temporary packet
		 */
		while(av_read_frame(mFormat->getAVFormatContext(),
							& packet) >= 0)
		{
			/**
			 *	check if the packet is from the stream we are interested in
			 */
			if(packet.stream_index == (int)mStreamIndex)
			{
				global_video_pkt_pts = packet.pts;
				
				/**
				 *	decoding the video
				 */
				avcodec_decode_video2(
									  mCodec->getAVCodecContext(),
									  decodedFrame_,
									  & frameFinished,
									  & packet
									  );
				
				/**
				 *	we have decoded a full frame
				 */
				if(frameFinished)
				{
					/**
					 *	adjusting the pts of the frame. this works together with our two
					 *	custom decoding functions defined in the Codec class.
					 *	the code comes from somewhere on the ffmpeg mailing list
					 */
					if(packet.dts == AV_NOPTS_VALUE &&
					   decodedFrame_->opaque &&
					   *(uint64_t*)decodedFrame_->opaque != AV_NOPTS_VALUE)
					{
						pts_ = *(uint64_t *)decodedFrame_->opaque;
					}
					else if(packet.dts != AV_NOPTS_VALUE)
					{
						pts_ = packet.dts;
					}
					else
					{
						pts_ = 0;
					}
					pts_ *= av_q2d(mFormat->getAVFormatContext()->streams[mStreamIndex]->time_base);
					
#ifdef DEBUG_SEEK
					std::cerr << "read " << std::fixed << pts_ << " ";
#endif
					
					/**
					 *	in case we need to read more than one frame
					 */
					if(seekAndRead)
					{
						/**
						 *	checking if the frame we read is a keyframe
						 *	we count our read frames from this keyframe on
						 */
						if(decodedFrame_->key_frame)
						{
							passedKeyFrame = true;
#ifdef DEBUG_SEEK
							std::cerr << "keyframe ";
#endif
						}
						if(passedKeyFrame)
						{
							readFrames++;
						}
						
						/**
						 *	checking the pts of our read frame
						 *	we assume we start with a pts smaller than the one to read
						 *	so if the current one is bigger, than we have our frame.
						 */
						if(pts_ >= targetPtsSeconds)
						{
							/**
							 *	in case we need to do color conversion or scaling,
							 *	so probably all the time
							 */
							if(mConvert)
							{
								/**
								 *	converting our read frame into the desired size and pixelformat
								 */
								this->convertFrame(decodedFrame_,
												   decodedImageBuffer_,
												   mCodec,
												   mScaleContext,
												   mColorConversionContext);
							}
							
							/**
							 *	we are done, so we need to clean up
							 */
							av_free_packet(& packet);
#ifdef DEBUG_SEEK
							std::cout << "Frame done" << std::endl;
							std::cerr << std::fixed << pts_ << std::endl;
#endif
							return true;
						}
					}
					else
					{
#ifdef DEBUG_SEEK
						std::cout << "Not using seekAndRead" << std::endl;
#endif
						if(decodedFrame_->key_frame)
						{
							passedKeyFrame = true;
						}
						/**
						 *	we are done reading
						 *	in terms of time
						 */
						if(pts_ >= targetPtsSeconds)
						{
							/**
							 *	in case we only want a keyframe, we check if we actually read a keyframe
							 */
							if(!mLoadKeyFrame ||
							   (mLoadKeyFrame && decodedFrame_->key_frame))
							{
								/**
								 *	in case we need to do color conversion or scaling,
								 *	so probably all the time
								 */
								if(mConvert)
								{
									/**
									 *	converting our read frame into the desired size and pixelformat
									 */
									this->convertFrame(decodedFrame_,
													   decodedImageBuffer_,
													   mCodec,
													   mScaleContext,
													   mColorConversionContext);
								}
								/**
								 *	we are done, so we need to clean up
								 */
								av_free_packet(& packet);
								
								return true;
							}
						}
					}
					
				}
			}
			/**
			 *	we are done, so we need to clean up
			 */
			av_free_packet(& packet);
		}
		/**
		 *	we are done, so we need to clean up
		 */
		av_free_packet(& packet);
		
		std::cerr << "Reached End of Stream!" << std::endl;
		return false;
		
		
	}
	
	void VideoFrame::prepareFrame(const VideoCodec * mCodec,
								  int mWidth/* = 0*/,
								  int mHeight/* = 0*/,
								  unsigned int mPixelFormat/* = PIX_FMT_RGB24*/)
	{
		/**
		 *	if frameRGB_ is not yet initialised
		 */
		if(!frameRGB_)
		{
			/**
			 *	we calculate the resulting width again of our frame based on the pixel aspect ratio
			 *	frameRGB_ will always store a 1:1 pixel aspect ratio image in RGB
			 */
			if((mCodec->getAVCodecContext()->sample_aspect_ratio.num != 0) &&
			   (mCodec->getAVCodecContext()->sample_aspect_ratio.den != 0))
			{
				int aspect1, aspect2;
				
				av_reduce(& aspect1, & aspect2,
						  mCodec->getAVCodecContext()->width * mCodec->getAVCodecContext()->sample_aspect_ratio.num,
						  mCodec->getAVCodecContext()->height* mCodec->getAVCodecContext()->sample_aspect_ratio.den,
						  1024 * 1024);
				
				height_ = mCodec->getAVCodecContext()->height;
				width_ = mCodec->getAVCodecContext()->height * aspect1 / aspect2;
			}
			else
			{
				height_ = mCodec->getAVCodecContext()->height;
				width_ = mCodec->getAVCodecContext()->width;
			}
			if(mWidth > 0)
			{
				width_ = mWidth;
			}
			if(mHeight > 0)
			{
				height_ = mHeight;
			}
			
			/**
			 *	allocate the frame and the memory to hold the data
			 */
			frameRGB_ = avcodec_alloc_frame();
			numBytes_ = avpicture_get_size(
										   PixelFormat(mPixelFormat),
										   width_,
										   height_
										   );
			
			imageBuffer_ = new uint8_t[numBytes_ / sizeof(uint8_t)];
			
			/**
			 *	sets up the AVFrame to use the image buffer for storage
			 */
			avpicture_fill(
						   (AVPicture *) frameRGB_,
						   imageBuffer_,
						   PixelFormat(mPixelFormat),
						   width_,
						   height_
						   );
		}
		
		if(!decodedFrame_)
		{
			/**
			 *	the frame that holds the image in decoded form, both with the decoded
			 *	pixel format and pixel aspect ratio
			 */
			decodedFrame_ = avcodec_alloc_frame();
			decodedNumBytes_ = avpicture_fill(
											  (AVPicture *) decodedFrame_,
											  NULL,
											  mCodec->getAVCodecContext()->pix_fmt,
											  mCodec->getAVCodecContext()->width,
											  mCodec->getAVCodecContext()->height
											  );
			decodedImageBuffer_ = new uint8_t[decodedNumBytes_ / sizeof(uint8_t)];
			avpicture_fill(
						   (AVPicture *) decodedFrame_,
						   decodedImageBuffer_,
						   mCodec->getAVCodecContext()->pix_fmt,
						   mCodec->getAVCodecContext()->width,
						   mCodec->getAVCodecContext()->height
						   );
		}
		
		if(!deinterlacedFrame_)
		{
			/**
			 *	holds the deinterlaced frame
			 *	it is the same as the decodedFrame_, just in deinterlaced form, in case decodedFrame_
			 *	stores a interlaced image
			 */
			deinterlacedFrame_ = avcodec_alloc_frame();
			
			deinterlacedNumBytes_ = avpicture_fill(
												   (AVPicture *) deinterlacedFrame_,
												   NULL,
												   mCodec->getAVCodecContext()->pix_fmt,
												   mCodec->getAVCodecContext()->width,
												   mCodec->getAVCodecContext()->height
												   );
			deinterlacedImageBuffer_ = new uint8_t[deinterlacedNumBytes_ / sizeof(uint8_t)];
			avpicture_fill(
						   (AVPicture *) deinterlacedFrame_,
						   deinterlacedImageBuffer_,
						   mCodec->getAVCodecContext()->pix_fmt,
						   mCodec->getAVCodecContext()->width,
						   mCodec->getAVCodecContext()->height
						   );
		}
		
		if(!fullSizeFrame_)
		{
			/**
			 *	this stores the full size image, but still in the original pixel format
			 */
			fullSizeFrame_ = avcodec_alloc_frame();
			
			fullSizeNumBytes_ = avpicture_fill(
											   (AVPicture *) fullSizeFrame_,
											   NULL,
											   mCodec->getAVCodecContext()->pix_fmt,
											   width_,
											   height_
											   );
			fullSizeImageBuffer_ = new uint8_t[fullSizeNumBytes_ / sizeof(uint8_t)];
			avpicture_fill(
						   (AVPicture *) fullSizeFrame_,
						   fullSizeImageBuffer_,
						   mCodec->getAVCodecContext()->pix_fmt,
						   width_,
						   height_
						   );
		}
	}
	
	void VideoFrame::deinterlaceSkipField()
	{
		/**
		 *	this routine can be activated in the convertframe method.
		 *	it does deinterlacing by duplicating every nth line into the (n+1)th line
		 *	ffmpeg does a faster and better job at it though (now)
		 */
		for (int i = 0; i < height_; i +=2) {
			memcpy(
				   (void *)(imageBuffer_ + (i * width_ * 3)),
				   (void *)(imageBuffer_ + ((i + 1) * width_ * 3)),
				   width_ * 3
				   );
		}
	}
}
