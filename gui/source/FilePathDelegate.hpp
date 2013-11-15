#ifndef FilePathDelegate_hpp
#define FilePathDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class FilePathDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	FilePathDelegate(QWidget* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif
