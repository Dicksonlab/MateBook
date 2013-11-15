#ifndef GenderDelegate_hpp
#define GenderDelegate_hpp

#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class GenderDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	GenderDelegate(QWidget* parent = 0);

	QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	void setEditorData(QWidget* editor, const QModelIndex& index) const;
	void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};

#endif
