#ifndef StringAccessor_hpp
#define StringAccessor_hpp

#include <string>

#include "Accessor.hpp"

/**
  * @class  StringAccessor
  * @brief  to access std::string
  *
  * For QString use ReadAccessor or ReadWriteAccessor as it can be converted to QVariant directly.
  */
class StringAccessor : public Accessor<Item> {
	typedef std::string (Item::*Getter)() const;
	typedef void (Item::*Setter)(const std::string&);

public:
	StringAccessor(Getter getter, Setter setter);

	Qt::ItemFlags flags(const Item* item) const;
	QVariant data(const Item* item, int role) const;
	bool setData(Item* item, const QVariant& value, int role) const;

private:
	Getter getter;
	Setter setter;
};

#endif
