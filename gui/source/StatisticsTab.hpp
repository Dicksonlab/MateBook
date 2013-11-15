#ifndef StatisticsTab_hpp
#define StatisticsTab_hpp

#include "AbstractTab.hpp"
#include "../../grapher/source/QGLGrapher.hpp"

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QVBoxLayout;
class QPlainTextEdit;
class QPushButton;
class QTableView;
class QStandardItemModel;
class QStringList;
class QComboBox;
class QModelIndex;
QT_END_NAMESPACE

// a class to visualize statistical results of songs and videos
class StatisticsTab : public AbstractTab
{
	Q_OBJECT

public:
	StatisticsTab(QWidget* parent = 0);
	virtual ~StatisticsTab();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

	void setProject(Project* project);
	void setCurrentItem(Item* item = NULL);

public slots:
	void enter();
	void leave();

	void cut();
	void copy();
	void paste();
	void del();

	void toBeDeleted(Item* item);
	void toBeClosed(boost::shared_ptr<Project> currentProject);

	void openStatisticFile();
	void changeDisplayMode(int index);
	void selectBar(const QModelIndex& index);
	void selectedIndexesChanged(std::pair<int, int> indexRange);

private:
	Project* currentProject;

	QPushButton* btnLoadStatFile;
	QComboBox* displayMode;

	QGLGrapher* plotStatistics;
	QTableView* fileTable;
	QTableView* binTable;
	QStandardItemModel* fileTableModel;
	QStandardItemModel* binTableModel;
	
	void loadStatisticalData(const QString& fileName);
	void loadStatisticsFile(const std::string& fileName);
	void loadGraphicalData(const char* fileName, std::vector< std::vector<float> >& data, QStringList& bins, QStringList& names);
	void drawBarChart(const std::string& fileName);
};

#endif
