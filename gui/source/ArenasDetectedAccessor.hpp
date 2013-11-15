#ifndef ArenasDetectedAccessor_hpp
#define ArenasDetectedAccessor_hpp

#include "Accessor.hpp"

class ArenasDetectedAccessor : public Accessor<Item> {
public:
	ArenasDetectedAccessor();

	Qt::ItemFlags flags(const Item* item) const;
	QVariant data(const Item* item, int role) const;
	bool setData(Item* item, const QVariant& value, int role) const;

private:
};

#endif
