#ifndef ArenasApprovedAccessor_hpp
#define ArenasApprovedAccessor_hpp

#include "Accessor.hpp"

class ArenasApprovedAccessor : public Accessor<Item> {
public:
	ArenasApprovedAccessor();

	Qt::ItemFlags flags(const Item* item) const;
	QVariant data(const Item* item, int role) const;
	bool setData(Item* item, const QVariant& value, int role) const;

private:
};

#endif
