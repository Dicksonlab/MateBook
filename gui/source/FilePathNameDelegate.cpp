#include <QLineEdit>
#include <QFileDialog>
#include <QPushButton>

#include "FilePathNameDelegate.hpp"
#include "FileSelector.hpp"

FilePathNameDelegate::FilePathNameDelegate(QWidget* parent) : QStyledItemDelegate(parent)
{
}

QWidget* FilePathNameDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	FileSelector* editor = new FileSelector(parent);
	return editor;
}