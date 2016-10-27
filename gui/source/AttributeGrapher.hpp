#ifndef AttributeGrapher_hpp
#define AttributeGrapher_hpp

#include <QWidget>
#include <boost/shared_ptr.hpp>
#include "Video.hpp"
#include "VideoPlayer.hpp"
#include "TrackingResults.hpp"
#include "../../grapher/source/QGLGrapher.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class QGroupBox;
QT_END_NAMESPACE

class ArenaItem;
class QGLGrapher;

class AttributeGrapher : public QWidget
{
	Q_OBJECT

public:
	AttributeGrapher(QWidget* parent = 0);
	virtual ~AttributeGrapher();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	size_t getNumValues() const;
	size_t getCenter() const;

signals:
	void plotCenterChanged(size_t newCenter);
	void plotZoomChanged(size_t newNumValues);

public slots:
	void reset();
	void setCurrentResults(boost::shared_ptr<TrackingResults> trackingResults);	// must be called whenever a new set of results is to be displayed
    void setCenter(size_t frame);
    void setNumValues(size_t numValues);
	void setAntiAliasing(bool enable);
	void setRotated(bool rotated);
	void setFontSize(int pixel);
	void attributeChanged();
	void dimensionChanged(int index);

private:
	boost::shared_ptr<TrackingResults> currentResults;
	std::vector<QVector4D> flyColors;

	QGroupBox* frame;
	QGLGrapher* plot;
//	QComboBox* unitComboBox;
//	QComboBox* flyComboBox;
	QComboBox* attributeComboBox;
	QComboBox* dimensionComboBox;
	const char delimiter = ':';
};

#endif
