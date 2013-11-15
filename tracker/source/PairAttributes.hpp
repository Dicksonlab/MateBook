#ifndef PairAttributes_hpp
#define PairAttributes_hpp

#include <vector>
#include <string>

#include "AttributeCollection.hpp"
#include "Attribute.hpp"

class PairAttributes : public AttributeCollection {
public:
	PairAttributes();

	void clearDerivedAttributes();

private:
	std::vector<std::string> derivedAttributes;	// attributes generated after normalizeTrackingData
};

#endif
