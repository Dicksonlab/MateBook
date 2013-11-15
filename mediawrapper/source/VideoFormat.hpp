/**
 *  @file	VideoFormat.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the VideoFormat class
 *	@date	18.06.09
 */

#ifndef __video_format_h
#define __video_format_h

#include <string>
#include <vector>

#include "../../common/source/mystdint.h"

struct AVFormatContext;

namespace mw
{
	
	class VideoOutputFormat;
	
	
	/**
	 *	@class	VideoFormat
	 *	@brief	class handeling a single videoformat
	 */
	class VideoFormat
	{
		
	public:
		
		/**
		 *	default constructor
		 */
		VideoFormat();
		
		/**
		 *	default destructor
		 */
		virtual ~VideoFormat();
		
		/**
		 *	loads the AVFormatContext from the given filename
		 *	@param	mFileName string containing the filename
		 *	@return	returns if success
		 */
		bool loadAVFormatContext(std::string mFileName);
		
		/**
		 *	allocs a new format context.
		 *	used for encoding
		 *	@return	returns if success
		 */
		bool allocAVFormatContext();
		
		/**
		 *	adds an output format to the context.
		 *	@param	mOutputFormat the new output format
		 */
		void addOutputFormat(VideoOutputFormat * mOutputFormat);
		
		/**
		 *	opens the output file
		 *	@param	mFileName string containing the filename
		 *	@return	returns if success
		 */
		bool openOutputFile(std::string mFileName);
		
		/**
		 *	writes the file header
		 *	@return	returns if success
		 */
		bool writeHeader();
		
		/**
		 *	writes the file trailer
		 *	@return	returns if success
		 */
		bool writeTrailer();
		
		/**
		 *	provides access to AVFormatContext
		 *	@return	returns a read only pointer to the struct
		 */
		AVFormatContext * getAVFormatContext() const;
		
		/**
		 *	computes the index in the streams array for the given video stream
		 *	@param	mVideoIndex index of the required video stream
		 */
		unsigned int getStreamIndexForVideoStream(unsigned int mVideoIndex = 0);
		
		/**
		 *	computes the index in the streams array for the given audio stream
		 *	@param	mAudioIndex index of the required audio stream
		 */
		unsigned int getStreamIndexForAudioStream(unsigned int mAudioIndex = 0);
		
		/**
		 *	finds all the indices of the audio streams found in the format
		 *	@return	returns a vector containing all
		 */
		std::vector<unsigned int> getStreamIndicesForAudioStreams();
		
		/**
		 *	converts a given frameNumber to timestamp
		 *	@param	mFrameNumber the given frame number
		 *	@param	mInterlaced if the video is in interlaced format
		 *	@param	mStreamIndex the streamindex in which respect we need the time stamp
		 *	@return	returns the corresponding time stamp
		 */
		int64_t frameNumberToTimeStamp(uint64_t mFrameNumber,
									   bool mInterlaced,
									   unsigned int mStreamIndex = 0) const;
		
		/**
		 *	sets the fps to a calculated value
		 *	@param	mFps the calculated value
		 */
		void setFps(float mFps);
		
		/**
		 *	gets the fps
		 *	@return	returns the framerate
		 */
		float getFps() const;
		
		/**
		 *	sets the pts of the first frame
		 *	@param	mFirstFramePts the pts of the first frame
		 */
		void setFirstFramePts(float mFirstFramePts);
		
		/**
		 *	gets the pts of the first frame
		 *	@return	returns firstFramePts_
		 */
		float getFirstFramePts() const;
		
	private:
		
		AVFormatContext	* avFormatContext_;			/**< context of the current video. */
		bool isInputFile_;							/**< if the format is of an input file. */
		
		float	fps_;								/**< the computed framerate of the video. */
		float	firstFramePts_;						/**< the pts of the first frame. */
		
	};
}

#endif //__video_format_h
