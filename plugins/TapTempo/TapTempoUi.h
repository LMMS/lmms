/********************************************************************************
** Form generated from reading UI file 'taptempo.ui'
**
** Created by: Qt User Interface Compiler version 5.15.9
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAPTEMPO_H
#define UI_TAPTEMPO_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TapTempo
{
public:
    QVBoxLayout *windowLayout;
    QVBoxLayout *mainLayout;
    QPushButton *tapButton;
    QHBoxLayout *sidebarLayout;
    QLabel *msLabel;
    QLabel *hzLabel;
    QVBoxLayout *verticalLayout;
    QCheckBox *precisionCheckBox;
    QCheckBox *muteCheckBox;

    void setupUi(QWidget *TapTempo)
    {
        if (TapTempo->objectName().isEmpty())
            TapTempo->setObjectName(QString::fromUtf8("TapTempo"));
        TapTempo->setWindowModality(Qt::NonModal);
        TapTempo->setEnabled(true);
        TapTempo->resize(250, 288);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TapTempo->sizePolicy().hasHeightForWidth());
        TapTempo->setSizePolicy(sizePolicy);
        TapTempo->setMinimumSize(QSize(250, 288));
        TapTempo->setMaximumSize(QSize(250, 288));
        QIcon icon;
        icon.addFile(QString::fromUtf8("logo.png"), QSize(), QIcon::Normal, QIcon::Off);
        TapTempo->setWindowIcon(icon);
        TapTempo->setAutoFillBackground(true);
        TapTempo->setStyleSheet(QString::fromUtf8("QLabel {\n"
" color: rgb(255, 255, 255);\n"
" background-color: transparent;\n"
"}\n"
""));
        windowLayout = new QVBoxLayout(TapTempo);
        windowLayout->setSpacing(0);
        windowLayout->setObjectName(QString::fromUtf8("windowLayout"));
        windowLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        windowLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout = new QVBoxLayout();
        mainLayout->setSpacing(5);
        mainLayout->setObjectName(QString::fromUtf8("mainLayout"));
        mainLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        mainLayout->setContentsMargins(2, 2, 2, 2);
        tapButton = new QPushButton(TapTempo);
        tapButton->setObjectName(QString::fromUtf8("tapButton"));
        tapButton->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(tapButton->sizePolicy().hasHeightForWidth());
        tapButton->setSizePolicy(sizePolicy1);
        tapButton->setMinimumSize(QSize(200, 200));
        tapButton->setMaximumSize(QSize(16777215, 16777215));
        tapButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"	font-size: 24px;\n"
"	color: rgb(255, 255, 255);\n"
"    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(62, 62, 62, 255), stop:1 rgba(32, 32, 32, 255));\n"
"    border: 2px solid rgb(0, 0, 0);\n"
"    border-radius: 15px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(52, 52, 52, 255), stop:1 rgba(22, 22, 22, 255));\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(32, 32, 32, 255), stop:1 rgba(12, 12, 12, 255));\n"
"}\n"
"\n"
"QPushButton:disabled {\n"
"    background-color: rgba(62, 62, 62, 255);\n"
"    border: 2px solid rgba(100, 100, 100, 255);\n"
"}\n"
"\n"
"QPushButton:disabled:hover {\n"
"    background-color: rgba(52, 52, 52, 255);\n"
"}"));

        mainLayout->addWidget(tapButton, 0, Qt::AlignHCenter|Qt::AlignVCenter);

        sidebarLayout = new QHBoxLayout();
        sidebarLayout->setSpacing(5);
        sidebarLayout->setObjectName(QString::fromUtf8("sidebarLayout"));
        sidebarLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        msLabel = new QLabel(TapTempo);
        msLabel->setObjectName(QString::fromUtf8("msLabel"));
        msLabel->setStyleSheet(QString::fromUtf8(""));

        sidebarLayout->addWidget(msLabel, 0, Qt::AlignHCenter);

        hzLabel = new QLabel(TapTempo);
        hzLabel->setObjectName(QString::fromUtf8("hzLabel"));
        hzLabel->setStyleSheet(QString::fromUtf8(""));

        sidebarLayout->addWidget(hzLabel, 0, Qt::AlignHCenter);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(5);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        precisionCheckBox = new QCheckBox(TapTempo);
        precisionCheckBox->setObjectName(QString::fromUtf8("precisionCheckBox"));
        precisionCheckBox->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"\n"
"QCheckBox::indicator::checked {\n"
"	color: rgb(0, 0, 0)\n"
"}"));
        precisionCheckBox->setTristate(false);

        verticalLayout->addWidget(precisionCheckBox);

        muteCheckBox = new QCheckBox(TapTempo);
        muteCheckBox->setObjectName(QString::fromUtf8("muteCheckBox"));
        muteCheckBox->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);"));

        verticalLayout->addWidget(muteCheckBox);


        sidebarLayout->addLayout(verticalLayout);


        mainLayout->addLayout(sidebarLayout);


        windowLayout->addLayout(mainLayout);


        retranslateUi(TapTempo);

        QMetaObject::connectSlotsByName(TapTempo);
    } // setupUi

    void retranslateUi(QWidget *TapTempo)
    {
        TapTempo->setWindowTitle(QCoreApplication::translate("TapTempo", "Tap Tempo", nullptr));
        tapButton->setText(QCoreApplication::translate("TapTempo", "TAP", nullptr));
#if QT_CONFIG(tooltip)
        msLabel->setToolTip(QCoreApplication::translate("TapTempo", "<html><head/><body><p>BPM in milliseconds</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        msLabel->setText(QCoreApplication::translate("TapTempo", "0.0 ms", nullptr));
#if QT_CONFIG(tooltip)
        hzLabel->setToolTip(QCoreApplication::translate("TapTempo", "<html><head/><body><p>Frequency of BPM</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        hzLabel->setText(QCoreApplication::translate("TapTempo", "0.0 hz", nullptr));
#if QT_CONFIG(tooltip)
        precisionCheckBox->setToolTip(QCoreApplication::translate("TapTempo", "<html><head/><body><p>Display in high precision</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        precisionCheckBox->setText(QCoreApplication::translate("TapTempo", "Precision", nullptr));
#if QT_CONFIG(tooltip)
        muteCheckBox->setToolTip(QCoreApplication::translate("TapTempo", "<html><head/><body><p>Mute metronome</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        muteCheckBox->setText(QCoreApplication::translate("TapTempo", "Mute", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TapTempo: public Ui_TapTempo {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAPTEMPO_H
