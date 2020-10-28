#pragma once

#include <QtWidgets>

#include "../Dimension.h"

//TODO: add shifting as parameter
//TODO: add default value as parameter
class DimensionField : public QWidget
{
  Q_OBJECT
  private:
    QLineEdit * label, *value, *minValue, *maxValue;
    QPushButton * deleteButton;

  protected slots:
    void deleteButtonPressed()
    {
      emit deleteSelf(this);
    }

  public:
    DimensionField()
    {
      label = new QLineEdit();
      value = new QLineEdit();
      minValue = new QLineEdit();
      maxValue = new QLineEdit();
      deleteButton = new QPushButton( "X", this);
      deleteButton->setCursor( QCursor( Qt::PointingHandCursor ) );
      connect( deleteButton, SIGNAL( clicked() ),
            this, SLOT( deleteButtonPressed() ) );

      QVBoxLayout * leftLayout = new QVBoxLayout;
      QVBoxLayout * rightLayout = new QVBoxLayout;
      QHBoxLayout * layout = new QHBoxLayout;
      this->setLayout(layout);
      leftLayout->addWidget(label);
      leftLayout->addWidget(value);
      rightLayout->addWidget(minValue);
      rightLayout->addWidget(maxValue);
      QWidget * leftContainer = new QWidget;
      QWidget * rightContainer = new QWidget;
      leftContainer->setLayout(leftLayout);
      rightContainer->setLayout(rightLayout);
      layout->addWidget(leftContainer);
      layout->addWidget(rightContainer);
      layout->addWidget(deleteButton);
      layout->setMargin(0);
      layout->setSpacing(0);
    }

    std::pair<std::string, double> getCoordinate() const
    {
      return std::make_pair(label->text().toStdString(), value->text().toDouble());
    }

    Diginstrument::Dimension getDimension() const
    {
      return Diginstrument::Dimension(label->text().toStdString(), minValue->text().toDouble(), maxValue->text().toDouble());
    }

  signals:
    void deleteSelf(DimensionField * self);
};