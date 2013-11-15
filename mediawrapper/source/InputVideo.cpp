/**
 *  @file	InputVideo.cpp
 *	@author	Herbert Grasberger
 *  @brief	contains the implementation for the InputVideo class
 *	@date	9.05.10
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#endif
#endif

#include "InputVideo.hpp"

#include "VideoFormat.hpp"
#include "VideoCodec.hpp"
#include "VideoFrame.hpp"

#include "OutputVideo.hpp"

#include <iostream>
#include <cmath>
#include <stdexcept>
#include <limits>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace mw
{
	InputVideo::InputVideo(std::string mFileName) :
	fileName_(mFileName),
	videoFormat_(NULL),
	videoCodec_(NULL),
	numFrames_(0),
	fps_(0),
	width_(0),
	height_(0),
	gopSize_(0),
	curFrame_(0),
	lastFrame_(0),
	audioInfos_(),
	interlaced_(false),
	firstStream_(0),
	seeked_(true),
	framesBeforeKey_(0),
	loadNextKeyFrame_(false),
	audioStreamIndices_(),
	scaleContext(NULL),
	colorConvertContext(NULL),
	cFormat(PIX_FMT_RGB24),
	frame(NULL)
	{
		/**
		 *	this part of the code is there to check if the filename passed as a parameter
		 *	is actually existent in the file system. The behaviour for ffmpeg for non-existing
		 *	files is somewhat ill defined, thus we manually check first
		 */
		FILE * file = NULL;
#ifdef WIN32
		fopen_s(& file, fileName_.c_str(), "r");
#else
		file = fopen(fileName_.c_str(), "r");
#endif
		if (!(file)) {
			throw std::runtime_error("File not present.");
		}
		fclose(file);
		file = NULL;
		
		/**
		 *	now we know that the file is there, and we can try loading it into ffmpeg
		 *	the loadAVFormat call checks if it is a ffmpeg readable (video/audio) file
		 */
		videoFormat_ = new VideoFormat();
		if (!videoFormat_->loadAVFormatContext(fileName_)) {
			throw std::runtime_error("Failed to load video.");
		}
		
		/**
		 *	videos can have several video streams. We always take the first one found
		 */
		firstStream_ = videoFormat_->getStreamIndexForVideoStream(0);
		
		/**
		 *	if the index is not -1, we have found a video stream and can start 
		 *	loading the information for the stream
		 */
		if (firstStream_ != (unsigned int) -1)
		{
			
			/**
			 *	we load the codec for the given stream
			 */
			videoCodec_ = new VideoCodec();
			videoCodec_->loadCodec(videoFormat_, firstStream_);
			
			/**
			 *	in some videos, a pixel is not square but a rectangle.
			 *	we look for this, and if needed adjust the output size of the video
			 *	usually this happens when not the whole width of the video needs to be stored,
			 *	but a squashed video is stored instead, that needs to be streched again for display
			 */
			if((videoCodec_->getAVCodecContext()->sample_aspect_ratio.num != 0) &&
			   (videoCodec_->getAVCodecContext()->sample_aspect_ratio.den != 0))
			{
				int aspect1, aspect2;
				
				av_reduce(& aspect1, & aspect2,
						  videoCodec_->getAVCodecContext()->width * videoCodec_->getAVCodecContext()->sample_aspect_ratio.num,
						  videoCodec_->getAVCodecContext()->height* videoCodec_->getAVCodecContext()->sample_aspect_ratio.den,
						  1024 * 1024);
				
				height_ = videoCodec_->getAVCodecContext()->height;
				width_ = videoCodec_->getAVCodecContext()->height * aspect1 / aspect2;
			}
			else
			{
				height_ = videoCodec_->getAVCodecContext()->height;
				width_ = videoCodec_->getAVCodecContext()->width;
			}
			
			curFrame_ = 0;
			
			/**
			 * the frame used for reading is created
			 */
			frame = new VideoFrame();
			/**
			 *	we get some information on the video, eg. if it is interlaced, how many frames before the first keyfram
			 *	its fps, ...
			 */
			frame->getFrameStats(videoFormat_, videoCodec_, & interlaced_, & framesBeforeKey_, & fps_, & gopSize_, firstStream_);
			
			/**
			 *	for some reason the gop_size reported in the AVContext is actually not the same value
			 *	than the number of frames between two keyframes.
			 *	previously, with an older version of ffmpeg, we needed to use the number manually calculated
			 *	however, it seems to work now with the reported number as well
			 *	hg, Aug 24, 2012
			 */
			gopSize_ = videoCodec_->getAVCodecContext()->gop_size;
			
			/**
			 *	we set the fps value in our video format class
			 */
			videoFormat_->setFps(fps_);
			
			/**
			 * this conditial checks if the start offset of a video is actually bigger than the end.
			 * there were some videos that were split by the camera and had "wrong" start offsets when only one
			 * video was taken into account, since the offset was calculated from the first video in the split series.
			 */
			int64_t start;
			if(videoFormat_->getAVFormatContext()->start_time > (videoFormat_->getAVFormatContext()->duration / 2))
			{
				start = 0;
			}
			else
			{
				start = videoFormat_->getAVFormatContext()->start_time;
			}
			
			/**
			 *	based on the information we have, we calculate the number of frames in the video. In some cases this can be wrong
			 *	when the wrong fps value is reported
			 */
			numFrames_ = (int64_t) std::floor((videoFormat_->getAVFormatContext()->duration - start) * fps_ / AV_TIME_BASE + 0.5f);
		}
		
		/**
		 *	we also parse audio information on the given file
		 *	every audio information we find, we store in our list, that we can then use
		 *	later on to read the whole audio into an array
		 */
		audioStreamIndices_ = videoFormat_->getStreamIndicesForAudioStreams();
		
		for(size_t i = 0; i < audioStreamIndices_.size(); ++i)
		{
			audioInfo curInfo;
			curInfo.audioCodec_ = new VideoCodec();
			curInfo.audioCodec_->loadCodec(videoFormat_, audioStreamIndices_[i]);
			
			curInfo.numChannels_ = curInfo.audioCodec_->getAVCodecContext()->channels;
			curInfo.sampleRate_ = curInfo.audioCodec_->getAVCodecContext()->sample_rate;
			
			curInfo.sampleCount_ = videoFormat_->getAVFormatContext()->streams[audioStreamIndices_[i]]->duration;
			
			audioInfos_.push_back(curInfo);
		}
	}
	
	VideoFormat * InputVideo::getVideoFormat() const
	{
		return videoFormat_;
	}
	
	VideoCodec * InputVideo::getVideoCodec() const
	{
		return videoCodec_;
	}
	
	std::string InputVideo::getVideoInformation() const
	{
		std::string result;
		
		result += videoCodec_->getCodecInformation();
		
		return result;
	}
	
	int64_t InputVideo::getNumberOfFrames()
	{
		return numFrames_;
	}
	
	float InputVideo::getFrameRate()
	{
		return fps_;
	}
	
	int InputVideo::getFrameWidth()
	{
		return width_;
	}
	
	int InputVideo::getFrameHeight()
	{
		return height_;
	}
	
	int InputVideo::getGopSize()
	{
		return gopSize_;
	}
	
	unsigned int InputVideo::getCurrentFrameNumber()
	{
		return lastFrame_;
	}
	
	InputVideo::~InputVideo()
	{
		if(frame)
		{
			delete frame;
			frame = NULL;
		}
		if(scaleContext)
		{
			sws_freeContext(scaleContext);
			scaleContext = NULL;
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
	
	bool InputVideo::seek(int64_t mFrameNumber)
	{
		bool result;
		/**
		 *	we mark our seeked_ flag as true and also since we seeked to an exact frame
		 *	we have to make sure we load the frame, and not just the next keyframe.
		 *	use seekApprox if you want to get only the keyframe
		 */
		seeked_ = true;
		loadNextKeyFrame_ = false;
		
		/**
		 *	sometimes the first frame in a video is not a key frame, thus we have to
		 *	calculate the offset
		 */
		int64_t frameRelativeToFirstKey = mFrameNumber - framesBeforeKey_;
		
		/**
		 *	in case we want to only read the next frame, we are done, since this is
		 *	the default behaviour once a frame is loaded.
		 */
		if(mFrameNumber == curFrame_ + 1)
		{
			curFrame_ = mFrameNumber;
			result = true;
		}
		else
		{
			/**
			 *	we need to find the closest keyframe to the frame we actually want.
			 *	in seekToKeyFrame the according timestamp in stream base units is calculated based on the fps information.
			 *	HOWEVER: when the timestamp calclulated i seekToKeyFrame is compared with the timestam of the frame read immediately after
			 *	the timestamp of the frame is actually higher
			 */
			curFrame_ = frameRelativeToFirstKey - std::max((frameRelativeToFirstKey % gopSize_), (int64_t)0) + framesBeforeKey_;
			
			if(curFrame_ < 0)
				curFrame_ = 0;
			
			/**
			 *	we seek to the next keyframe and when actually reading, all the necessary frames will be decoded
			 *	until the actual desired frame is read
			 */
			result = this->seekToKeyFrame(curFrame_);
		}
		
		lastFrame_ = mFrameNumber;
		return result;
	}
	
	bool InputVideo::seekApprox(int64_t mFrameNumber)
	{
		if(mFrameNumber > numFrames_)
		{
			return false;
		}
		/**
		 *	this time we mark both, seeked_ and loadNextKeyFrame_ as true
		 *	since the next frame we are interested has to be the keyframe before the given
		 *	frame number
		 */
		loadNextKeyFrame_ = true;
		seeked_ = true;
		/**
		 *	sometimes the first frame in a video is not a key frame, thus we have to
		 *	calculate the offset
		 */
		int64_t frameRelativeToFirstKey = mFrameNumber - framesBeforeKey_;
		
		/**
		 *	we caluclate the frame number of the keyframe before mFrameNumber
		 */
		curFrame_ = lastFrame_ = frameRelativeToFirstKey - std::max((frameRelativeToFirstKey % gopSize_), (int64_t)0) + framesBeforeKey_;
		if(curFrame_ < 0)
			curFrame_ = lastFrame_ = 0;
		/**
		 *	we seek to the next keyframe and when reading, this is the frame returned.
		 */
		return this->seekToKeyFrame(lastFrame_);
	}
	
	VideoFrame * InputVideo::getVideoFrame(colorFormat mColorFormat)
	{
		/**
		 *	in case seek or seekApprox hasn't been called before, e.g. when a 
		 *	frame needed to be read twice in a row, then we seek back to the last read frame
		 */
		if(!seeked_)
		{
			this->seek(curFrame_);
		}
		
		/**
		 *	in most cases we will need a context to scale our images into the right size.
		 *	if the source and destination scale is the same, the scale context will deal with
		 *	not scaling
		 */
		if(!scaleContext)
		{
			/**
			 *	the scale context is created. It is only there to rescale, but doesn't do color conversion
			 *	it turned out, that splitting scale and color conversion increased performance a lot
			 */
			scaleContext = sws_getCachedContext(scaleContext,
												videoCodec_->getAVCodecContext()->width,
												videoCodec_->getAVCodecContext()->height,
												videoCodec_->getAVCodecContext()->pix_fmt,
												width_,
												height_,
												videoCodec_->getAVCodecContext()->pix_fmt,
												SWS_BICUBIC,
												NULL,
												NULL,
												NULL
												);
		}
		
		/**
		 *	similar to the scale, we have a color conversion context
		 */
		if(!colorConvertContext)
		{
			/**
			 *	this context converts from the videos original color format to the one desired by the user
			 */
			colorConvertContext = sws_getCachedContext(colorConvertContext,
													   width_,
													   height_,
													   videoCodec_->getAVCodecContext()->pix_fmt,
													   width_,
													   height_,
													   PixelFormat(mColorFormat),
													   SWS_BICUBIC,
													   NULL,
													   NULL,
													   NULL
													   );
			
			cFormat = mColorFormat;
		}
		else if(mColorFormat != cFormat)
		{
			/**
			 *	in case another color format is required, than the one currently in use
			 *	we delete the old context and create a new one
			 */
			sws_freeContext(colorConvertContext);
			colorConvertContext = sws_getCachedContext(colorConvertContext,
													   width_,
													   height_,
													   videoCodec_->getAVCodecContext()->pix_fmt,
													   width_,
													   height_,
													   PixelFormat(mColorFormat),
													   SWS_BICUBIC,
													   NULL,
													   NULL,
													   NULL
													   );
			
			cFormat = mColorFormat;
		}
		
		/**
		 *	we load the next frame in the video. if the loading failed, we return NULL
		 */
		if(frame && !frame->loadFrame(videoFormat_,
									  videoCodec_,
									  scaleContext,
									  colorConvertContext,
									  curFrame_,
									  lastFrame_,
									  firstStream_,
									  true,
									  loadNextKeyFrame_))
		{
			return NULL;
		}
		curFrame_ = lastFrame_;
		seeked_ = false;
		return frame;
	}
	
	uint8_t* InputVideo::getFrameBuffer(colorFormat mColorFormat)
	{
		/**
		 *	combined method that loads the next frame and returns its buffer
		 *	since getVideoFrame returns a pointer to the class member stored frame
		 *	this does not produce a memory leak
		 */
		if (VideoFrame* videoFrame = getVideoFrame(mColorFormat)) {
			return videoFrame->getRawBufferPtr();
		}
		return NULL;
	}
	
	int InputVideo::getAudioStreamCount()
	{
		return (int) audioStreamIndices_.size();
	}
	
	int InputVideo::getAudioChannelCount(unsigned int mStreamNumber)
	{
		return audioInfos_[mStreamNumber].numChannels_;
	}
	
	int InputVideo::getSampleRate(unsigned int mStreamNumber)
	{
		return audioInfos_[mStreamNumber].sampleRate_;
	}
	
	int InputVideo::getSampleCount(unsigned int mStreamNumber)
	{
		return audioInfos_[mStreamNumber].sampleCount_;
	}
	
	
	bool InputVideo::getAudioData(unsigned int mStreamNumber, unsigned int mChannel, std::vector<int16_t> & mSamples)
	{
		/**
		 *	create the temporary AVPacket, that we will be using to read the audio data into
		 *	piece by piece
		 */
		AVPacket packet;
		int len1;
		int data_size = audioInfos_[mStreamNumber].sampleCount_;
		/**
		 *	allocate enough memory for the whole audio data
		 */
		mSamples.resize(data_size, 0);
		
		size_t buffer_index = 0;
		
		/**
		 *	temporary audio buffer
		 */
		int bufferSize = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2 * sizeof(int16_t);
		int16_t * audioBuffer = (int16_t *)av_malloc(bufferSize);
		
		size_t allvals = 0;
		
		/**
		 *	similar loop as readin video frames
		 *	we loop over every packet in the video
		 */
		while(av_read_frame(videoFormat_->getAVFormatContext(), & packet) >= 0)
		{
			/**
			 *	if our packet is part of the audio stream we are interested in
			 *	we actually have to do work
			 */
			if(packet.stream_index == audioStreamIndices_[mStreamNumber])
			{
				/**
				 *	given our packet actually stores information
				 */
				while (packet.size > 0)
				{
					/**
					 *	we decode the compressed audio in our packet into our temporary audioBuffer
					 */
					bufferSize = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;
					len1 = avcodec_decode_audio3(audioInfos_[mStreamNumber].audioCodec_->getAVCodecContext(),
												 audioBuffer,
												 & bufferSize,
												 & packet);
					if(len1 > 0)
					{
						size_t values = bufferSize / sizeof(int16_t);
						values /= audioInfos_[mStreamNumber].numChannels_;
						
						/**
						 *	then we copy the audio data into our final array
						 */
						if(data_size - values > 0)
						{
							/**
							 *	we are only interested in a given channel, so we only copy that one
							 *	the other channels are skipped
							 */
							for(size_t i = 0; i < values; i += audioInfos_[mStreamNumber].numChannels_)
							{
								mSamples[buffer_index++] = audioBuffer[i + mChannel];
							}
							data_size -= values;
						}
						else
						{
							std::cerr << "Still reading but no memory left " << std::endl;
						}
						allvals += values;
						
					}
					else
					{
						/**
						 *	early exit.
						 *	need to free the packet
						 */
						std::cerr << "Something went wrong" << std::endl;
						av_free_packet(& packet);
						return false;
					}
					packet.size -= len1;
					packet.data += len1;
				}
				
				
			}
			else
			{
				av_free_packet(& packet);
			}
			
		}
		/**
		 *	free the packet and the temp buffer
		 */
		av_free_packet(& packet);
		av_free(audioBuffer);
		
		return true;
	}
	
	unsigned int InputVideo::getAudioData(unsigned int streamNumber, unsigned int channelNumber, float* data, unsigned int length)
	{
		/**
		 *	this method reads and converts the audio data into float values.
		 *	the memory for the float audio is already pre-allocated
		 */
		
		/**
		 *	we create the temporary int16_t buffer and read our audio into it
		 */
		std::vector<int16_t> samples;
		getAudioData(streamNumber, channelNumber, samples);
		unsigned int returnedLength = (length < samples.size()) ? length : samples.size();
		
		/**
		 *	after we read our audio information, we convert it to float and copy it into the desired memory location
		 */
		for (size_t i = 0; i < returnedLength; ++i) {
			data[i] = (float) samples[i] / std::numeric_limits<int16_t>::max();
		}
		return returnedLength;
	}
	
	bool InputVideo::seekToKeyFrame(int64_t mFrameNumber)
	{
		/**
		 *	we need to calculate the time stamp for our given frame number.
		 *	since we are seeking we have to reset global_video_pkt_pts, since otherwise
		 *	the pts of the next read frame is wrong.
		 */
		global_video_pkt_pts = AV_NOPTS_VALUE;
		int64_t timestamp = videoFormat_->frameNumberToTimeStamp(mFrameNumber,
																 interlaced_,
																 firstStream_);
		
		/**
		 *	depending on the given codec of the video, we need separate flags for seeking
		 *	and seeking to the start of the video. if this isn't in there, seeking to 0 with MTS files
		 *	didn't work properly
		 */
		int result;
		int flags;
		if(videoCodec_->getAVCodec()->id == CODEC_ID_H264)
		{
			flags = AVSEEK_FLAG_BACKWARD;
		}
		else
		{
			flags = (timestamp == 0) ? AVSEEK_FLAG_FRAME : AVSEEK_FLAG_BACKWARD;
		}
		
		/**
		 *	seek to the closest time key frame for the given time stamp
		 */
		result = av_seek_frame(videoFormat_->getAVFormatContext(),
							   firstStream_,
							   timestamp,
							   flags);
		
		/**
		 *	flushes the buffers, so we don't get any decoding artefacts with the new frames
		 */
		avcodec_flush_buffers(videoCodec_->getAVCodecContext());
		
		if(result < 0)
		{
			std::cerr << "Reached End of Stream!" << std::endl;
			return false;
		}
		
		return true;
	}
}
