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


namespace gui
{


class WaveForm : public QWidget {
    	Q_OBJECT
    protected:
        // virtual void enterEvent( QEvent * _e );
        // virtual void leaveEvent( QEvent * _e );
        // virtual void mousePressEvent( QMouseEvent * _me );
        // virtual void mouseReleaseEvent( QMouseEvent * _me );
        // virtual void mouseMoveEvent( QMouseEvent * _me );
        // virtual void wheelEvent( QWheelEvent * _we );
        virtual void paintEvent( QPaintEvent * _pe );


    private:
        
        QPixmap m_graph;

        SampleBuffer currentSample;
        int width;
        int height;

        void drawWaveForm();

    public:
        WaveForm(int _w, int _h, std::vector<int> & _slicePoints, QWidget * _parent);
        void updateFile(QString file);

    private:
        std::vector<int> & slicePoints;

};
}}


#endif