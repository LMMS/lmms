#include "WaveForm.h"
#include "SlicerT.h"

#include <stdio.h>

namespace lmms
{


namespace gui
{
    WaveForm::WaveForm(int _w, int _h, SlicerT  * _instrument, QWidget * _parent) :
        QWidget(_parent),
        seeker(QPixmap(_w, _h*seekerRatio)),
        sliceEditor(QPixmap(_w, _h*(1 - seekerRatio) - margin)),
        currentSample(),
        slicePoints(_instrument->slicePoints)
        {
            width = _w;
            height = _h;
            slicerTParent = _instrument;
            setFixedSize(width, height);
            setMouseTracking( true );
            setAcceptDrops( true );

            sliceEditor.fill(waveformBgColor);
            seeker.fill(waveformBgColor);

            connect(slicerTParent, 
                    SIGNAL(isPlaying(float, float, float)), 
                    this, 
                    SLOT(isPlaying(float, float, float)));
        }

    void WaveForm::drawEditor() {
        sliceEditor.fill(waveformBgColor);
        QPainter brush(&sliceEditor);
        brush.setPen(waveformColor);

        float startFrame = seekerStart * currentSample.frames();
        float endFrame = seekerEnd * currentSample.frames();

        currentSample.visualize(
		brush,
		QRect( 0, 0, sliceEditor.width(), sliceEditor.height() ),
		startFrame, endFrame);


        for (int i = 0;i<slicePoints.size();i++) {
            int sliceIndex = slicePoints[i];
            brush.setPen(QPen(sliceColor, 2));

            if (sliceIndex >= startFrame && sliceIndex <= endFrame) {
                float xPos = (float)(sliceIndex - startFrame) / (float)(endFrame - startFrame) * (float)width;
                if (i == sliceSelected) {
                    brush.setPen(QPen(selectedSliceColor, 2));
                }

                brush.drawLine(xPos, 0, xPos, height);
            }
        }
    }

    void WaveForm::drawSeeker() {
        seeker.fill(waveformBgColor);
        QPainter brush(&seeker);
        brush.setPen(waveformColor);

        currentSample.visualize(
		brush,
		QRect( 0, 0, seeker.width(), seeker.height() ),
		0, currentSample.frames());

        // draw slice points
        brush.setPen(sliceColor);
        for (int i = 0;i<slicePoints.size();i++) {
            float xPos = (float)slicePoints[i] / (float)currentSample.frames() * (float)width;
            brush.drawLine(xPos, 0, xPos, height);
        }

        // draw current playBack
        brush.setPen(playColor);
        // printf("noteplay index: %i\n", noteCurrent);
        brush.drawLine(noteCurrent*width, 0, noteCurrent*width, height);
        brush.fillRect(noteStart*width, 0, (noteEnd-noteStart)*width, height, playHighlighColor);

        // draw seeker points
        brush.setPen(QPen(seekerColor, 3));
        brush.drawLine(seekerStart*width, 0, seekerStart*width, height);
        brush.drawLine(seekerEnd*width, 0, seekerEnd*width, height);

        // shadow on not selected area
        brush.fillRect(0, 0, seekerStart*width, height, seekerShadowColor);
        brush.fillRect(seekerEnd*width, 0, width, height, seekerShadowColor);
    }

    void WaveForm::updateUI() {
        drawSeeker();
        drawEditor();
        update();
    }

    void WaveForm::updateFile(QString file) {
        currentSample.setAudioFile(file);
        updateUI();
    }

    void WaveForm::isPlaying(float current, float start, float end) {
        noteCurrent = current;
        noteStart = start;
        noteEnd = end;
        updateUI();
    }

    void WaveForm::mousePressEvent( QMouseEvent * _me ) {
        float normalizedClick = (float)_me->x() / width;
        
        if (_me->button() == Qt::MouseButton::MiddleButton) {
            seekerStart = 0;
            seekerEnd = 1;
            return;
        }

        if (_me->y() < height*seekerRatio) {
            if (abs(normalizedClick - seekerStart) < 0.03) {
                currentlyDragging = draggingTypes::seekerStart;

            } else if (abs(normalizedClick - seekerEnd) < 0.03) {
                currentlyDragging = draggingTypes::seekerEnd;

            } else if (normalizedClick > seekerStart && normalizedClick < seekerEnd) {
                currentlyDragging = draggingTypes::seekerMiddle;
                seekerMiddle = normalizedClick;
            }

        } else {
            sliceSelected = -1;
            float startFrame = seekerStart * currentSample.frames();
            float endFrame = seekerEnd * currentSample.frames();
            
            for (int i = 0;i<slicePoints.size();i++) {
                int sliceIndex = slicePoints[i];
                float xPos = (float)(sliceIndex - startFrame) / (float)(endFrame - startFrame);

                if (abs(xPos - normalizedClick) < 0.03) {
                    currentlyDragging = draggingTypes::slicePoint;
                    sliceSelected = i;

                }
            }
        } 

        if (_me->button() == Qt::MouseButton::LeftButton) {
            isDragging = true;

        } else if (_me->button() == Qt::MouseButton::RightButton) {
            if (sliceSelected != -1 && slicePoints.size() > 2) {
                slicePoints.erase(slicePoints.begin() + sliceSelected);
                sliceSelected = -1;
            }
        }  

    }

    void WaveForm::mouseReleaseEvent( QMouseEvent * _me ) {
        isDragging = false;
        currentlyDragging = draggingTypes::nothing;
        updateUI();
    }

    void WaveForm::mouseMoveEvent( QMouseEvent * _me ) {
        float normalizedClick = (float)_me->x() / width;
        
        // handle dragging events
        if (isDragging) { 
            // printf("drag type:%i , seekerStart: %f , seekerEnd: %f \n", currentlyDragging, seekerStart, seekerEnd);
            if (currentlyDragging == draggingTypes::seekerStart) {
                seekerStart = std::clamp(normalizedClick, 0.0f, seekerEnd - 0.13f);

            } else if (currentlyDragging == draggingTypes::seekerEnd) {
                seekerEnd = std::clamp(normalizedClick, seekerStart + 0.13f, 1.0f);;

            } else if (currentlyDragging == draggingTypes::seekerMiddle) {
                float distStart = seekerStart - seekerMiddle;
                float distEnd = seekerEnd - seekerMiddle;

                seekerMiddle = normalizedClick;

                if (seekerMiddle + distStart > 0 && seekerMiddle + distEnd < 1) {
                    seekerStart = seekerMiddle + distStart;
                    seekerEnd = seekerMiddle + distEnd;
                }

            } else if (currentlyDragging == draggingTypes::slicePoint) {
                float startFrame = seekerStart * currentSample.frames();
                float endFrame = seekerEnd * currentSample.frames();

                slicePoints[sliceSelected] = startFrame + normalizedClick * (endFrame - startFrame);

                slicePoints[sliceSelected] = std::clamp(slicePoints[sliceSelected], 0, currentSample.frames());

                std::sort(slicePoints.begin(), slicePoints.end());

            }
            updateUI();
        } else {

        }

        

    }

    void WaveForm::mouseDoubleClickEvent(QMouseEvent * _me) {
        float normalizedClick = (float)_me->x() / width;
        float startFrame = seekerStart * currentSample.frames();
        float endFrame = seekerEnd * currentSample.frames();

        float slicePosition = startFrame + normalizedClick * (endFrame - startFrame);

        for (int i = 0;i<slicePoints.size();i++) {
            if (slicePoints[i] < slicePosition) {
                slicePoints.insert(slicePoints.begin() + i, slicePosition);
                break;
            }
        }

        std::sort(slicePoints.begin(), slicePoints.end());
        
    }

    // pianoView.cpp always claims focus while in plugin screen, so this doesnt work
    void WaveForm::keyPressEvent(QKeyEvent * ke) {
        int key = ke->key();
        printf("key: %i\n", ke->key());
        if ((key == 16777219 || key == 16777223) && // delete and backspace 
            (sliceSelected != -1 && slicePoints.size() > 2)) {

            slicePoints.erase(slicePoints.begin() + sliceSelected);
        }
        updateUI();

    }

    void WaveForm::paintEvent( QPaintEvent * _pe) {
        QPainter p( this );
        p.drawPixmap(0, height*0.3f + margin, sliceEditor);
        p.drawPixmap(0, 0, seeker);
    }



}
}