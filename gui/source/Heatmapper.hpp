#ifndef Heatmapper_hpp
#define Heatmapper_hpp

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class QGroupBox;
QT_END_NAMESPACE

class VerticalWidgetList;
class ArenaItem;

class Heatmapper : public QWidget
{
	Q_OBJECT

public:
	Heatmapper(QWidget* parent = 0);
	virtual ~Heatmapper();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	QImage drawHeatmap(const std::vector<ArenaItem*>& arenaItems);

signals:
	void attributeChanged();
	void dimensionChanged(int index);

public slots:
	void reset();

private slots:
//	void attributeChanged();
//	void dimensionChanged(int index);

private:
	QString getAttributePath() const;

	QComboBox* attributeComboBox;
	QComboBox* activeFlyComboBox;
	QComboBox* passiveFlyComboBox;

	QSpinBox* imageWidthSpinBox;
	QSpinBox* imageHeightSpinBox;

	QSpinBox* horizontalBinsSpinBox;
	QSpinBox* verticalBinsSpinBox;

	QDoubleSpinBox* horizontalScaleFactorSpinBox;
	QDoubleSpinBox* verticalScaleFactorSpinBox;

	QDoubleSpinBox* centerXSpinBox;
	QDoubleSpinBox* centerYSpinBox;

	QSpinBox* lowestCountSpinBox;
	QSpinBox* highestCountSpinBox;

	QComboBox* colorMapComboBox;
	QComboBox* mappingComboBox;

	VerticalWidgetList* filterList;
};

#endif
