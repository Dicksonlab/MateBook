#include <QtGui>

#include <stdexcept>
#include "AbstractTab.hpp"

AbstractTab::AbstractTab(QWidget* parent) : QWidget(parent)
{
}

AbstractTab::~AbstractTab()
{
}

void AbstractTab::readGuiSettings(const QSettings& settings)
{
}

void AbstractTab::writeGuiSettings(QSettings& settings)
{
}

void AbstractTab::readHeaderSettings()
{
}

void AbstractTab::writeHeaderSettings()
{
}

void AbstractTab::viewWasModified()
{
	emit projectWasModified();
}