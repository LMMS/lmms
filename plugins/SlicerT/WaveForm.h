#include <QApplication>
#include <QFontMetrics>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>

#include "SampleBuffer.h"

#ifndef WAVEFORM_H
#define WAVEFORM_H

namespace lmms
{

class SlicerT;

namespace gui
{


class WaveForm : public QWidget {
    	Q_OBJECT
    protected:
        virtual void mousePressEvent(QMouseEvent * me);
        virtual void mouseReleaseEvent(QMouseEvent * me);
        virtual void mouseMoveEvent(QMouseEvent * me);
        virtual void mouseDoubleClickEvent(QMouseEvent * me);

        virtual void paintEvent(QPaintEvent * pe);

    private:
        int m_width;
        int m_height;
        float m_m_seekerRatio = 0.3f;
        int m_margin = 5;
        QColor m_waveformBgColor = QColor(11, 11, 11);
        QColor m_waveformColor = QColor(124, 49, 214);
        QColor m_playColor = QColor(255, 255, 255, 200);
        QColor m_playHighlighColor = QColor(255, 255, 255, 70);
        QColor m_sliceColor = QColor(49, 214, 124);
        QColor m_selectedSliceColor = QColor(172, 236, 190);
        QColor m_seekerColor = QColor(214, 124, 49);
        QColor m_seekerShadowColor = QColor(0, 0, 0, 175);

        enum class m_draggingTypes {
            nothing,
            m_seekerStart,
            m_seekerEnd,
            m_seekerMiddle,
            slicePoint,
        };
        m_draggingTypes m_currentlyDragging;
        bool m_isDragging = false;

        float m_seekerStart = 0;
        float m_seekerEnd = 1;
        float m_seekerMiddle = 0.5f;
        int m_sliceSelected = 0;

        float m_noteCurrent;
        float m_noteStart;
        float m_noteEnd;

        QPixmap m_sliceEditor;
        QPixmap m_seeker;

        SampleBuffer m_currentSample;

        void drawEditor();
        void drawSeeker();
        void updateUI();

    public slots:
        void updateData();
        void isPlaying(float current, float start, float end);

    public:
        WaveForm(int w, int h, SlicerT * instrument, QWidget * parent);

    private:
        SlicerT * m_slicerTParent;
        std::vector<int> & m_slicePoints;

};
}}


#endif