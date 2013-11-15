#include "FancyMenu.hpp"

FancyMenu::FancyMenu(QWidget* parent) : QMenu(parent)
{
}

FancyMenu::~FancyMenu()
{
}

QAction* FancyMenu::exec(const QPoint& p, const QModelIndex& currentIndex, const QModelIndexList& selection, QAction* action)
{
	return QMenu::exec(p, action);
}