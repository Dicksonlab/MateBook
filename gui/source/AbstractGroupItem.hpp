#ifndef AbstractGroupItem_hpp
#define AbstractGroupItem_hpp

#include <QList>
#include <QVariant>
#include <QPixmap>
#include <QTime>
#include <QDir>
#include <QString>

#include <map>

#include <boost/shared_ptr.hpp>

#include "Item.hpp"

class MateBook;
class Project;

/**
  * @class  AbstractGroupItem
  * @brief  base class for things we want to display as a row in a GroupTree
  */
class AbstractGroupItem : public QObject {
	Q_OBJECT

public:
	virtual ~AbstractGroupItem();

	virtual AbstractGroupItem* parent() = 0;	//TODO: shouldn't this be const?
	virtual AbstractGroupItem* child(int row) const = 0;
	virtual int childCount() const = 0;
	virtual void removeChildren(int position, int count) = 0;
	virtual int childIndex() const = 0;	// the parent's child index for this item

	virtual void removedByUser() = 0;

	virtual QString getName() const = 0;
	virtual Item::VideoStage getCurrentVideoStage() const = 0;
	virtual Item::Status getCurrentVideoStatus() const = 0;
	virtual Item::AudioStage getCurrentAudioStage() const = 0;
	virtual Item::Status getCurrentAudioStatus() const = 0;

protected:
	AbstractGroupItem(MateBook* mateBook, Project* project);

	MateBook* getMateBook() const;
	Project* getProject() const;

signals:
	void itemChanged(AbstractGroupItem*);

private slots:
	void childItemChanged(AbstractGroupItem*);

private:
	MateBook* mateBook;
	Project* project;
};

#endif
