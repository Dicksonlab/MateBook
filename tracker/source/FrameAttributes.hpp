#ifndef FrameAttributes_hpp
#define FrameAttributes_hpp

#include <vector>
#include <string>

#include "AttributeCollection.hpp"
#include "Attribute.hpp"

class FrameAttributes : public AttributeCollection {
public:
	FrameAttributes();

	#if !defined(MATEBOOK_GUI)
		void appendFrame(const TrackedFrame& frame);
	#endif

	void swap(const std::vector<bool>& mask);
	void clearDerivedAttributes();

	bool isOcclusionSScore(std::string name) const;
	bool isOcclusionTScore(std::string name) const;

	bool isOcclusionSProb(std::string name) const;
	bool isOcclusionTProb(std::string name) const;

	std::vector<std::string> getOcclusionSScoreNames() const;
	std::vector<std::string> getOcclusionTScoreNames() const;

	std::vector<std::string> getOcclusionSProbNames() const;
	std::vector<std::string> getOcclusionTProbNames() const;

private:
	//TODO: shouldn't these be sets instead of vectors?
	std::vector<std::string> trackingAttributes;	// attributes acquired during tracking or generated in normalizeTrackingData
	std::vector<std::string> derivedAttributes;	// attributes generated after normalizeTrackingData

	std::vector<std::string> occlusionScoreAttributes;	// attributes that must be swapped by multiplying by -1
	std::vector<std::string> occlusionSScoreAttributes;
	std::vector<std::string> occlusionTScoreAttributes;

	std::vector<std::string> occlusionProbAttributes;	// attributes a that must be swapped by a = 1 - a
	std::vector<std::string> occlusionSProbAttributes;
	std::vector<std::string> occlusionTProbAttributes;
};

#endif
