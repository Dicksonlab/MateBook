/**
 *  @file	InputVideo.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the InputVideo class
 *	@date	9.05.10
 */

#ifndef __input_video_h
#define __input_video_h

#include "Video.hpp"
#include "ColorFormat.hpp"

#include <vector>
#include <string>

struct SwsContext;

namespace mw
{
	/**
	 *	@class	InputVideo
	 *	@brief	class handling an input video derived from Video
	 */
	class InputVideo {
	public:
		
		/**
		 *	@param	mFileName the file name of the video
		 */
		InputVideo(std::string mFileName);
		
		/**
		 *	destructor
		 */
		~InputVideo();

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
		 *	seeks to the given framenumber and stores it in the video object to use for reading
		 *	it when the mFrameNumber is different than lastFrameRead+1 it actually seeks to the
		 *	next keyframe and stores this framenumber for reading purposes
		 *	@param	mFrameNumber the number of the frame
		 *	@return	returns if seek was successful
		 */
		bool seek(int64_t mFrameNumber);
		
		/**
		 *	seeks to the keyframe before the given framenumber and stores it in the video object
		 *	@param	mFrameNumber the number of the frame
		 *	@return	returns if seek was successful
		 */
		bool seekApprox(int64_t mFrameNumber);
		
		/**
		 *	access to raw image data of the current frame.
		 *	The data returned by this function are not to be deleted!
		 *	@param	mColorFormat the color format that the final frame should be
		 *	@return	returns a pointer to the beginning of the buffer or NULL
		 */
		uint8_t* getFrameBuffer(colorFormat mColorFormat);
		
		/**
		 *	gets the number of audio streams
		 *	@return	returns the number of audio streams
		 */
		int getAudioStreamCount();
		
		/**
		 *	gets the number of channels
		 *	@param	mStreamNumber the number of the stream
		 *	@return	returns the number of channels
		 */
		int getAudioChannelCount(unsigned int mStreamNumber);
		
		/**
		 *	gets the sample rate
		 *	@param	mStreamNumber the number of the stream
		 *	@return	returns the sample rate
		 */
		int getSampleRate(unsigned int mStreamNumber);
		
		/**
		 *	gets the sample count
		 *	@param	mStreamNumber the number of the stream
		 *	@return	returns the number of samples
		 */
		int getSampleCount(unsigned int mStreamNumber);
		
		/**
		 *	reads the entire audio from the given file
		 *	@param	mStreamNumber the number of the stream
		 *	@param	mChannel the channel
		 *	@param	mSamples where the samples should be read into
		 *	@return	returns if reading was successful
		 */
		bool getAudioData(unsigned int mStreamNumber,
						  unsigned int mChannel,
						  std::vector<int16_t>& mSamples);
		
		/**
		 *	reads the entire audio from the given file
		 *	@param	streamNumber the number of the stream
		 *	@param	channelNumber the number of the channel
		 *	@param	data float array to hold the data
		 *	@param	length the size of the float array
		 *	@return	returns size of actually read data
		 */
		unsigned int getAudioData(unsigned int streamNumber,
								  unsigned int channelNumber,
								  float* data,
								  unsigned int length);
		
	private:
		
		/**
		 *	access to a certain number of VideoFrames.
		 *	The VideoFrame returned by this function are not to be deleted!
		 *	@param	mColorFormat the color format that the final frame should be
		 *	@return	returns the read frame
		 */
		VideoFrame * getVideoFrame(colorFormat mColorFormat);

		/**
		 *	seeks to the next keyframe
		 *	@param	mFrameNumber the number of the keyframe
		 *	@return	returns if success
		 */
		bool seekToKeyFrame(int64_t mFrameNumber);
		

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

		bool				  interlaced_;			/**< if the source video is interlaced. */
		unsigned int		  firstStream_;			/**< stream index containing the first video stream. */
		
		bool				  seeked_;				/**< if it has been seeked since the last read. */
		int					  framesBeforeKey_;		/**< the number of frames before the keyframe. */
		bool				  loadNextKeyFrame_;	/**< if the next frame to be loaded should be a keyframe. */
		
		std::vector<unsigned int> audioStreamIndices_;	/**< the indices of the audio streams. */
		
		SwsContext				* scaleContext;			/**< the software scale context. */
		SwsContext				* colorConvertContext;	/**< the context used for color conversion. */
		colorFormat				cFormat;				/**< the color format. */
		VideoFrame				* frame;				/**< pointer to a frame. */
	};
}

#endif //__input_video_h
