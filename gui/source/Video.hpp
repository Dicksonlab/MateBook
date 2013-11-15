#ifndef Video_hpp
#define Video_hpp

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "../../mediawrapper/source/mediawrapper.hpp"

class SongMatlabDataLoader;
/**
  * @class  Video
  * @brief  represents a video file
  */
class Video {
public:
	Video();
	Video(const std::string& fileName);

	~Video();

	std::string getFileName() const;
	
	// video specific
	int getWidth() const;
	int getHeight() const;
	unsigned int getNumFrames() const;
	double getFps() const;
	double getFrameTime() const;

	// audio specific
	unsigned int getSamples() const;
	unsigned int getSampleRate() const;
	const std::vector<float>& getSong() const;

	unsigned char* getCurrentFrame();
	unsigned char* getNextFrame();
	bool isVideo();
	unsigned int getCurrentFrameNumber() const;
	void seek(unsigned int frameNumber);
	void rewind();

private:
	std::string fileName;
	boost::shared_ptr<mw::InputVideo> inputVideo;
	int width;
	int height;
	double fps;

	unsigned int numFrames;
	unsigned int sampleRate;
	std::vector<float> song;

	void readWaveFile(const std::string& path);
};

#endif
