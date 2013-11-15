#ifndef AddressAccessor_hpp
#define AddressAccessor_hpp

#include "Accessor.hpp"

template<class ITEMBASE>
class AddressAccessor : public Accessor<ITEMBASE> {
public:
	AddressAccessor() : Accessor<ITEMBASE>()
	{
	}

	Qt::ItemFlags flags(const ITEMBASE* item) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	QVariant data(const ITEMBASE* item, int role) const
	{
		if (role == Qt::DisplayRole) {
			return QString::number((qulonglong)item, 16);
		} else if (role == Qt::BackgroundRole) {
			return this->getBackgroundColor(item);
		} else if (role == Qt::UserRole) {
			return QVariant::fromValue((void*)item);
		}
		
		return QVariant();
	}

	bool setData(ITEMBASE* item, const QVariant& value, int role) const
	{
		return false;
	}

private:
};

#endif
