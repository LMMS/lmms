#include <MetronomeSettingsMenu.h>
#include <QtWidgets/QHBoxLayout>
#include <QIcon>

#include "embed.h"

MetronomeSettingsMenu::MetronomeSettingsMenu(short optionsPerLine, QWidget *parent)
    :QWidget(parent), m_maxMenuRowElements(optionsPerLine)
{
    m_maxMenuRowElements = 3;
    m_groupNameFont.setWeight(QFont::ExtraBold);
    setWindowFlags(Qt::CustomizeWindowHint); // turns off all windows flags, hence no icons nor frame

    m_generalMenuLayout = new QVBoxLayout();
    m_generalMenuLayout->addLayout(makeVolumeControlSection("Volume"));
    m_generalMenuLayout->addLayout(makeGeneralMenuSection("Rythm", std::vector<QString>{"1/1", "1/2", "1/4", "1/8", "1/16"}));
    this->setLayout(m_generalMenuLayout);
}

void MetronomeSettingsMenu::propagateInitialSettings()
{
    int initialVolume = 80; // % of slider range
    QString initialRythm = "1/4";


    for (std::vector<std::pair<QString,QPushButton*> >::iterator rythm_option = m_optionElements.rythm.begin(); 
         rythm_option != m_optionElements.rythm.end(); rythm_option++)
    {
        if (initialRythm.compare((*rythm_option).first) == 0)        
            emit (*rythm_option).second->released();
    }

    m_optionElements.volume->setValue(initialVolume);
    emit m_optionElements.volume->sliderMoved(initialVolume);
}

QLabel* MetronomeSettingsMenu::makeGroupTitle(QString title)
{
    QLabel* group_name = new QLabel(title);
    group_name->setFont(m_groupNameFont);
    return group_name;
}

QVBoxLayout* MetronomeSettingsMenu::makeVolumeControlSection(QString title)
{
    QVBoxLayout* sectionLayout = new QVBoxLayout();
    sectionLayout->addWidget(makeGroupTitle(title));
    QSlider * volumeControl = new QSlider(Qt::Horizontal);    
    connect(volumeControl, &QSlider::sliderMoved, this, &MetronomeSettingsMenu::handleVolumeUpdates);
    sectionLayout->addWidget(volumeControl);
    m_optionElements.volume = volumeControl;
    return sectionLayout;
}

QVBoxLayout* MetronomeSettingsMenu::makeGeneralMenuSection(QString title, std::vector<QString> elements)
{
    QVBoxLayout* sectionLayout = new QVBoxLayout();
    QHBoxLayout* rowLayout = new QHBoxLayout();
    sectionLayout->addWidget(makeGroupTitle(title));
    for (std::vector<QString>::iterator element = elements.begin(); element != elements.end(); element ++)
    {
        QPushButton * option = new QPushButton(*element);
        option->setFlat(true);
        option->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        m_optionElements.rythm.push_back(std::make_pair(*element, option));
        connect(option, SIGNAL(released()), this, SLOT(handleOptionUpdates()));
        rowLayout->addWidget(option);
        
        // add a new row if necessary
        if (m_optionElements.rythm.size() % m_maxMenuRowElements == 0)
        {
            sectionLayout->addLayout(rowLayout);
            rowLayout = new QHBoxLayout();
        }
    }
    sectionLayout->addLayout(rowLayout);
    return sectionLayout;
}

void MetronomeSettingsMenu::handleVolumeUpdates(int volume)
{
    emit volumeChanged(volume*0.01);
}

void MetronomeSettingsMenu::handleOptionUpdates()
{
    QObject* option = sender();
    QString titleStr, option_str;
    for (std::vector<std::pair<QString,QPushButton*> >::iterator rythm_option = m_optionElements.rythm.begin();
         rythm_option != m_optionElements.rythm.end(); rythm_option ++)
    {
        if ((*rythm_option).second == option)
        {
            option_str = (*rythm_option).first;
            (*rythm_option).second->setIcon(embed::getIconPixmap( "autoscroll_on" ));
        }
        else // remove possible previouse user coice icon
        {
            (*rythm_option).second->setIcon(QIcon());
        }

    }

    hide();
    emit optionChanged(std::make_pair("Rythm", option_str));
}

void MetronomeSettingsMenu::leaveEvent(QEvent *event)
{
    hide();
    QWidget::leaveEvent(event);
}
