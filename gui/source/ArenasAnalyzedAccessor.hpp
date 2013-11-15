#ifndef ArenasAnalyzedAccessor_hpp
#define ArenasAnalyzedAccessor_hpp

#include "Accessor.hpp"

class ArenasAnalyzedAccessor : public Accessor<Item> {
public:
	ArenasAnalyzedAccessor();

	Qt::ItemFlags flags(const Item* item) const;
	QVariant data(const Item* item, int role) const;
	bool setData(Item* item, const QVariant& value, int role) const;

private:
};

#endif
