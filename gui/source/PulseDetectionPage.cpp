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
#include <QCheckBox>

#include <map>
#include <fstream>
#include <string>
#include <stdexcept>

#include "PulseDetectionPage.hpp"

PulseDetectionPage::PulseDetectionPage(Settings& trackerSettings, QWidget* parent) : ConfigPage(trackerSettings, parent)
{
	allOptions.reserve(23); // 23 options
	// sine options
	QLabel *lblTapersBandWSize = new QLabel(tr("Tapers time-bandwidth:"));
	QDoubleSpinBox* spinbTapersTimeBandwith = new QDoubleSpinBox(); // standard-value: 12
	spinbTapersTimeBandwith->setValue(12);
	spinbTapersTimeBandwith->setToolTip("TapersTimeBandwith");
	allOptions.push_back(spinbTapersTimeBandwith);

	QLabel *lblIndTapersCount = new QLabel(tr("Independent tapers count:"));
	QDoubleSpinBox* spinbIdependentTapersCount = new QDoubleSpinBox(); // standard-value: 20
	spinbIdependentTapersCount->setValue(20);
	spinbIdependentTapersCount->setToolTip("IndependentTapersCount");
	allOptions.push_back(spinbIdependentTapersCount);

	QLabel *lblStepSize = new QLabel(tr("Window step size:"));
	QDoubleSpinBox* spinbSStepSize = new QDoubleSpinBox(); // standard-value: 0.01
	spinbSStepSize->setValue(0.01);
	spinbSStepSize->setToolTip("WindowStepSize");
	allOptions.push_back(spinbSStepSize);

	QLabel *lblWindowLength = new QLabel(tr("Window length:"));
	QDoubleSpinBox* spinbSWindowLength = new QDoubleSpinBox(); // standard-value: 0.1
	spinbSWindowLength->setValue(0.1);
	spinbSWindowLength->setToolTip("WindowLength");
	allOptions.push_back(spinbSWindowLength);

	QLabel *lblFCrit = new QLabel(tr("F-test criterion:"));
	QDoubleSpinBox* spinbSPval = new QDoubleSpinBox(); // standard-value: 0.05
	spinbSPval->setValue(0.05);
	spinbSPval->setToolTip("FTestCrit");
	allOptions.push_back(spinbSPval);

	QLabel *lblLowestSineFreq = new QLabel(tr("Lowest sine frequency:"));
	QDoubleSpinBox* spinbLowestSineFreq = new QDoubleSpinBox(); // standard-value: 100
	spinbLowestSineFreq->setMaximum(1000);
	spinbLowestSineFreq->setValue(100);
	spinbLowestSineFreq->setToolTip("LowestSineFS");
	allOptions.push_back(spinbLowestSineFreq);

	QLabel *lblHighestSineFreq = new QLabel(tr("Highest sine frequency:"));
	QDoubleSpinBox* spinbHighestSineFreq = new QDoubleSpinBox();	// standard-value: 300
	spinbHighestSineFreq->setMaximum(1000);
	spinbHighestSineFreq->setValue(300);
	spinbHighestSineFreq->setToolTip("HighestSineFS");
	allOptions.push_back(spinbHighestSineFreq);

	QLabel *lblSineRangePerc = new QLabel(tr("Sine search range:"));
	QDoubleSpinBox* spinbSineRangePerc = new QDoubleSpinBox(); // standard-value: 0.2
	spinbSineRangePerc->setValue(0.2);
	spinbSineRangePerc->setToolTip("SineRangePerc");
	allOptions.push_back(spinbSineRangePerc);

	QLabel *lblMinSineSize = new QLabel(tr("Minimal sine size:"));
	QDoubleSpinBox* spinbMinSineSize = new QDoubleSpinBox();	// standard-value: 3
	spinbMinSineSize->setValue(3);
	spinbMinSineSize->setToolTip("MinSineSize");
	allOptions.push_back(spinbMinSineSize);

	QVBoxLayout* vertSineOLayout = new QVBoxLayout;
	vertSineOLayout->setAlignment(Qt::AlignTop);
	vertSineOLayout->addWidget(lblTapersBandWSize);
	vertSineOLayout->addWidget(spinbTapersTimeBandwith);
	vertSineOLayout->addWidget(lblIndTapersCount);
	vertSineOLayout->addWidget(spinbIdependentTapersCount);
	vertSineOLayout->addWidget(lblStepSize);
	vertSineOLayout->addWidget(spinbSStepSize);
	vertSineOLayout->addWidget(lblWindowLength);
	vertSineOLayout->addWidget(spinbSWindowLength);
	vertSineOLayout->addWidget(lblFCrit);
	vertSineOLayout->addWidget(spinbSPval);
	vertSineOLayout->addWidget(lblLowestSineFreq);
	vertSineOLayout->addWidget(spinbLowestSineFreq);
	vertSineOLayout->addWidget(lblHighestSineFreq);
	vertSineOLayout->addWidget(spinbHighestSineFreq);
	vertSineOLayout->addWidget(lblSineRangePerc);
	vertSineOLayout->addWidget(spinbSineRangePerc);
	vertSineOLayout->addWidget(lblMinSineSize);
	vertSineOLayout->addWidget(spinbMinSineSize);

	QGroupBox* grpSineOptions = new QGroupBox("Sine Options");
	grpSineOptions->setLayout(vertSineOLayout);

	// train options
	QLabel *lblCutoff = new QLabel(tr("Noise cutoff quantile:"));
	QDoubleSpinBox* spinbCutoff = new QDoubleSpinBox();	// standard-value: 0.8
	spinbCutoff->setValue(0.8);
	spinbCutoff->setToolTip("NoiseCutofF");
	allOptions.push_back(spinbCutoff);

	QLabel *lblExpandRange = new QLabel(tr("Train expand range:"));
	QDoubleSpinBox* spinbExpandRange = new QDoubleSpinBox(); // standard-value: 1.3
	spinbExpandRange->setValue(1.3);
	spinbExpandRange->setToolTip("TrainExpandRange");
	allOptions.push_back(spinbExpandRange);

	QLabel *lblCombineTim = new QLabel(tr("Combine step size:"));
	QDoubleSpinBox* spinbCombineTime = new QDoubleSpinBox();	// standard-value: 15
	spinbCombineTime->setValue(15);
	spinbCombineTime->setToolTip("CombineStepSize");
	allOptions.push_back(spinbCombineTime);

	QVBoxLayout* vertTrainOLayout = new QVBoxLayout;
	vertTrainOLayout->setAlignment(Qt::AlignTop);
	vertTrainOLayout->addWidget(lblCutoff);
	vertTrainOLayout->addWidget(spinbCutoff);
	vertTrainOLayout->addWidget(lblExpandRange);
	vertTrainOLayout->addWidget(spinbExpandRange);
	vertTrainOLayout->addWidget(lblCombineTim);
	vertTrainOLayout->addWidget(spinbCombineTime);

	QGroupBox* grpTrainOptions = new QGroupBox("Train Options");
	grpTrainOptions->setLayout(vertTrainOLayout);

	// pulse options
	QLabel *lblCutoffFactor = new QLabel(tr("Cutoff Factor:"));
	QDoubleSpinBox* spinbCutoffFactor = new QDoubleSpinBox(); // standard-value: 5
	spinbCutoffFactor->setValue(5);
	spinbCutoffFactor->setToolTip("CutoffFactor");
	allOptions.push_back(spinbCutoffFactor);

	QLabel *lblMinPulseHeight = new QLabel(tr("Minimal pulse height:"));
	QDoubleSpinBox* spinbMinPulseHeight = new QDoubleSpinBox();	// standard-value: 3
	spinbMinPulseHeight->setValue(10);
	spinbMinPulseHeight->setToolTip("MinPulseHeight");
	allOptions.push_back(spinbMinPulseHeight);

	QLabel *lblMaxScaleFrequency = new QLabel(tr("Maximal scale frequency:"));
	QDoubleSpinBox* spinbMaxScaleFrequency = new QDoubleSpinBox(); // standard-value: 700
	spinbMaxScaleFrequency->setMaximum(1000);
	spinbMaxScaleFrequency->setValue(700);
	spinbMaxScaleFrequency->setToolTip("MaxPulseScaleFreq");
	allOptions.push_back(spinbMaxScaleFrequency);

	QLabel *lblDoGSeperatePeaks = new QLabel(tr("Minimum DoG speration distance (ms):"));
	QDoubleSpinBox* spinbDoGSeperatePeaks = new QDoubleSpinBox();	// standard-value: fs/1000
	spinbDoGSeperatePeaks->setMaximum(1000);
	spinbDoGSeperatePeaks->setValue(1);
	spinbDoGSeperatePeaks->setToolTip("MinDogSperation");
	allOptions.push_back(spinbDoGSeperatePeaks);

	QLabel *lblMorletSeperatePeaks = new QLabel(tr("Minimum Morlet seperation distance (ms):"));
	QDoubleSpinBox* spinbMorletSeperatePeaks = new QDoubleSpinBox(); // standard-value: fs/1000
	spinbMorletSeperatePeaks->setMaximum(1000);
	spinbMorletSeperatePeaks->setValue(1);
	spinbMorletSeperatePeaks->setToolTip("MinMorletSeperation");
	allOptions.push_back(spinbMorletSeperatePeaks);

	QLabel *lblPeakExpandToPulseFactor = new QLabel(tr("Peak expansion factor (ms):"));
	QDoubleSpinBox* spinbPeakExpandToPulseFactor = new QDoubleSpinBox(); // standard-value: fs/3000
	spinbPeakExpandToPulseFactor->setMaximum(1000);
	spinbPeakExpandToPulseFactor->setValue(0.333);
	spinbPeakExpandToPulseFactor->setToolTip("PeakExpansionFact");
	allOptions.push_back(spinbPeakExpandToPulseFactor);

	QLabel *lblPeakToPeakVoltageWindow = new QLabel(tr("Peak to Peak voltage window (ms):"));
	QDoubleSpinBox* spinbPeakToPeakVoltageWindow = new QDoubleSpinBox(); // standard-value: fs/50
	spinbPeakToPeakVoltageWindow->setMaximum(9000);
	spinbPeakToPeakVoltageWindow->setValue(20);
	spinbPeakToPeakVoltageWindow->setToolTip("PeakToPeakVoltage");
	allOptions.push_back(spinbPeakToPeakVoltageWindow);

	QLabel *lblMaxPulseDistance = new QLabel(tr("Maximal pulse distance (ms):"));
	QDoubleSpinBox* spinbMaxPulseDistance = new QDoubleSpinBox(); // standard-value: fs/2
	spinbMaxPulseDistance->setMaximum(9000);
	spinbMaxPulseDistance->setValue(80);
	spinbMaxPulseDistance->setToolTip("MaxPulseDistance");
	allOptions.push_back(spinbMaxPulseDistance);

	QLabel *lblMinPulseDistance = new QLabel(tr("Minimal pulse distance (ms):"));
	QDoubleSpinBox* spinbMinPulseDistance = new QDoubleSpinBox(); // standard-value: fs/100
	spinbMinPulseDistance->setMaximum(5000);
	spinbMinPulseDistance->setValue(15);
	spinbMinPulseDistance->setToolTip("MinPulseDistance");
	allOptions.push_back(spinbMinPulseDistance);

	QLabel *lblPulseTimeWindow = new QLabel(tr("Pulse time window (ms):"));
	QDoubleSpinBox* spinbPulseTimeWindow = new QDoubleSpinBox(); // standard-value: fs/100
	spinbPulseTimeWindow->setMaximum(5000);
	spinbPulseTimeWindow->setValue(5);
	spinbPulseTimeWindow->setToolTip("PulseTimeWindow");
	allOptions.push_back(spinbPulseTimeWindow);

	QCheckBox* chbExclude0Cycles = new QCheckBox("Exclude pulses with 0 cycles"); // standard-value: fs/100
	chbExclude0Cycles->setToolTip("Exclude0Cycles");
	allOptions.push_back(chbExclude0Cycles);

	QVBoxLayout* vertPulseOLayout = new QVBoxLayout;
	vertPulseOLayout->setAlignment(Qt::AlignTop);
	vertPulseOLayout->addWidget(lblCutoffFactor);
	vertPulseOLayout->addWidget(spinbCutoffFactor);
	vertPulseOLayout->addWidget(lblMinPulseHeight);
	vertPulseOLayout->addWidget(spinbMinPulseHeight);
	vertPulseOLayout->addWidget(lblMaxScaleFrequency);
	vertPulseOLayout->addWidget(spinbMaxScaleFrequency);
	vertPulseOLayout->addWidget(lblDoGSeperatePeaks);
	vertPulseOLayout->addWidget(spinbDoGSeperatePeaks);
	vertPulseOLayout->addWidget(lblMorletSeperatePeaks);
	vertPulseOLayout->addWidget(spinbMorletSeperatePeaks);
	vertPulseOLayout->addWidget(lblPeakExpandToPulseFactor);
	vertPulseOLayout->addWidget(spinbPeakExpandToPulseFactor);
	vertPulseOLayout->addWidget(lblPeakToPeakVoltageWindow);
	vertPulseOLayout->addWidget(spinbPeakToPeakVoltageWindow);
	vertPulseOLayout->addWidget(lblMaxPulseDistance);
	vertPulseOLayout->addWidget(spinbMaxPulseDistance);
	vertPulseOLayout->addWidget(lblMinPulseDistance);
	vertPulseOLayout->addWidget(spinbMinPulseDistance);
	vertPulseOLayout->addWidget(lblPulseTimeWindow);
	vertPulseOLayout->addWidget(spinbPulseTimeWindow);
	vertPulseOLayout->addWidget(chbExclude0Cycles);

	QGroupBox* grpPulseOptions = new QGroupBox("Pulse Options");
	grpPulseOptions->setLayout(vertPulseOLayout);

	QHBoxLayout* optionLayout = new QHBoxLayout;
	optionLayout->addWidget(grpSineOptions);
	optionLayout->addWidget(grpTrainOptions);
	optionLayout->addWidget(grpPulseOptions);

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
	layout->addLayout(buttonLayout);
	
	setLayout(layout);

	connect(btnLoadOptions, SIGNAL(clicked(bool)), this, SLOT(loadOptions()));
	connect(btnSaveOptions, SIGNAL(clicked(bool)), this, SLOT(saveOptions()));
}

QSize PulseDetectionPage::minimumSizeHint() const
{
	return QSize(160, 60);
}

QSize PulseDetectionPage::sizeHint() const
{
	return QSize(100, 100);
}

std::map<QString, float> PulseDetectionPage::getOptions() const
{
	std::map<QString, float> options;
	for(std::vector<QWidget*>::const_iterator iter = allOptions.begin(); iter != allOptions.end(); ++iter){
		if(QDoubleSpinBox* spinb = dynamic_cast<QDoubleSpinBox*>((*iter))){
			options[spinb->toolTip()] = spinb->value();
		}else if(QCheckBox* cheB = dynamic_cast<QCheckBox*>((*iter))){
			options[cheB->toolTip()] = cheB->isChecked();
		}
	}
	return options;
}

void PulseDetectionPage::setOptions(std::map<QString, float> options) const
{
	for(std::vector<QWidget*>::const_iterator iter = allOptions.begin(); iter != allOptions.end(); ++iter){
		if(QDoubleSpinBox* spinB = dynamic_cast<QDoubleSpinBox*>((*iter))){
			spinB->setValue(options[spinB->toolTip()]);
		}else if(QCheckBox* cheB = dynamic_cast<QCheckBox*>((*iter))){
			cheB->setChecked(options[cheB->toolTip()]);
		}
	}
}

void PulseDetectionPage::loadOptions()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Pulse Detection Settings"), "../", tr("Option Files (*.txt)"));
	
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

		for(std::vector<QWidget*>::const_iterator iter = allOptions.begin(); iter != allOptions.end(); ++iter){
			if(QDoubleSpinBox* spinB = dynamic_cast<QDoubleSpinBox*>((*iter))){
				spinB->setValue(options[spinB->toolTip()]);
			}else if(QCheckBox* cheB = dynamic_cast<QCheckBox*>((*iter))){
				cheB->setChecked(options[cheB->toolTip()]);
			}
		}
	}
}

void PulseDetectionPage::saveOptions()
{
	std::map<QString, float> options = getOptions();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Pulse Detection Settings"), "../", tr("Option Files (*.txt)"));
	
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
