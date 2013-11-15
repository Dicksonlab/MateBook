/**
 *  @file	VideoOutputFormat.h
 *	@author	Herbert Grasberger
 *  @brief	contains the interface for the VideoOutputFormat class
 *	@date	10.07.09
 */

#ifndef __video_output_format_h
#define __video_output_format_h

#include <string>


struct AVOutputFormat;

namespace mw
{
	
	/**
	 *	@class	VideoOutputFormat
	 *	@brief	class handeling a single output videoformat
	 */
	class VideoOutputFormat
		{
			
		public:
			
			/**
			 *	default constructor
			 */
			VideoOutputFormat();
			
			/**
			 *	default destructor
			 */
			virtual ~VideoOutputFormat();
			
			/**
			 *	guesses the output format according to the given filename
			 *	@param	mFileName string containing the filename
			 *	@return	returns if success
			 */
			bool guessOutputFormat(std::string mFileName);
			
			/**
			 *	provides access to AVOutputFormat
			 *	@return	returns a read only pointer to the struct
			 */
			AVOutputFormat * getAVOutputFormat() const;
			
		private:
			
			AVOutputFormat	* avOutputFormat_;			/**< output format. */
			
		};
}

#endif //__video_output_format_h
