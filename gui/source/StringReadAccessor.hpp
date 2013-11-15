#ifndef StringReadAccessor_hpp
#define StringReadAccessor_hpp

#include <string>

#include "Accessor.hpp"

/**
  * @class  StringReadAccessor
  * @brief  for read-only access to std::string
  *
  * For QString use ReadAccessor or ReadWriteAccessor as it can be converted to QVariant directly.
  */
class StringReadAccessor : public Accessor<Item> {
	typedef std::string (Item::*Getter)() const;

public:
	StringReadAccessor(Getter getter);

	Qt::ItemFlags flags(const Item* item) const;
	QVariant data(const Item* item, int role) const;
	bool setData(Item* item, const QVariant& value, int role) const;

private:
	Getter getter;
};

#endif
