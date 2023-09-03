#include <stdio.h>

#include "WaveForm.h"

namespace lmms
{


namespace gui
{
    WaveForm::WaveForm(int _w, int _h, std::vector<int> & _slicePoints, QWidget * _parent) :
        QWidget(_parent),
        m_graph( QPixmap(_w, _h)),
        currentSample(),
        slicePoints(_slicePoints)
        {
            width = _w;
            height = _h;
            setFixedSize( width, height );

            m_graph.fill(QColor(11, 11, 11));
        }

    void WaveForm::drawWaveForm() {
        m_graph.fill(QColor(11, 11, 11));
        QPainter brush(&m_graph);
        brush.setPen(QColor(255, 0, 0));
        currentSample.visualize(
		brush,
		QRect( 0, 0, m_graph.width(), m_graph.height() ),
		0, currentSample.frames());
        brush.setPen(QColor(0, 255, 0));

        for (int i = 0;i<slicePoints.size();i++) {
            // printf("%i\n", slicePoints[i]);
            float xPos = (float)slicePoints[i] / (float)currentSample.frames() * (float)width;
            // printf("%i / %i * %i = %f\n", slicePoints[i] , currentSample.frames() , width, xPos);
            brush.drawLine(xPos, 0, xPos, height);
        }

    }


    void WaveForm::paintEvent( QPaintEvent * _pe) {
        QPainter p( this );
        drawWaveForm();
        p.drawPixmap(0, 0, m_graph);
    }

    void WaveForm::updateFile(QString file) {
        currentSample.setAudioFile(file);
        drawWaveForm();
        update();
    }

}
}