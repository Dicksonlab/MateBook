#ifndef FoldableGroupBox_hpp
#define FoldableGroupBox_hpp

#include <QGroupBox>

/**
  * @class  FoldableGroupBox
  * @brief  class deriving from QGroupBox, making it foldable (checkbox)
  */
class FoldableGroupBox : public QGroupBox {
	Q_OBJECT

public:
	FoldableGroupBox(const QString & title, QWidget * parent = 0);

public slots:
	void setChecked(bool checked);
	
};

#endif
