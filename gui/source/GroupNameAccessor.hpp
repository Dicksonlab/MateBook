#ifndef GroupNameAccessor_hpp
#define GroupNameAccessor_hpp

#include "Accessor.hpp"
#include "AbstractGroupItem.hpp"

class GroupNameAccessor : public Accessor<AbstractGroupItem> {
public:
	GroupNameAccessor();

	Qt::ItemFlags flags(const AbstractGroupItem* item) const;
	QVariant data(const AbstractGroupItem* item, int role) const;
	bool setData(AbstractGroupItem* item, const QVariant& value, int role) const;

private:
};

#endif
