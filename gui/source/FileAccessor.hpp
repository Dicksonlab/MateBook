#ifndef FileAccessor_hpp
#define FileAccessor_hpp

#include "Accessor.hpp"

/**
  * @class  FileAccessor
  * @brief  to get the video file of a Item
  */
class FileAccessor : public Accessor<Item> {
public:
	FileAccessor();

	Qt::ItemFlags flags(const Item* item) const;
	QVariant data(const Item* item, int role) const;
	bool setData(Item* item, const QVariant& value, int role) const;

private:
};

#endif
