#include "TrackingResults.hpp"
#include <QFile>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QTextStream>
#include <iostream>
#include <fstream>
#include <cassert>
#include <stdint.h>
#include "RuntimeError.hpp"

TrackingResults::TrackingResults(const std::string& fileName, const std::string& contourFileName, const std::string& smoothHistogramFileName) :
	fileName(fileName),
	contourFileName(contourFileName),
	smoothHistogramFileName(smoothHistogramFileName),
	flyCount(0)
{
	QStringList frameAttribFiles = QDir(QString::fromStdString(fileName + "/frame/")).entryList();
	foreach (QString file, frameAttribFiles) {
		std::string fileStdString = file.toStdString();
		if (frameAttributes.has(fileStdString)) {
			frameAttributes.getEmpty(fileStdString).readBinaries(fileName + "/frame/" + fileStdString);
		} else {
			std::cerr << "warning: skipping unknown attribute " << file.toStdString() << std::endl;
		}
	}

	QStringList flyAttribDirs = QDir(QString::fromStdString(fileName + "/fly/")).entryList();
	foreach (QString dir, flyAttribDirs) {
		bool isNumber = false;
		unsigned int flyNumber = 0;
		flyNumber = dir.toUInt(&isNumber);
		if (isNumber) {
			if (flyNumber + 1 > flyAttributes.size()) {
				flyAttributes.resize(flyNumber + 1);
			}

			QStringList flyAttribFiles = QDir(QString::fromStdString(fileName + "/fly/" + dir.toStdString())).entryList();
			foreach (QString file, flyAttribFiles) {
				std::string fileStdString = file.toStdString();
				if (flyAttributes[flyNumber].has(fileStdString)) {
					flyAttributes[flyNumber].getEmpty(fileStdString).readBinaries(fileName + "/fly/" + dir.toStdString() + "/" + fileStdString);
				} else {
					std::cerr << "warning: skipping unknown attribute " << file.toStdString() << std::endl;
				}
			}
		}
	}
	flyCount = flyAttributes.size();

	QStringList activeAttribDirs = QDir(QString::fromStdString(fileName + "/pair/")).entryList();
	foreach (QString activeDir, activeAttribDirs) {
		bool isNumber = false;
		unsigned int activeNumber = 0;
		activeNumber = activeDir.toUInt(&isNumber);
		if (isNumber) {
			if (activeNumber + 1 > pairAttributes.size()) {
				pairAttributes.resize(activeNumber + 1);
			}

			QStringList passiveAttribDirs = QDir(QString::fromStdString(fileName + "/pair/" + activeDir.toStdString())).entryList();
			foreach (QString passiveDir, passiveAttribDirs) {
				bool isNumber = false;
				unsigned int passiveNumber = 0;
				passiveNumber = passiveDir.toUInt(&isNumber);
				if (isNumber) {
					if (passiveNumber + 1 > pairAttributes[activeNumber].size()) {
						pairAttributes[activeNumber].resize(passiveNumber + 1);
					}


					QStringList pairAttribFiles = QDir(QString::fromStdString(fileName + "/pair/" + activeDir.toStdString() + "/" + passiveDir.toStdString())).entryList();
					foreach (QString file, pairAttribFiles) {
						std::string fileStdString = file.toStdString();
						if (pairAttributes[activeNumber][passiveNumber].has(fileStdString)) {
							pairAttributes[activeNumber][passiveNumber].getEmpty(fileStdString).readBinaries(fileName + "/pair/" + activeDir.toStdString() + "/" + passiveDir.toStdString() + "/" + fileStdString);
						} else {
							std::cerr << "warning: skipping unknown attribute " << file.toStdString() << std::endl;
						}
					}
				}
			}
		}
	}

	// sanity check videoFrame numbers: they have to be contiguous
	const Attribute<uint32_t>& videoFrame = getFrameData<uint32_t>("videoFrame");
	if (!videoFrame.empty()) {
		size_t videoFrameNumber = videoFrame.front();
		for (std::vector<uint32_t>::const_iterator iter = videoFrame.begin(); iter != videoFrame.end(); ++iter) {
			if ((*iter) != videoFrameNumber) {
				throw RuntimeError(QObject::tr("The videoFrame numbers are not contiguous in file %1.").arg(QString::fromStdString(fileName)));
			}
			++videoFrameNumber;
		}
	}

	occlusionMap = OcclusionMap(getFrameData<MyBool>("isOcclusion").getData(), getOffset());

	//TODO: load the contours into a VBO...how do we share it between contexts?
	std::ifstream contourFile(contourFileName.c_str(), std::ios::in | std::ios::binary);
	if (contourFile) {
		contourFile.seekg(0, std::ios::beg);
		std::streampos fbegin = contourFile.tellg();
		contourFile.seekg(0, std::ios::end);
		std::streampos fend = contourFile.tellg();
		contourFile.seekg(0, std::ios::beg);
		size_t fileSize = fend - fbegin;	// in bytes

		contours.resize(fileSize);
		if (fileSize != 0) {
			if (!contourFile.read(&contours[0], fileSize)) {
				std::cerr << "couldn't read the expected number of bytes from " << contourFileName << std::endl;
			}
		}
	}

	std::ifstream smoothHistogramFile(smoothHistogramFileName.c_str(), std::ios::in | std::ios::binary);
	if (smoothHistogramFile) {
		smoothHistogramFile.seekg(0, std::ios::beg);
		std::streampos fbegin = smoothHistogramFile.tellg();
		smoothHistogramFile.seekg(0, std::ios::end);
		std::streampos fend = smoothHistogramFile.tellg();
		smoothHistogramFile.seekg(0, std::ios::beg);
		size_t fileSize = fend - fbegin;	// in bytes

		if (fileSize % (256 * sizeof(float)) != 0) {
			std::cerr << "couldn't read the expected number of bytes from " << smoothHistogramFileName << std::endl;
		} else {
			smoothHistograms.resize(fileSize / sizeof(float));
			if (fileSize != 0) {
				if (!smoothHistogramFile.read(reinterpret_cast<char*>(&smoothHistograms[0]), fileSize)) {
					std::cerr << "couldn't read the expected number of bytes from " << smoothHistogramFileName << std::endl;
				}
			}
		}
	}
}

TrackingResults::~TrackingResults()
{
}

std::string TrackingResults::getFileName() const
{
	return fileName;
}

size_t TrackingResults::getFlyCount() const
{
	return flyCount;
}

bool TrackingResults::hasData(size_t videoFrameNumber) const
{
	const Attribute<uint32_t>& videoFrame = getFrameData<uint32_t>("videoFrame");
	if (videoFrame.empty()) {
		return false;
	}
	return videoFrame.front() <= videoFrameNumber && videoFrameNumber <= videoFrame.back();
}

std::vector<std::string> TrackingResults::getFrameAttributeNames() const
{
	return frameAttributes.getNames();
}

std::vector<std::string> TrackingResults::getFlyAttributeNames() const
{
	if (flyAttributes.empty()) {
		return std::vector<std::string>();
	}
	return flyAttributes[0].getNames();
}

std::vector<std::string> TrackingResults::getPairAttributeNames() const
{
	if (pairAttributes.empty() || flyCount < 2) {
		return std::vector<std::string>();
	}
	return pairAttributes[0][0].getNames();
}

const OcclusionMap& TrackingResults::getOcclusionMap() const
{
	return occlusionMap;
}

OcclusionMap& TrackingResults::getOcclusionMap()
{
	return occlusionMap;
}

const std::vector<char>& TrackingResults::getContours() const
{
	return contours;
}

const float* TrackingResults::getSmoothHistogram(size_t videoFrameNumber) const
{
	size_t offset = getOffset();
	if (videoFrameNumber < offset || (videoFrameNumber - offset + 1) * 256 > smoothHistograms.size()) {
		return NULL;
	}
	return &smoothHistograms[(videoFrameNumber - offset) * 256];
}

size_t TrackingResults::getOffset() const
{
	if (getFrameData<uint32_t>("videoFrame").size() == 0) {
		return 0;
	}
	return getFrameData<uint32_t>("videoFrame").front();
}

void TrackingResults::saveAnnotation(const std::string& fileName) const
{
	const char delimiter = '\t';
	std::ostringstream trans;

	std::vector<std::string> attributeNames = frameAttributes.getNames();
	for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
		trans << delimiter;
		getFrameData(*iter)->write(trans, delimiter);
		trans << '\n';
	}

	for (size_t flyNumber = 0; flyNumber != getFlyCount(); ++flyNumber) {
		attributeNames = flyAttributes[flyNumber].getNames();
		for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
			trans << stringify(flyNumber) << delimiter;
			getFlyData(flyNumber, *iter)->write(trans, delimiter);
			trans << '\n';
		}
	}

	for (size_t activeFly = 0; activeFly != getFlyCount(); ++activeFly) {
		for (size_t passiveFly = 0; passiveFly != getFlyCount(); ++passiveFly) {
			if (activeFly == passiveFly) {
				continue;
			}
			attributeNames = pairAttributes[activeFly][passiveFly].getNames();
			for (std::vector<std::string>::const_iterator iter = attributeNames.begin(); iter != attributeNames.end(); ++iter) {
				trans << (stringify(activeFly) + " " + stringify(passiveFly)) << delimiter;
				getPairData(activeFly, passiveFly, *iter)->write(trans, delimiter);
				trans << '\n';
			}
		}
	}

	std::ofstream annotationFile(fileName.c_str());
	annotationFile << ::transpose(trans.str());
}

std::auto_ptr<AbstractAttribute> TrackingResults::getFrameData(std::string attributeName) const
{
	if (hasFrameAttribute<MyBool>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFrameData<MyBool>(attributeName).clone());
	} else if (hasFrameAttribute<float>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFrameData<float>(attributeName).clone());
	} else if (hasFrameAttribute<uint32_t>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFrameData<uint32_t>(attributeName).clone());
	} else if (hasFrameAttribute<Vf2>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFrameData<Vf2>(attributeName).clone());
	}
	return std::auto_ptr<AbstractAttribute>();
}

std::auto_ptr<AbstractAttribute> TrackingResults::getFlyData(size_t flyNumber, std::string attributeName) const
{
	if (hasFlyAttribute<MyBool>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFlyData<MyBool>(flyNumber, attributeName).clone());
	} else if (hasFlyAttribute<float>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFlyData<float>(flyNumber, attributeName).clone());
	} else if (hasFlyAttribute<uint32_t>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFlyData<uint32_t>(flyNumber, attributeName).clone());
	} else if (hasFlyAttribute<Vf2>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getFlyData<Vf2>(flyNumber, attributeName).clone());
	}
	return std::auto_ptr<AbstractAttribute>();
}

std::auto_ptr<AbstractAttribute> TrackingResults::getPairData(size_t activeFly, size_t passiveFly, std::string attributeName) const
{
	if (hasPairAttribute<MyBool>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getPairData<MyBool>(activeFly, passiveFly, attributeName).clone());
	} else if (hasPairAttribute<float>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getPairData<float>(activeFly, passiveFly, attributeName).clone());
	} else if (hasPairAttribute<uint32_t>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getPairData<uint32_t>(activeFly, passiveFly, attributeName).clone());
	} else if (hasPairAttribute<Vf2>(attributeName)) {
		return std::auto_ptr<AbstractAttribute>(getPairData<Vf2>(activeFly, passiveFly, attributeName).clone());
	}
	return std::auto_ptr<AbstractAttribute>();
}
