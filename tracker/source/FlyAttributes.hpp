#ifndef FlyAttributes_hpp
#define FlyAttributes_hpp

#include <vector>
#include <string>

#include "AttributeCollection.hpp"
#include "Attribute.hpp"
#if !defined(MATEBOOK_GUI)
#include "Fly.hpp"
#endif

class FlyAttributes : public AttributeCollection {
public:
	FlyAttributes();

#if !defined(MATEBOOK_GUI)
	void appendFly(const Fly& fly);
	void appendEmpty();
#endif

	void swap(FlyAttributes& other, const std::vector<bool>& mask);
	void clearDerivedAttributes();

private:
	std::vector<std::string> contourAttributes;	// attributes concerning the flies' contours
	std::vector<std::string> trackingAttributes;	// attributes acquired during tracking and copied from the Fly objects in normalizeTrackingData
	std::vector<std::string> derivedAttributes;	// attributes generated after normalizeTrackingData
};

#endif
