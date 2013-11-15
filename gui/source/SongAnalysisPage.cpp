#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QFormLayout>

#include <map>
#include <fstream>
#include <string>
#include <stdexcept>

#include "SongAnalysisPage.hpp"
#include "../../common/source/Settings.hpp"

SongAnalysisPage::SongAnalysisPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	allOptions.reserve(10); // 10 options
	// IPI Options
	QLabel *lblMinITrainLength = new QLabel(tr("Min. train length (pulses):"));
	QSpinBox* spinbMinITrainLength = new QSpinBox();
	spinbMinITrainLength->setMinimum(1);
	spinbMinITrainLength->setMaximum(1000);
	spinbMinITrainLength->setValue(3);
	spinbMinITrainLength->setToolTip("IPI:MinTrainLength");
	allOptions.push_back(spinbMinITrainLength);

	QLabel *lblMinIPIdistance = new QLabel(tr("Minimal IPI distance (ms):"));
	QSpinBox* spinbMinIPIdistance = new QSpinBox();
	spinbMinIPIdistance->setMaximum(1000);
	spinbMinIPIdistance->setValue(15);
	spinbMinIPIdistance->setToolTip("MinIPIdistance");
	allOptions.push_back(spinbMinIPIdistance);

	QLabel *lblMaxIPIdistance = new QLabel(tr("Maximal IPI distance (ms):"));
	QSpinBox* spinbMaxIPIdistance = new QSpinBox();
	spinbMaxIPIdistance->setMaximum(1000);
	spinbMaxIPIdistance->setValue(80);
	spinbMaxIPIdistance->setToolTip("MaxIPIdistance");
	allOptions.push_back(spinbMaxIPIdistance);

	QLabel *lblIPIbin = new QLabel(tr("IPI bin size (ms):"));
	QSpinBox* spinbIPIbin = new QSpinBox();
	spinbIPIbin->setMinimum(1);
	spinbIPIbin->setMaximum(1000);
	spinbIPIbin->setValue(5); 
	spinbIPIbin->setToolTip("IPIbin");
	allOptions.push_back(spinbIPIbin);

	QVBoxLayout* vertIPIOLayout = new QVBoxLayout;
	vertIPIOLayout->setAlignment(Qt::AlignTop);
	vertIPIOLayout->addWidget(lblMinITrainLength);
	vertIPIOLayout->addWidget(spinbMinITrainLength);
	vertIPIOLayout->addWidget(lblMinIPIdistance);
	vertIPIOLayout->addWidget(spinbMinIPIdistance);
	vertIPIOLayout->addWidget(lblMaxIPIdistance);
	vertIPIOLayout->addWidget(spinbMaxIPIdistance);
	vertIPIOLayout->addWidget(lblIPIbin);
	vertIPIOLayout->addWidget(spinbIPIbin);

	QGroupBox* grpIPIOptions = new QGroupBox("IPI Options");
	grpIPIOptions->setLayout(vertIPIOLayout);

	// Train Options
	QLabel *lblMinTTrainLength = new QLabel(tr("Min. train length (pulses):"));
	QSpinBox* spinbMinTTrainLength = new QSpinBox();
	spinbMinTTrainLength->setMinimum(1);
	spinbMinTTrainLength->setMaximum(1000);
	spinbMinTTrainLength->setValue(2);
	spinbMinTTrainLength->setToolTip("Train:MinTrainLength");
	allOptions.push_back(spinbMinTTrainLength);

	QLabel *lblTrainbin = new QLabel(tr("Train bin size (pulses):"));
	QSpinBox* spinbTrainbin = new QSpinBox();
	spinbTrainbin->setMinimum(1);
	spinbTrainbin->setMaximum(1000);
	spinbTrainbin->setValue(5);
	spinbTrainbin->setToolTip("Trainbin");
	allOptions.push_back(spinbTrainbin);

	QLabel *lblPulsePerMinExclude = new QLabel(tr("Pulse/min excluding pulses in groups of:"));
	QSpinBox* spinbPulsePerMin = new QSpinBox();
	spinbPulsePerMin->setMinimum(1);
	spinbPulsePerMin->setMaximum(1000);
	spinbPulsePerMin->setValue(1);
	spinbPulsePerMin->setToolTip("PulsePerMinuteExclude");
	allOptions.push_back(spinbPulsePerMin);

	QVBoxLayout* vertTrainOLayout = new QVBoxLayout;
	vertTrainOLayout->setAlignment(Qt::AlignTop);
	vertTrainOLayout->addWidget(lblMinTTrainLength);
	vertTrainOLayout->addWidget(spinbMinTTrainLength);
	vertTrainOLayout->addWidget(lblTrainbin);
	vertTrainOLayout->addWidget(spinbTrainbin);
	vertTrainOLayout->addWidget(lblPulsePerMinExclude);
	vertTrainOLayout->addWidget(spinbPulsePerMin);

	QGroupBox* grpTrainOptions = new QGroupBox("Train Options");
	grpTrainOptions->setLayout(vertTrainOLayout);

	// Cycles Options
	QLabel *lblMinCTrainLength = new QLabel(tr("Min. train length (pulses):"));
	QSpinBox* spinbMinCTrainLength = new QSpinBox();
	spinbMinCTrainLength->setMinimum(1);
	spinbMinCTrainLength->setMaximum(1000);
	spinbMinCTrainLength->setValue(2);
	spinbMinCTrainLength->setToolTip("Cycle:MinTrainLength");
	allOptions.push_back(spinbMinCTrainLength);

	QLabel *lblMinCyclesNumber = new QLabel(tr("Min. number of cycles (#):"));
	QSpinBox* spinbMinCyclesNumber = new QSpinBox();
	spinbMinCyclesNumber->setMinimum(0);
	spinbMinCyclesNumber->setMaximum(1000);
	spinbMinCyclesNumber->setValue(1);
	spinbMinCyclesNumber->setToolTip("MinCycleNumber");
	allOptions.push_back(spinbMinCyclesNumber);

	QVBoxLayout* vertCyclesOLayout = new QVBoxLayout;
	vertCyclesOLayout->setAlignment(Qt::AlignTop);
	vertCyclesOLayout->addWidget(lblMinCTrainLength);
	vertCyclesOLayout->addWidget(spinbMinCTrainLength);
	vertCyclesOLayout->addWidget(lblMinCyclesNumber);
	vertCyclesOLayout->addWidget(spinbMinCyclesNumber);

	QGroupBox* grpCyclesOptions = new QGroupBox("Cycle Options");
	grpCyclesOptions->setLayout(vertCyclesOLayout);

	QHBoxLayout* optionLayout = new QHBoxLayout;
	optionLayout->addWidget(grpIPIOptions);
	optionLayout->addWidget(grpTrainOptions);
	optionLayout->addWidget(grpCyclesOptions);

	// batch options
	QSpinBox* spinbMinIPInumber = new QSpinBox();
	spinbMinIPInumber->setMaximum(1000);
	spinbMinIPInumber->setValue(10);
	spinbMinIPInumber->setToolTip("Batch:MinIPInumber");
	allOptions.push_back(spinbMinIPInumber);

	QSpinBox* spinbMinCyclePulseNumber = new QSpinBox();
	spinbMinCyclePulseNumber->setMaximum(1000);
	spinbMinCyclePulseNumber->setValue(10);
	spinbMinCyclePulseNumber->setToolTip("Batch:MinPulseNumber");
	allOptions.push_back(spinbMinCyclePulseNumber);

	QSpinBox* spinbMinTrainNumber = new QSpinBox();
	spinbMinTrainNumber->setMaximum(1000);
	spinbMinTrainNumber->setValue(10);
	spinbMinTrainNumber->setToolTip("Batch:MinTrainNumber");
	allOptions.push_back(spinbMinTrainNumber);

	QFormLayout* batchOptionsLayout = new QFormLayout;
	batchOptionsLayout->addRow(tr("Min. number of pulses (#):"), spinbMinCyclePulseNumber);
	batchOptionsLayout->addRow(tr("Min. number of IPIs (#):"), spinbMinIPInumber);
	batchOptionsLayout->addRow(tr("Min. number of trains (#):"), spinbMinTrainNumber);

	QGroupBox* grpBatchOptions = new QGroupBox("Batchrun Options");
	grpBatchOptions->setLayout(batchOptionsLayout);
	grpBatchOptions->setToolTip("minimum numbers a song must have to be included in the calculations:\n number of pulses: concerns the CPP calculation and pulses per minute");
	// buttons
	const int width = 80;
	btnLoadOptions = new QPushButton("Load...");
	btnLoadOptions->setFixedWidth(width);
	btnSaveOptions = new QPushButton("Save...");
	btnSaveOptions->setFixedWidth(width);

	QHBoxLayout* buttonLayout = new QHBoxLayout;
	buttonLayout->addWidget(btnLoadOptions, 0, Qt::AlignRight);
	buttonLayout->addWidget(btnSaveOptions);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addLayout(optionLayout);
	layout->addWidget(grpBatchOptions);
	layout->addLayout(buttonLayout);
	
	setLayout(layout);

	connect(btnLoadOptions, SIGNAL(clicked(bool)), this, SLOT(loadOptions()));
	connect(btnSaveOptions, SIGNAL(clicked(bool)), this, SLOT(saveOptions()));
}

QSize SongAnalysisPage::minimumSizeHint() const
{
	return QSize(160, 60);
}

QSize SongAnalysisPage::sizeHint() const
{
	return QSize(100, 100);
}

std::map<QString, float> SongAnalysisPage::getOptions()
{
	std::map<QString, float> options;
	for(std::vector<QAbstractSpinBox*>::iterator iter = allOptions.begin(); iter != allOptions.end(); ++iter){
		QSpinBox* spinB = static_cast<QSpinBox*>((*iter));
		options[spinB->toolTip()] = spinB->value();
	}
	return options;
}

void SongAnalysisPage::setOptions(std::map<QString, float> options)
{
	for(std::vector<QAbstractSpinBox*>::iterator iter = allOptions.begin(); iter != allOptions.end(); ++iter){
		QSpinBox* spinB = static_cast<QSpinBox*>((*iter));
		spinB->setValue(options[spinB->toolTip()]);
	}
}

void SongAnalysisPage::loadOptions()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Analysis Options"), "../", tr("Option Files (*.txt)"));
	
	if(!fileName.isEmpty()){
		std::ifstream file (std::string(fileName.toStdString()).c_str(), std::ios::in);
		std::string line;
		std::map <QString, float> options;

		if(file.is_open()){
			while(file.good()){
				getline(file, line);
				if(!file.eof()){
					QString temp(line.c_str());
					QStringList list = temp.split('\t');
					if(list.size() == 2){
						try{
							options[list[0]] = list[1].toFloat();
						}catch(std::runtime_error &e){
							QMessageBox::critical(this, tr("Loading Settings"), e.what());
						}
					}else{
						QMessageBox::critical(this, tr("Loading Settings"), "Wrong file format");
					}
				}
			}
			file.close();
		}

		for(std::vector<QAbstractSpinBox*>::const_iterator iter = allOptions.begin(); iter != allOptions.end(); ++iter){
			static_cast<QSpinBox*>((*iter))->setValue(options[static_cast<QSpinBox*>((*iter))->toolTip()]);
		}
	}
}

void SongAnalysisPage::saveOptions()
{
	std::map<QString, float> options = getOptions();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Analysis Options"), "../", tr("Option Files (*.txt)"));
	
	if(!fileName.isEmpty()){
		std::ofstream file(std::string(fileName.toStdString()).c_str(), std::ios::out);
		try{
			if(file.is_open() && file.good()){
				for(std::map<QString, float>::const_iterator it = options.begin(); it != options.end(); ++it){
					file << (*it).first.toStdString() << '\t' << (*it).second << '\n';
				}
			}
		}catch(std::runtime_error &e){
			QMessageBox::critical(this, tr("Saving Settings"), e.what());
		}
	}
}