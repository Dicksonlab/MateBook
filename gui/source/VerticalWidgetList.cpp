#include <QtGui>

#include "VerticalWidgetList.hpp"

VerticalWidgetList::VerticalWidgetList(boost::function<QWidget* ()> widgetCreator, QWidget* parent) : QWidget(parent),
	widgetCreator(widgetCreator)
{
	verticalLayout = new QVBoxLayout;
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	verticalLayout->setSpacing(0);
	this->setLayout(verticalLayout);
}

VerticalWidgetList::~VerticalWidgetList()
{
	setWidgetCount(0);
}

QSize VerticalWidgetList::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize VerticalWidgetList::sizeHint() const
{
	return QSize(640, 480);
}

QWidget* VerticalWidgetList::getWidget(int widgetIndex)
{
	QWidget* dummy = verticalLayout->itemAt(widgetIndex)->widget();
	return dummy->layout()->itemAt(0)->widget();
}

int VerticalWidgetList::count() const
{
	return verticalLayout->count();
}

void VerticalWidgetList::setContentAlignment(Qt::Alignment alignment)
{
	verticalLayout->setAlignment(alignment);
}

void VerticalWidgetList::setWidgetCount(int widgetCount)
{
	if (widgetCount < 0) {
		widgetCount = 0;
	}

	while (count() < widgetCount) {
		QWidget* dummy = makeDummy();
		if (!dummy) {
			return;
		}
		verticalLayout->addWidget(dummy);
	}

	while (count() > widgetCount) {
		QLayoutItem* toBeDeleted = verticalLayout->takeAt(count() - 1);
		toBeDeleted->widget()->deleteLater();
	}
}

void VerticalWidgetList::removeRow()
{
	QObject* thisSender = sender();
	QWidget* dummy = thisSender->property("dummyPointer").value<QWidget*>();
	verticalLayout->removeWidget(dummy);
	dummy->deleteLater();
}

void VerticalWidgetList::addRow()
{
	QObject* thisSender = sender();
	QWidget* senderDummy = thisSender->property("dummyPointer").value<QWidget*>();
	int senderDummyIndex = verticalLayout->indexOf(senderDummy);

	QWidget* newDummy = makeDummy();
	if (!newDummy) {
		return;
	}
	verticalLayout->insertWidget(senderDummyIndex + 1, newDummy);
}

void VerticalWidgetList::moveRowUp()
{
	QObject* thisSender = sender();
	QWidget* senderDummy = thisSender->property("dummyPointer").value<QWidget*>();
	int senderDummyIndex = verticalLayout->indexOf(senderDummy);

	if (senderDummyIndex > 0) {
		verticalLayout->removeWidget(senderDummy);
		verticalLayout->insertWidget(senderDummyIndex - 1, senderDummy);
	}
}

void VerticalWidgetList::moveRowDown()
{
	QObject* thisSender = sender();
	QWidget* senderDummy = thisSender->property("dummyPointer").value<QWidget*>();
	int senderDummyIndex = verticalLayout->indexOf(senderDummy);

	if (senderDummyIndex + 1 < count()) {
		verticalLayout->removeWidget(senderDummy);
		verticalLayout->insertWidget(senderDummyIndex + 1, senderDummy);
	}
}

QWidget* VerticalWidgetList::makeDummy() const
{
	QWidget* newWidget = widgetCreator();
	if (!newWidget) {
		return NULL;
	}
	QWidget* dummy = new QWidget;	// the top-level widget per row in the verticalLayout
	QGridLayout* buttons = new QGridLayout;
	{
		QPushButton* minus = new QPushButton(QIcon(":/mb/icons/minus.png"), "");
		minus->setToolTip("Remove this row.");
		minus->setProperty("widgetPointer", QVariant::fromValue(newWidget));
		minus->setProperty("dummyPointer", QVariant::fromValue(dummy));
		connect(minus, SIGNAL(clicked()), this, SLOT(removeRow()));
		minus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		buttons->addWidget(minus, 0, 0, Qt::AlignBottom);
	}
	{
		QPushButton* plus = new QPushButton(QIcon(":/mb/icons/plus.png"), "");
		plus->setToolTip("Add another row below this one.");
		plus->setProperty("widgetPointer", QVariant::fromValue(newWidget));
		plus->setProperty("dummyPointer", QVariant::fromValue(dummy));
		connect(plus, SIGNAL(clicked()), this, SLOT(addRow()));
		plus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		buttons->addWidget(plus, 1, 0, Qt::AlignTop);
	}
	{
		QPushButton* up = new QPushButton(QIcon(":/mb/icons/up.png"), "");
		up->setToolTip("Move this row up.");
		up->setProperty("widgetPointer", QVariant::fromValue(newWidget));
		up->setProperty("dummyPointer", QVariant::fromValue(dummy));
		connect(up, SIGNAL(clicked()), this, SLOT(moveRowUp()));
		up->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		buttons->addWidget(up, 0, 1, Qt::AlignBottom);
	}
	{
		QPushButton* down = new QPushButton(QIcon(":/mb/icons/down.png"), "");
		down->setToolTip("Move this row down.");
		down->setProperty("widgetPointer", QVariant::fromValue(newWidget));
		down->setProperty("dummyPointer", QVariant::fromValue(dummy));
		connect(down, SIGNAL(clicked()), this, SLOT(moveRowDown()));
		down->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		buttons->addWidget(down, 1, 1, Qt::AlignTop);
	}
	QHBoxLayout* widgetAndButtons = new QHBoxLayout;
	widgetAndButtons->setContentsMargins(0, 0, 0, 0);
	widgetAndButtons->setSpacing(0);
	widgetAndButtons->addWidget(newWidget);
	widgetAndButtons->addLayout(buttons);
	dummy->setLayout(widgetAndButtons);

	return dummy;
}
