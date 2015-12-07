#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include "global.hpp"
#include "../../common/source/Stopwatch.hpp"
#include "Fly.hpp"
#include "getBackground.hpp"
#include "getBodyThreshold.hpp"
#include "drawFly.hpp"
#include "../../common/source/serialization.hpp"
#include "Arena.hpp"
#include "findArenas.hpp"
#include "hofacker.hpp"
#include "../../common/source/Settings.hpp"
#include "../../common/source/fileUtilities.hpp"
#include "../../mediawrapper/source/mediawrapper.hpp"
#include "../../mediawrapper/source/VideoFrame.hpp"
#include "../../common/source/debug.hpp"

int main(int argc, char* const argv[])
{
	#if !defined(_DEBUG)
	try {
	#endif

		// turn off modal dialog on application crash - we'd rather have the tracker script continue with the next video
		//crashReport(false);

		// configuration
		Settings commandLine;
		commandLine.add("executable", global::executable);
		commandLine.add("in", global::videoFile);
		commandLine.add("settings", global::settingsFile);
		commandLine.add("out", global::outDir);
		float timeBegin; commandLine.add("begin", timeBegin);
		float timeEnd; commandLine.add("end", timeEnd);
		bool preprocess; commandLine.add("preprocess", preprocess);
		bool track; commandLine.add("track", track);
		bool postprocess; commandLine.add("postprocess", postprocess);
		commandLine.importProgramArguments(argc, argv);

		Settings trackerSettings;
		bool attachDebugger; trackerSettings.add("debug", attachDebugger);
		bool visualize; trackerSettings.add("visualize", visualize);
		bool saveContours; trackerSettings.add("contours", saveContours);
		bool saveHistograms; trackerSettings.add("histograms", saveHistograms);

		int shape = CIRCLE; trackerSettings.add("shape", shape);
		int interior = EITHER; trackerSettings.add("interior", interior);
		float diameter; trackerSettings.add("diameter", diameter);
		float arenaBorderSize; trackerSettings.add("arenaBorderSize", arenaBorderSize);
		unsigned int fliesPerArena; trackerSettings.add("hints_flyCount", fliesPerArena);
		bool annotation; trackerSettings.add("annotation", annotation);

		bool segmentation_gradientCorrection; trackerSettings.add("segmentation_gradientCorrection", segmentation_gradientCorrection);
		bool fullyMergeMissegmentations; trackerSettings.add("fullyMergeMissegmentations", fullyMergeMissegmentations);
		bool splitBodies; trackerSettings.add("splitBodies", splitBodies);
		bool splitWings; trackerSettings.add("splitWings", splitWings);
		float segmentation_thresholdOffset; trackerSettings.add("segmentation_thresholdOffset", segmentation_thresholdOffset);
		float segmentation_minFlyBodySize; trackerSettings.add("segmentation_minFlyBodySize", segmentation_minFlyBodySize);
		float segmentation_maxFlyBodySize; trackerSettings.add("segmentation_maxFlyBodySize", segmentation_maxFlyBodySize);

		bool discardMissegmentations; trackerSettings.add("discardMissegmentations", discardMissegmentations);
		float occlusions_sSize; trackerSettings.add("occlusions_sSize", occlusions_sSize);
		float occlusions_tPos; trackerSettings.add("occlusions_tPos", occlusions_tPos);
		float occlusions_tBoc; trackerSettings.add("occlusions_tBoc", occlusions_tBoc);

		float heading_sMotion; trackerSettings.add("heading_sMotion", heading_sMotion);
		float heading_sWings; trackerSettings.add("heading_sWings", heading_sWings);
		float heading_sMaxMotionWings; trackerSettings.add("heading_sMaxMotionWings", heading_sMaxMotionWings);
		float heading_sColor; trackerSettings.add("heading_sColor", heading_sColor);
		float heading_tBefore; trackerSettings.add("heading_tBefore", heading_tBefore);

		float copulating_medianFilterWidth; trackerSettings.add("copulating_medianFilterWidth", copulating_medianFilterWidth);
		float copulating_persistence; trackerSettings.add("copulating_persistence", copulating_persistence);

		float orienting_maxAngle; trackerSettings.add("orienting_maxAngle", orienting_maxAngle);
		float orienting_minDistance; trackerSettings.add("orienting_minDistance", orienting_minDistance);
		float orienting_maxDistance; trackerSettings.add("orienting_maxDistance", orienting_maxDistance);
		float orienting_maxSpeedSelf; trackerSettings.add("orienting_maxSpeedSelf", orienting_maxSpeedSelf);
		float orienting_maxSpeedOther; trackerSettings.add("orienting_maxSpeedOther", orienting_maxSpeedOther);
		float orienting_medianFilterWidth; trackerSettings.add("orienting_medianFilterWidth", orienting_medianFilterWidth);
		float orienting_persistence; trackerSettings.add("orienting_persistence", orienting_persistence);

		float rayEllipseOrienting_growthOther; trackerSettings.add("rayEllipseOrienting_growthOther", rayEllipseOrienting_growthOther);
		float rayEllipseOrienting_maxAngle; trackerSettings.add("rayEllipseOrienting_maxAngle", rayEllipseOrienting_maxAngle);
		float rayEllipseOrienting_minDistance; trackerSettings.add("rayEllipseOrienting_minDistance", rayEllipseOrienting_minDistance);
		float rayEllipseOrienting_maxDistance; trackerSettings.add("rayEllipseOrienting_maxDistance", rayEllipseOrienting_maxDistance);
		float rayEllipseOrienting_maxSpeedSelf; trackerSettings.add("rayEllipseOrienting_maxSpeedSelf", rayEllipseOrienting_maxSpeedSelf);
		float rayEllipseOrienting_maxSpeedOther; trackerSettings.add("rayEllipseOrienting_maxSpeedOther", rayEllipseOrienting_maxSpeedOther);
		float rayEllipseOrienting_medianFilterWidth; trackerSettings.add("rayEllipseOrienting_medianFilterWidth", rayEllipseOrienting_medianFilterWidth);
		float rayEllipseOrienting_persistence; trackerSettings.add("rayEllipseOrienting_persistence", rayEllipseOrienting_persistence);

		float following_maxChangeOfDistance; trackerSettings.add("following_maxChangeOfDistance", following_maxChangeOfDistance);
		float following_maxAngle; trackerSettings.add("following_maxAngle", following_maxAngle);
		float following_minDistance; trackerSettings.add("following_minDistance", following_minDistance);
		float following_maxDistance; trackerSettings.add("following_maxDistance", following_maxDistance);
		float following_minSpeed; trackerSettings.add("following_minSpeed", following_minSpeed);
		float following_maxMovementDirectionDifference; trackerSettings.add("following_maxMovementDirectionDifference", following_maxMovementDirectionDifference);
		float following_medianFilterWidth; trackerSettings.add("following_medianFilterWidth", following_medianFilterWidth);
		float following_persistence; trackerSettings.add("following_persistence", following_persistence);

		float circling_minDistance; trackerSettings.add("circling_minDistance", circling_minDistance);
		float circling_maxDistance; trackerSettings.add("circling_maxDistance", circling_maxDistance);
		float circling_maxAngle; trackerSettings.add("circling_maxAngle", circling_maxAngle);
		float circling_minSpeedSelf; trackerSettings.add("circling_minSpeedSelf", circling_minSpeedSelf);
		float circling_maxSpeedOther; trackerSettings.add("circling_maxSpeedOther", circling_maxSpeedOther);
		float circling_minAngleDifference; trackerSettings.add("circling_minAngleDifference", circling_minAngleDifference);
		float circling_minSidewaysSpeed; trackerSettings.add("circling_minSidewaysSpeed", circling_minSidewaysSpeed);
		float circling_medianFilterWidth; trackerSettings.add("circling_medianFilterWidth", circling_medianFilterWidth);
		float circling_persistence; trackerSettings.add("circling_persistence", circling_persistence);

		float wingExtension_minAngle; trackerSettings.add("wingExtension_minAngle", wingExtension_minAngle);
		float wingExtension_tailQuadrantAreaRatio; trackerSettings.add("wingExtension_tailQuadrantAreaRatio", wingExtension_tailQuadrantAreaRatio);
		float wingExtension_directionTolerance; trackerSettings.add("wingExtension_directionTolerance", wingExtension_directionTolerance);
		float wingExtension_minBoc; trackerSettings.add("wingExtension_minBoc", wingExtension_minBoc);
		float wingExtension_angleMedianFilterWidth; trackerSettings.add("wingExtension_angleMedianFilterWidth", wingExtension_angleMedianFilterWidth);
		float wingExtension_areaMedianFilterWidth; trackerSettings.add("wingExtension_areaMedianFilterWidth", wingExtension_areaMedianFilterWidth);
		float wingExtension_persistence; trackerSettings.add("wingExtension_persistence", wingExtension_persistence);

		float circlingWeight; trackerSettings.add("circling_contribution", circlingWeight);
		float copulatingWeight; trackerSettings.add("copulating_contribution", copulatingWeight);
		float followingWeight; trackerSettings.add("following_contribution", followingWeight);
		float orientingWeight; trackerSettings.add("orienting_contribution", orientingWeight);
		float rayEllipseOrientingWeight; trackerSettings.add("rayEllipseOrienting_contribution", rayEllipseOrientingWeight);
		float wingExtWeight; trackerSettings.add("wingExtension_contribution", wingExtWeight);

		float binSize; trackerSettings.add("binSize", binSize);
		unsigned int binCount; trackerSettings.add("binCount", binCount);

		std::string ethogramSpecification; trackerSettings.add("ethogramSpecification", ethogramSpecification);

		trackerSettings.importFrom(global::settingsFile);

		breakIf(attachDebugger);

		// visualize crashes the program if there's no display (like on a cluster node)
		#if !defined(WIN32) && !defined(__APPLE__)
			char* display = std::getenv("DISPLAY");
			if (!display || std::string(display).empty()) {
				visualize = false;
				std::cerr << "warning: live visualization requested, but no DISPLAY defined" << std::endl;
			}
		#endif

		bool processArenaSubsetOnly = false;
		std::set<std::string> arenasToProcess;	// shall contain arena IDs
		try {
/* TODO: read a set<unsigned int> from trackerSettings
			if (Singleton<Settings>::instance().defined("arena")) {
				arenasToProcess.insert(Singleton<Settings>::instance().get("arena").as<unsigned int>());
				processArenaSubsetOnly = true;
			}
*/
		} catch (...) {
			//TODO: fix usage
			std::cerr << "usage: " << global::executable << " -in \"C:/path/to/input video file.MTS\" -out \"C:/path/to/output directory/\" [-preprocess] [-track] [-postprocess] [-visualize] [-arena N] [-settings file]" << std::endl;
			return 1;
		}

		std::string imageFormat(".png");

		// load the source video
		mw::initialize();
		mw::InputVideo sourceVideo(global::videoFile);

		// get the meta data for the video
		int sourceWidth = sourceVideo.getFrameWidth();
		int sourceHeight = sourceVideo.getFrameHeight();
		double sourceFrameRate = sourceVideo.getFrameRate();
		size_t sourceFrameCount = sourceVideo.getNumberOfFrames();
		if (!(sourceWidth > 0 && sourceHeight > 0 && sourceFrameRate > 0 && sourceFrameCount > 0)) {
			std::cerr << "error: video meta data did not pass sanity check" << std::endl;
			return -1;
		}
		std::cout << "info: resolution " << sourceWidth << "x" << sourceHeight << ", fps " << sourceFrameRate << ", frames " << sourceFrameCount << std::endl;

		// determine the range of frames to be processed
		size_t frameBegin = static_cast<size_t>(sourceFrameRate * timeBegin);
		size_t frameEnd = std::min(sourceFrameCount, static_cast<size_t>(sourceFrameRate * timeEnd));
		
		// preprocessing: either generate or load bgMedian and arenas
		cv::Mat bgMedian;
		std::vector<Arena> arenas;
		if (preprocess) {
			// get the background
			bgMedian = getBackground(sourceVideo, "courtship");	//TODO: pass frameBegin and frameEnd to take only that range into account when generating the background?
			if (visualize) {
				cv::namedWindow("bgMedian", CV_WINDOW_AUTOSIZE);
				cv::imshow("bgMedian", bgMedian);
				cv::namedWindow("tracking", CV_WINDOW_AUTOSIZE);
			}
			cv::imwrite(global::outDir + "/background" + imageFormat, bgMedian);

			// find the arenas and pick only the selected ones
			std::vector<Arena> arenasFound = findArenas(bgMedian, (Shape)shape, sourceFrameRate, fliesPerArena, diameter, arenaBorderSize, (Interior)interior);	//TODO: check range of shape and interior
			if (processArenaSubsetOnly) {
				for (std::vector<Arena>::const_iterator iter = arenasFound.begin(); iter != arenasFound.end(); ++iter) {
					if (arenasToProcess.find(iter->getId()) != arenasToProcess.end()) {
						arenas.push_back(*iter);
					}
				}
			} else {
				arenas = arenasFound;
			}

			// write arena meta data
			for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
				std::string arenaFileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/arena.tsv");
				std::ofstream arenaFile(arenaFileName.c_str());
				cv::Rect bb = arenas[arenaNumber].getBoundingBox();
				arenaFile << "left" << '\t' << bb.x << "\n";
				arenaFile << "top" << '\t' << bb.y << "\n";
				arenaFile << "width" << '\t' << bb.width << "\n";
				arenaFile << "height" << '\t' << bb.height << "\n";
				arenaFile << "diameter" << '\t' << diameter << "\n";
				arenaFile << "approved" << '\t' << true << "\n";
				arenaFile.close();
			}

			// write arena masks
			for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
				std::string maskFileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/mask" + imageFormat);
				imwrite(maskFileName, arenas[arenaNumber].getMask());
			}

			// create file to indicate successful preprocessing
			std::ofstream((global::outDir + "/" + "preprocess_done_success.txt").c_str()).close();

			// write an image file to visualize the arenas (for debugging purposes)
			cv::Mat bgArenas = bgMedian.clone();
			for (std::vector<Arena>::const_iterator iter = arenas.begin(); iter != arenas.end(); ++iter) {
				rectangle(bgArenas, iter->getBoundingBox().tl(), iter->getBoundingBox().br(), cv::Scalar(0, 255, 0));
			}
			imwrite(global::outDir + "/arenas" + imageFormat, bgArenas);
		} else {
			// load preprocessor output from disk
			bgMedian = cv::imread(global::outDir + "/background" + imageFormat);
			std::vector<std::string> arenaFileNames = ls(global::outDir);
			for (std::vector<std::string>::const_iterator iter = arenaFileNames.begin(); iter != arenaFileNames.end(); ++iter) {
				try {
					Settings arenaSettings;
					unsigned int left; arenaSettings.add("left", left);
					unsigned int top; arenaSettings.add("top", top);
					unsigned int width; arenaSettings.add("width", width);
					unsigned int height; arenaSettings.add("height", height);
					float diameter; arenaSettings.add("diameter", diameter);
					bool approved; arenaSettings.add("approved", approved);
					//TODO: add arenaBorderSize?
					arenaSettings.importFrom(global::outDir + "/" + *iter + "/arena.tsv");

					std::string id(*iter);
					if (processArenaSubsetOnly && arenasToProcess.find(id) == arenasToProcess.end()) {
						continue;
					}

					std::string arenaMaskName = global::outDir + "/" + id + "/mask" + imageFormat;
					cv::Mat arenaMask = cv::imread(arenaMaskName, 0);	// 0 means load as grayscale

					if (arenaMask.empty()) {	// loading from disk failed
						arenaMask = drawArenaMask(cv::Size(width, height), (Shape)shape, arenaBorderSize);
						imwrite(arenaMaskName, arenaMask);
					}

					cv::Rect bb(left, top, width, height);
					arenas.push_back(Arena(id, sourceFrameRate, fliesPerArena, bb, diameter, arenaBorderSize, cv::Mat(arenaMask), cv::Mat(bgMedian, bb)));
				} catch (const std::runtime_error& e) {
					std::cerr << "failed to load " << *iter << ": " << e.what() << std::endl;
					continue;
				} catch (...) {
					std::cerr << "failed to load " << *iter << std::endl;
					continue;
				}
			}
		}

		if (preprocess && !track) {
			return 0;
		}

		// tracking: either generate or load normalized tracking data (frameAttributes, flyAttributes and occlusionMap) per arena
		if (track) {
			Stopwatch stopwatch;
			stopwatch.start();
			cv::Mat frame(cv::Size(sourceWidth, sourceHeight), CV_8UC3);
			cv::Mat visualizedContours(frame.size(), frame.type());
			for (unsigned int frameNumber = frameBegin; frameNumber != frameEnd && cv::waitKey(30) < 0; ++frameNumber) {
				if (!sourceVideo.seek(frameNumber)) {
					break;
				}
				if (unsigned char* frameData = sourceVideo.getFrameBuffer(PIX_FMT_BGR24)) {
					std::copy(frameData, frameData + sourceWidth * sourceHeight * 3, frame.datastart);
				}
				//cvtColor(frame, frame, CV_RGB2BGR);	// OpenCV functions like imshow expect BGR

				visualizedContours = frame.clone();
				for (unsigned int arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
					arenas[arenaNumber].track(frame, frameNumber, sourceFrameCount, frameEnd - frameBegin, visualizedContours, segmentation_thresholdOffset, segmentation_minFlyBodySize, segmentation_maxFlyBodySize, segmentation_gradientCorrection, fullyMergeMissegmentations, splitBodies, splitWings, saveContours, saveHistograms);
				}
				if (visualize) {
					imshow("tracking", visualizedContours);
				}

				if (frameNumber % static_cast<int>(sourceFrameRate) == 0) {
					stopwatch.stop();
					std::cout << "@frame " << frameNumber << ": ";
					std::cout << "1 second of video data processed in " << stopwatch.read() << " seconds" << std::endl;
					stopwatch.set();
					stopwatch.start();
				}
			}

			for (unsigned int arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
				arenas[arenaNumber].normalizeTrackingData();
			}
		} else {
			for (unsigned int arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
				std::string trackFileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/track.tsv");
				std::ifstream trackFile(trackFileName.c_str());
				arenas[arenaNumber].importTrackingData(trackFile);
			}
		}

		// postprocessing
		for (unsigned int arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::cout << "postprocessing arena " << arenas[arenaNumber].getId() << std::endl;

			std::cout << "  prepareInterpolation" << std::endl;
			arenas[arenaNumber].prepareInterpolation();

			std::cout << "  buildSequenceMaps" << std::endl;
			arenas[arenaNumber].buildSequenceMaps();

			std::cout << "  detectMissegmentations" << std::endl;
			arenas[arenaNumber].detectMissegmentations(segmentation_minFlyBodySize, segmentation_maxFlyBodySize);

			std::cout << "  writing segmentation statistics" << std::endl;
			{
				std::string fileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/segmentationStatistics.tsv");
				std::ofstream file(fileName.c_str());
				arenas[arenaNumber].writeSegmentationStatistics(file);
			}

			std::cout << "  calculateTScores" << std::endl;
			arenas[arenaNumber].calculateTScores(occlusions_tPos, occlusions_tBoc);

			std::cout << "  solveOcclusions" << std::endl;
			arenas[arenaNumber].solveOcclusions(occlusions_sSize, discardMissegmentations);

			if (annotation) {
				std::cout << "  addAnnotations" << std::endl;
				arenas[arenaNumber].addAnnotations(global::outDir + "/" + arenas[arenaNumber].getId() + "/annotation.tsv");
			}

			std::cout << "  writing occlusion report" << std::endl;
			{
				std::string fileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/occlusionReport.tsv");
				std::ofstream file(fileName.c_str());
				arenas[arenaNumber].writeOcclusionReport(file);
			}

			std::cout << "  interpolateAttributes" << std::endl;
			arenas[arenaNumber].interpolateAttributes();

			std::cout << "  deriveHeadingIndependentAttributes" << std::endl;
			arenas[arenaNumber].deriveHeadingIndependentAttributes();

			std::cout << "  solveHeading" << std::endl;
			arenas[arenaNumber].solveHeading(heading_sMotion, heading_sWings, heading_sMaxMotionWings, heading_sColor, heading_tBefore);

			std::cout << "  interpolateOrientation" << std::endl;
			arenas[arenaNumber].interpolateOrientation();

			std::cout << "  selectQuadrants" << std::endl;
			arenas[arenaNumber].selectQuadrants();

			std::cout << "  deriveHeadingDependentAttributes" << std::endl;
			arenas[arenaNumber].deriveHeadingDependentAttributes();

			std::cout << "  convertUnits" << std::endl;
			arenas[arenaNumber].convertUnits();

			std::cout << "  deriveEvents" << std::endl;
			arenas[arenaNumber].deriveCopulating(copulating_medianFilterWidth, copulating_persistence);
			arenas[arenaNumber].deriveOrienting(orienting_maxAngle, orienting_minDistance, orienting_maxDistance, orienting_maxSpeedSelf, orienting_maxSpeedOther, orienting_medianFilterWidth, orienting_persistence);
			arenas[arenaNumber].deriveRayEllipseOrienting(rayEllipseOrienting_growthOther, rayEllipseOrienting_maxAngle, rayEllipseOrienting_minDistance, rayEllipseOrienting_maxDistance, rayEllipseOrienting_maxSpeedSelf, rayEllipseOrienting_maxSpeedOther, rayEllipseOrienting_medianFilterWidth, rayEllipseOrienting_persistence);
			arenas[arenaNumber].deriveFollowing(following_maxChangeOfDistance, following_maxAngle, following_minDistance, following_maxDistance, following_minSpeed, following_maxMovementDirectionDifference, following_medianFilterWidth, following_persistence);
			arenas[arenaNumber].deriveCircling(circling_minDistance, circling_maxDistance, circling_maxAngle, circling_minSpeedSelf, circling_maxSpeedOther, circling_minAngleDifference, circling_minSidewaysSpeed, circling_medianFilterWidth, circling_persistence);
			arenas[arenaNumber].deriveWingExt(wingExtension_minAngle, wingExtension_tailQuadrantAreaRatio, wingExtension_directionTolerance, wingExtension_minBoc, wingExtension_angleMedianFilterWidth, wingExtension_areaMedianFilterWidth, wingExtension_persistence);
			arenas[arenaNumber].deriveCourtship(circlingWeight, copulatingWeight, followingWeight, orientingWeight, rayEllipseOrientingWeight, wingExtWeight);
			arenas[arenaNumber].deriveNew();
		}

		// export the data
		std::cout << "exporting the data" << std::endl;
		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::string trackFileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/track.tsv");
			std::ofstream trackFile(trackFileName.c_str());
			arenas[arenaNumber].exportTrackingData(trackFile);
		}

		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::string trackDirectory(global::outDir + "/" + arenas[arenaNumber].getId() + "/track");
			//removeDirectory(trackDirectory);
			makeDirectory(trackDirectory);
			if (true) {	//TODO: check if it exists
				arenas[arenaNumber].exportAttributes(trackDirectory);

				// create file to indicate successful tracking
				std::ofstream((global::outDir + "/" + arenas[arenaNumber].getId() + "/track_done_success.txt").c_str()).close();
			} else {
				std::cerr << "warning: could not create \"" << trackDirectory << "\"" << std::endl;
			}
		}

		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::string fileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/mean.tsv");
			std::ofstream file(fileName.c_str());
			arenas[arenaNumber].writeMean(file);
		}

		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::string fileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/behavior.tsv");
			std::ofstream file(fileName.c_str());
			arenas[arenaNumber].writeBehavior(file, binSize, binCount);
		}

		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			arenas[arenaNumber].writeEthograms(global::outDir, ethogramSpecification);
		}

		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::string fileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/missegmented.tsv");
			std::ofstream file(fileName.c_str());
			arenas[arenaNumber].writeMissegmented(file);
		}

		for (size_t arenaNumber = 0; arenaNumber != arenas.size(); ++arenaNumber) {
			std::string fileName(global::outDir + "/" + arenas[arenaNumber].getId() + "/positionCorrelation.tsv");
			std::ofstream file(fileName.c_str());
			arenas[arenaNumber].writePositionCorrelation(file);
		}

	#if !defined(_DEBUG)
	} catch (std::exception& e) {
		std::cerr << "std::exception: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "exception: unknown" << std::endl;
		return 1;
	}
	#endif

	return 0;
}
