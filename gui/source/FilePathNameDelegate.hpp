#ifndef FilePathNameDelegate_hpp
#define FilePathNameDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class FilePathNameDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	FilePathNameDelegate(QWidget* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif
