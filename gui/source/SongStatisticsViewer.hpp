#ifndef SongStatisticsViewer_hpp
#define SongStatisticsViewer_hpp

#include <QWidget>
#include <vector>
#include "../../grapher/source/QGLGrapher.hpp"

#include "SongResults.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QHBoxLayout;
class QVBoxLayout;
class QGroupBox;
class QLabel;
class QStringList;
QT_END_NAMESPACE

class ArenaItem;
class QGLGrapher;

class SongStatisticsViewer : public QWidget
{
	Q_OBJECT

public:
	SongStatisticsViewer(QWidget* parent = 0);
	virtual ~SongStatisticsViewer();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	/**
	 * method to set data at position of the data array
	 * @param  data the new Data to be displayed
	 * @param  binSize size of the bins
	 */
	void setGraph(const std::vector< SongResults::binStruct >& data, const QStringList& names);

	void setCurrentAttribute(QString attributeName);

signals:
	void plotCenterChanged(size_t newCenter);
	void plotZoomChanged(size_t newNumValues);

public slots:
	void clear();
	void setCurrentResults();	// must be called whenever a new set of results is to be displayed
    void center(size_t frame);
    void setNumValues(size_t numValues);
	void setAntiAliasing(bool enable);
	void setRotated(bool rotated);
	void setFontSize(int pixel);

private slots:
	void displayModeChanged(int index);

private:
	QGroupBox* frame;
	QGLGrapher* plot;
	QComboBox* ipiDisplayMode;
	QLabel* userInput;

	std::vector< SongResults::binStruct > data;
};

#endif
