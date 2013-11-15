#include <QLineEdit>
#include <QFileDialog>
#include <QPushButton>

#include "FilePathDelegate.hpp"
#include "PathSelector.hpp"

FilePathDelegate::FilePathDelegate(QWidget* parent) : QStyledItemDelegate(parent)
{
}

QWidget* FilePathDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	PathSelector* editor = new PathSelector(parent);
	return editor;
}