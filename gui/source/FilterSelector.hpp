#ifndef FilterSelector_hpp
#define FilterSelector_hpp

#include <QWidget>
#include <QDir>

#include <boost/shared_ptr.hpp>
#include "ArenaItem.hpp"
#include "../../tracker/source/Attribute.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class QGroupBox;
class QSpinBox;
class QDoubleSpinBox;
QT_END_NAMESPACE

class ColorButton;

class Filter {
public:
	Filter(QString attributeName, QString attributeKind, unsigned int activeFly, unsigned int passiveFly) :
		attributeName(attributeName),
		attributeKind(attributeKind),
		activeFly(activeFly),
		passiveFly(passiveFly)
	{
	}

	virtual std::vector<size_t> apply(const ArenaItem& arenaItem, const std::vector<size_t> indexes) = 0;

protected:
	QString getAttributePath() const
	{
		QString path("track/");
		path += attributeKind + "/";
		path += (attributeKind == "fly" || attributeKind == "pair") ? (QString::number(activeFly) + "/") : "";
		path += (attributeKind == "pair") ? (QString::number(passiveFly) + "/") : "";
		path += attributeName;
		return path;
	}

private:
	QString attributeName;
	QString attributeKind;
	unsigned int activeFly;
	unsigned int passiveFly;
};

template<class T>
class KeepEqual {
public:
	bool operator()(const T& left, const T& right)
	{
		return left == right;
	}
};

template<class T>
class KeepLessThan {
public:
	bool operator()(const T& left, const T& right)
	{
		return left < right;
	}
};

template<class T>
class KeepGreaterThan {
public:
	bool operator()(const T& left, const T& right)
	{
		return left > right;
	}
};

template<class T, template<class> class OP>	// use one of the templates defined above (e.g., KeepEqual) as template-template-parameters
class AttributeFilter : public Filter {
public:
	AttributeFilter(QString attributeName, QString attributeKind, unsigned int activeFly, unsigned int passiveFly, T operand) : Filter(attributeName, attributeKind, activeFly, passiveFly),
		operand(operand)
	{
	}

	std::vector<size_t> apply(const ArenaItem& arenaItem, const std::vector<size_t> indexes)
	{
		QString attributePath = arenaItem.absoluteDataDirectory().filePath(getAttributePath());
		Attribute<T> attribute;
		attribute.readBinaries(attributePath.toStdString());
		std::vector<size_t> passedIndexes;
		passedIndexes.reserve(indexes.size());
		for (std::vector<size_t>::const_iterator iter = indexes.begin(); iter != indexes.end(); ++iter) {
			if (*iter < attribute.size() && OP<T>()(attribute[*iter], operand)) {
				passedIndexes.push_back(*iter);
			}
		}
		return passedIndexes;
	}

private:
	T operand;
};

template<class S, size_t N, template<class> class OP>	// use one of the templates defined above (e.g., Equal) as template-template-parameters
class AttributeFilter<Vec<S, N>, OP> : public Filter {
public:
	AttributeFilter(QString attributeName, QString attributeKind, unsigned int activeFly, unsigned int passiveFly, S operand, unsigned int dimension) : Filter(attributeName, attributeKind, activeFly, passiveFly),
		operand(operand),
		dimension(dimension)
	{
		assert(dimension < N);
	}

	std::vector<size_t> apply(const ArenaItem& arenaItem, const std::vector<size_t> indexes)
	{
		QString attributePath = arenaItem.absoluteDataDirectory().filePath(getAttributePath());
		Attribute<Vec<S, N> > attribute;
		attribute.readBinaries(attributePath.toStdString());
		std::vector<size_t> passedIndexes;
		passedIndexes.reserve(indexes.size());
		for (std::vector<size_t>::const_iterator iter = indexes.begin(); iter != indexes.end(); ++iter) {
			if (*iter < attribute.size() && OP<S>()(attribute[*iter][dimension], operand)) {
				passedIndexes.push_back(*iter);
			}
		}
		return passedIndexes;
	}

private:
	S operand;
	unsigned int dimension;
};

class FilterSelector : public QWidget {
	Q_OBJECT

public:
	FilterSelector(QWidget* parent = 0);
	virtual ~FilterSelector();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	boost::shared_ptr<Filter> getFilter() const;

public slots:
	void reset();

private slots:
	void attributeChanged();
//	void dimensionChanged(int index);

private:
	QComboBox* attributeComboBox;
	QComboBox* dimensionComboBox;
	QComboBox* activeFlyComboBox;
	QComboBox* passiveFlyComboBox;
	QComboBox* operatorComboBox;
	QSpinBox* intSpinBox;
	QDoubleSpinBox* realSpinBox;
	QComboBox* unitComboBox;
};

#endif
