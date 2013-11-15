#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <stdint.h>
#include <boost/static_assert.hpp>
#include "Video.hpp"
#include "../../mediawrapper/source/VideoFrame.hpp"
#include "../../common/source/BinaryReader.hpp"

Video::Video() :
	fileName()
{
}

Video::Video(const std::string& fileName) :
	fileName(fileName),
	inputVideo(new mw::InputVideo(fileName)),
	width(),
	height(),
	fps(),
	numFrames(),
	sampleRate(),
	song()
{
	width = inputVideo->getFrameWidth();
	height = inputVideo->getFrameHeight();
	fps = inputVideo->getFrameRate();
	numFrames = inputVideo->getNumberOfFrames();

	if (numFrames == 0 && fileName.size() >= 4 && fileName.substr(fileName.size() - 4) == ".wav") {
		//unsigned int streamNumber = getAudioStreamCount(videoId);
		//unsigned int channelNumber = getAudioChannelCount(videoId, streamNumber-1);
		//sampleRate = ::getSampleRate(videoId, streamNumber-1);
		//samples = getSampleCount(videoId, streamNumber-1);
		//song.resize(samples);
		//unsigned int sampleNumberWritten = getAudioData(videoId, streamNumber-1, channelNumber, &song[0], samples);

		readWaveFile(fileName);
	} else if (width < 1 || height < 1 || fps <= 0 || numFrames < 1) {
		throw std::runtime_error("video did not pass sanity checks: " + fileName);
	}
}

Video::~Video()
{
	inputVideo.reset();
}

std::string Video::getFileName() const
{
	return fileName;
}

int Video::getWidth() const
{
	return width;
}

int Video::getHeight() const
{
	return height;
}

unsigned int Video::getNumFrames() const
{
	return numFrames;
}

double Video::getFps() const
{
	return fps;
}

double Video::getFrameTime() const
{
	return 1 / fps;
}

unsigned int Video::getSamples() const
{
	return song.size();
}

unsigned int Video::getSampleRate() const
{
	return sampleRate;
}

const std::vector<float>& Video::getSong() const
{
	return song;
}

unsigned char* Video::getCurrentFrame()
{
	return inputVideo->getFrameBuffer(PIX_FMT_RGB24);
}

unsigned char* Video::getNextFrame()
{
	if (inputVideo->seek(inputVideo->getCurrentFrameNumber() + 1)) {
		return inputVideo->getFrameBuffer(PIX_FMT_RGB24);
	}
	return NULL;
}

bool Video::isVideo()
{
	return numFrames == 0 ? false : true;
}

unsigned int Video::getCurrentFrameNumber() const
{
	return inputVideo->getCurrentFrameNumber();
}

void Video::seek(unsigned int frameNumber)
{
	inputVideo->seek(frameNumber);
}

void Video::rewind()
{
	inputVideo->seek(0);
}

void Video::readWaveFile(const std::string& path)
{
	struct WaveHeader {
		char chunkID[4];
		int32_t chunkSize;
		char format[4];
		char subchunk1ID[4];
		int32_t subchunk1Size;
		int16_t audioFormat;
		int16_t numChannels; 
		int32_t sampleRate;
		int32_t byteRate;
		int16_t blockAlign;
		int16_t bitsPerSample;
		char subchunk2ID[4];
		int32_t subchunk2Size;
	};
	BOOST_STATIC_ASSERT(sizeof(WaveHeader) == 44);

	WaveHeader waveHeader = {};
	BinaryReader waveFile(path);
	waveFile.readInto(waveHeader);

	// sanity checks to see if we support this file
	if (
		std::string(waveHeader.chunkID, 4) != "RIFF" ||
		std::string(waveHeader.format, 4) != "WAVE"  ||
		std::string(waveHeader.subchunk1ID, 4) != "fmt " ||
		waveHeader.subchunk1Size != 16 ||
		waveHeader.numChannels != 1 ||
		waveHeader.bitsPerSample != 16 ||
		std::string(waveHeader.subchunk2ID, 4) != "data" ||
		waveHeader.subchunk2Size < 0
	) {
		throw std::runtime_error(std::string("Could not decode .wav file \"") + path + "\".");
	}

	this->sampleRate = waveHeader.sampleRate;

	std::vector<int16_t> data(waveHeader.subchunk2Size / (waveHeader.bitsPerSample / 8));
	waveFile.readInto(data);
	song.resize(data.size());
	for (size_t i = 0; i != data.size(); ++i) {
		song[i] = static_cast<float>(data[i]) / std::numeric_limits<int16_t>::max();
	}
}
