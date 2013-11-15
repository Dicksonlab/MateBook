#include <QtGui>

#include <cmath>
#include <set>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <limits>
#include "GroupsTab.hpp"
#include "ArenaItem.hpp"
#include "RuntimeError.hpp"
#include "makeColorMap.hpp"
#include "GraphicsPrimitives.hpp"
#include "TimeDelegate.hpp"
#include "DateDelegate.hpp"
#include "GenderDelegate.hpp"
#include "FilePathDelegate.hpp"
#include "FilePathNameDelegate.hpp"
#include "ArenasApprovedDelegate.hpp"
#include "Project.hpp"
#include "ItemTree.hpp"
#include "GroupTree.hpp"
#include "GroupItem.hpp"
#include "../../common/source/debug.hpp"
#include "../../common/source/mathematics.hpp"

GroupsTab::GroupsTab(QWidget* parent) : AbstractTab(parent),
	currentProject(NULL)
{
	QHBoxLayout* viewsAndButtonsLayout = new QHBoxLayout;

	{	// views
		QSplitter* splitter = new QSplitter;
		splitter->setOrientation(Qt::Vertical);

		groupsView = new QTreeView;
		splitter->addWidget(groupsView);

		testsView = new QTreeView;
		splitter->addWidget(testsView);

		viewsAndButtonsLayout->addWidget(splitter, 1);
	}

	{	// buttons
		QVBoxLayout* buttonsLayout = new QVBoxLayout;
		buttonsLayout->setAlignment(Qt::AlignTop);

		QGroupBox* permutationTestGroupBox = new QGroupBox(tr("Permutation Test"));
		permutationTestGroupBox->setFlat(true);
		QVBoxLayout* permutationTestGroupBoxLayout = new QVBoxLayout;

		QFormLayout* formLayout = new QFormLayout;

		searchLineEdit = new QLineEdit;
		formLayout->addRow(QString("Search: "), searchLineEdit);
		formLayout->itemAt(formLayout->count() - 2)->widget()->setToolTip("Regular expression search in the group name.");
		formLayout->itemAt(formLayout->count() - 1)->widget()->setToolTip("Regular expression search in the group name.");

		replaceLineEdit = new QLineEdit;
		formLayout->addRow(QString("Replace: "), replaceLineEdit);
		formLayout->itemAt(formLayout->count() - 2)->widget()->setToolTip("Regular expression replacement in the group name.");
		formLayout->itemAt(formLayout->count() - 1)->widget()->setToolTip("Regular expression replacement in the group name.");

		roundsSpinBox = new QSpinBox;
		roundsSpinBox->setRange(1, 1000000);
		roundsSpinBox->setValue(10000);
		formLayout->addRow(QString("Rounds: "), roundsSpinBox);
		formLayout->itemAt(formLayout->count() - 2)->widget()->setToolTip("Number of rounds to simulate for each pair of groups.");
		formLayout->itemAt(formLayout->count() - 1)->widget()->setToolTip("Number of rounds to simulate for each pair of groups.");

		permutationTestGroupBoxLayout->addLayout(formLayout);

		QPushButton* runButton = new QPushButton("Run");
		runButton->setToolTip("Run the statistical test with the given settings");
		permutationTestGroupBoxLayout->addWidget(runButton);
		connect(runButton, SIGNAL(clicked()), this, SLOT(runPermutationTest()));

		permutationTestGroupBox->setLayout(permutationTestGroupBoxLayout);
		buttonsLayout->addWidget(permutationTestGroupBox);

		QScrollArea* buttonsScrollArea = new QScrollArea;
		buttonsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		buttonsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		QWidget* dummyScrollAreaWidget = new QWidget;
		dummyScrollAreaWidget->setLayout(buttonsLayout);
		buttonsScrollArea->setWidget(dummyScrollAreaWidget);
		viewsAndButtonsLayout->addWidget(buttonsScrollArea);
	}

	setLayout(viewsAndButtonsLayout);
}

GroupsTab::~GroupsTab()
{
}

QSize GroupsTab::minimumSizeHint() const
{
	return QSize(160, 120);
}

QSize GroupsTab::sizeHint() const
{
	return QSize(640, 480);
}

void GroupsTab::setProject(Project* project)
{
	groupsView->setModel(project->getGroupTree());
//	testsView->setModel(project->getTestTree());

	currentProject = project;
}

void GroupsTab::setCurrentItem(Item* item)
{
}

void GroupsTab::enter()
{
}

void GroupsTab::leave()
{
}

void GroupsTab::cut()
{
	/*	//TODO: see which kind of cut/copy/paste actually makes sense here
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}
	QApplication::clipboard()->setText(getCurrentProject()->data(currentIndex).toString());
	getCurrentProject()->setData(currentIndex, QVariant());
	*/
}

void GroupsTab::copy()
{
	/*	//TODO: see which kind of cut/copy/paste actually makes sense here
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}
	QApplication::clipboard()->setText(getCurrentProject()->data(currentIndex).toString());
	*/
}

void GroupsTab::paste()
{
	/*	//TODO: see which kind of cut/copy/paste actually makes sense here
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}
	getCurrentProject()->setData(currentIndex, QApplication::clipboard()->text());
	*/
}

void GroupsTab::del()
{
}

void GroupsTab::runPermutationTest()
{
	if (!currentProject) {
		return;
	}

	QString resultFileName(currentProject->getDirectory().canonicalPath() + "/" + "permutationTest_" + QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz") + "_" + QString::number(qrand()) + ".tsv");
	std::ofstream resultFile(resultFileName.toStdString().c_str());
	char separator = '\t';
	resultFile <<
		"1st Group" << separator << "Count" << separator << "Mean" << separator << "Standard Deviation" << separator << "S.E.M." << separator <<
		"2nd Group" << separator << "Count" << separator << "Mean" << separator << "Standard Deviation" << separator << "S.E.M." << separator <<
		"Learning Index" << separator << "p" << '\n';

	const size_t rounds = roundsSpinBox->value();

	//TODO: this should not be hardcoded
	const size_t maleCourtingIndex = 18;	// the column in the TSV file

	//TODO: this should be implemented with caching...there's a lot of overhead in reading the means for each pair
	const std::vector<AbstractGroupItem*>& groups = currentProject->getGroupTree()->getGroups();
	const std::map<QString, size_t>& groupIndexes = currentProject->getGroupTree()->getGroupIndexes();

	// regular expression search/replace and group groups
	QRegExp regExp(searchLineEdit->text());
	QString replaceString = replaceLineEdit->text();
	std::map<QString, std::set<size_t> > groupedGroups;	// maps the replaced name to the index
	for (std::map<QString, size_t>::const_iterator iter = groupIndexes.begin(); iter != groupIndexes.end(); ++iter) {
		QString replaced = iter->first;
		replaced.replace(regExp, replaceString);
		groupedGroups[replaced].insert(iter->second);
	}

	// we're defining them outside to avoid reallocations
	std::vector<float> shuffledIndexes;
	std::vector<float> naiveIndexes;
	std::vector<float> trainedIndexes;

	for (std::map<QString, std::set<size_t> >::const_iterator iter = groupedGroups.begin(); iter != groupedGroups.end(); ++iter) {
		std::set<size_t> thisGroupIndexes = iter->second;
		for (std::set<size_t>::const_iterator firstIter = thisGroupIndexes.begin(); firstIter != thisGroupIndexes.end(); ++firstIter) {
			GroupItem* firstGroup = assert_cast<GroupItem*>(groups[*firstIter]);
			naiveIndexes = firstGroup->getMeans(maleCourtingIndex);
			float naiveMean = mean(naiveIndexes);

			for (std::set<size_t>::const_iterator secondIter = thisGroupIndexes.begin(); secondIter != thisGroupIndexes.end(); ++secondIter) {
				GroupItem* secondGroup = assert_cast<GroupItem*>(groups[*secondIter]);
				trainedIndexes = secondGroup->getMeans(maleCourtingIndex);
				float trainedMean = mean(trainedIndexes);
				float learningIndex = 1 - trainedMean / naiveMean;

				shuffledIndexes = naiveIndexes;
				shuffledIndexes.insert(shuffledIndexes.end(), trainedIndexes.begin(), trainedIndexes.end());

				size_t exceededCount = 0;	// how often the random learning index exceeded the actual one
				for (size_t round = 0; round < rounds; ++round) {
					std::random_shuffle(shuffledIndexes.begin(), shuffledIndexes.end());
					float firstMean = mean(shuffledIndexes.begin(), shuffledIndexes.begin() + naiveIndexes.size());
					float secondMean = mean(shuffledIndexes.begin() + naiveIndexes.size(), shuffledIndexes.end());
					float randomLearningIndex = 1 - secondMean / firstMean;
					if (randomLearningIndex >= learningIndex) {
						++exceededCount;
					}
				}

				float p = static_cast<float>(exceededCount) / static_cast<float>(rounds);

				if (naiveIndexes.size() >= 2) {
					resultFile <<
						firstGroup->getName().toStdString() << separator <<
						naiveIndexes.size() << separator <<
						naiveMean << separator <<
						stddev(naiveIndexes) << separator <<
						sem(naiveIndexes) << separator;
				} else {
					resultFile <<
						firstGroup->getName().toStdString() << separator <<
						naiveIndexes.size() << separator <<
						naiveMean << separator <<
						"" << separator <<
						"" << separator;
				}
				if (trainedIndexes.size() >= 2) {
					resultFile <<
						secondGroup->getName().toStdString() << separator <<
						trainedIndexes.size() << separator <<
						trainedMean << separator <<
						stddev(trainedIndexes) << separator <<
						sem(trainedIndexes) << separator;
				} else {
					resultFile <<
						secondGroup->getName().toStdString() << separator <<
						trainedIndexes.size() << separator <<
						trainedMean << separator <<
						"" << separator <<
						"" << separator;
				}
				resultFile << learningIndex << separator << p << '\n';
			}
		}
	}
	resultFile.close();
	QDesktopServices::openUrl(QUrl("file:///" + resultFileName));
}

void GroupsTab::toBeDeleted(FileItem* fileItem)
{
}
/*
ArenaItem* GroupsTab::getSelectedArenaItem() const
{
	QModelIndex currentIndex = videoView->selectionModel()->currentIndex();
	if (!currentIndex.isValid() || !currentIndex.parent().isValid()) {	// checks if a child is selected or not
		return NULL;
	}
	return static_cast<ArenaItem*>(getItem(currentIndex));
}

Item* GroupsTab::getItem(const QModelIndex& index) const
{
	if (!index.isValid()) {
		throw std::runtime_error("invalid index");
	}
//	return static_cast<Item*>(proxyModel->mapToSource(index).internalPointer());	//TODO: remove this if the code below works
	// here we assume that no proxy remaps column 0 and that it contains the address of the Item
	return static_cast<Item*>(index.sibling(index.row(), 0).data(Qt::UserRole).value<void*>());
}
*/