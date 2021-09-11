#ifndef METRONOMESETTINGSMENU_H
#define METRONOMESETTINGSMENU_H

#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>
#include <QtCore/QString>
#include <QMouseEvent>
#include <QFont>

#include "ToolButton.h"
#include "AutomatableSlider.h"


struct OptionElements{
        std::vector<std::pair<QString, QPushButton*> > rhythm;
        AutomatableSlider* volume;
};

/**
 * @brief Class for the Metronome's GUI-Submenu
 * 
 * The widget that is created here is opened via RMB on the metronome, it is intended to gather all
 * possible metronome features (e.g. volume control, speed, sound) 
 */
class MetronomeSettingsMenu : public QMenu //public QWidget
{
    Q_OBJECT

public:
    MetronomeSettingsMenu(short optionsPerLine, QWidget *parent = nullptr);
    ToolButton* getMenuButton(QWidget * _parent=nullptr);
    void propagateInitialSettings();

private:
    QVBoxLayout* m_generalMenuLayout;
    /**
    * @brief Make a standardized subgroup for the GUI element
    * @param title group name
    * @param elements the individual options for this group
    * @return layout of subgroup
    */
    QVBoxLayout* makeGeneralMenuSection(QString title, std::vector<QString> elements);

    /**
    * @brief Make a standardized subgroup for the volume GUI element
    * @param title group name
    * @return layout of subgroup
    */
    QVBoxLayout* makeVolumeControlSection(QString title);
    /**
    * @brief Helperfun to make a standardized title appearance
    * @param title group name
    */
    QLabel* makeGroupTitle(QString title);

    // Holds the gui elements
    OptionElements m_optionElements;
    // Limit of options until a new line should be opened
    short m_maxMenuRowElements;

    QFont m_groupNameFont;
    
    /**
    * @brief closes the widget if its area was left by the cursor, for cases when the user 
    * didn't change the settings
    */
    void leaveEvent(QEvent *event);

signals:
    // whenever the user clicked an option this pair of <group name, option name> is emitted
    void optionChanged(std::pair<QString, QString>);
    // emit for every user chang of the volume control's slider
    void volumeChanged(float);

private slots:
    void handleOptionUpdates();
    void handleVolumeUpdates(int);
};



#endif // METRONOMESETTINGSMENU_H
