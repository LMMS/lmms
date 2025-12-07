#include "Visualiser.h"

#include <lmms/Plugin.h>

#include <lmms/SampleFrame.h>

#include <QWidget>

#include <QPainter>

#include <QTimer>

#include <QVBoxLayout>

#include <QHBoxLayout>

#include <QLabel>

#include <QDomDocument>

#include <QStandardPaths>

#include <algorithm>

#include <cmath>

#include <complex>

#include <vector>

#include <cstdlib>



using namespace lmms;



extern "C" LMMS_EXPORT Plugin::Descriptor Visualiser_plugin_descriptor;



#define BUFFER_SIZE 512

static float g_bufferL[BUFFER_SIZE];

static float g_bufferR[BUFFER_SIZE];

#define SAMPLE_RATE 44100.0f 



std::vector<std::complex<float>> computeComplexFFT(float* data) {

    int bins = 128; 

    std::vector<std::complex<float>> output(bins);

    for (int k = 0; k < bins; ++k) {

        std::complex<float> sum(0, 0);

        for (int t = 0; t < BUFFER_SIZE; t += 2) { 

            float angle = 2.0f * M_PI * t * k / BUFFER_SIZE;

            sum += std::complex<float>(data[t] * cos(angle), data[t] * -sin(angle));

        }

        output[k] = sum / (float)BUFFER_SIZE;

    }

    return output;

}



QString freqToNote(float freq) {

    if (freq < 20.0f) return "--"; 

    int midi = std::round(69 + 12 * std::log2(freq / 440.0));

    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    int noteIndex = midi % 12;

    int octave = (midi / 12) - 1;

    if (noteIndex < 0) noteIndex = 0; 

    return QString("%1%2").arg(notes[noteIndex]).arg(octave);

}



// ================================================================

// SCREEN WIDGET

// ================================================================

class VisualiserScreen : public QWidget{

public:

    explicit VisualiserScreen(QWidget *parent = nullptr)

        : QWidget(parent)

        , m_mode(0)

        , m_showText(true)

        , m_wiggleText(false)

        , m_hue(0)

        , m_randomModeIndex(0)

        , m_cycleCounter(0)

        , m_isRecording(false)

        , m_recResolution(0)

        , m_reactionBand(0)

        , m_recFrameCount(0)

    {

        setMinimumSize(300, 250); 

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);



        for(int i=0; i<100; ++i) {

            m_stars.push_back({

                (float)(rand()%2000 - 1000), 

                (float)(rand()%2000 - 1000), 

                (float)(rand()%1000 + 1)

            });

        }

        memset(m_fire, 0, sizeof(m_fire));



        QTimer *t = new QTimer(this);

        connect(t, &QTimer::timeout, this, [this]() { 

            updateLogic();

            if(m_isRecording) saveFrame(); // SAVE TO DISK

            update(); 

        });

        t->start(30);

    }



    // --- SMART REACTION ENGINE ---

    float getReactionLevel() {

        if (m_reactionBand == 0) return std::abs(g_bufferL[0]); // All (Simple Kick Proxy)

        

        // If specific band, sum FFT bins

        auto bins = computeComplexFFT(g_bufferL);

        float sum = 0;

        int start = 0; int end = 128;



        if (m_reactionBand == 1) { start = 0; end = 6; }    // Low (Bass)

        else if (m_reactionBand == 2) { start = 6; end = 30; } // Mid

        else if (m_reactionBand == 3) { start = 30; end = 100; } // High (Treble)



        for(int i=start; i<end && i<(int)bins.size(); ++i) {

            sum += std::abs(bins[i]);

        }

        return sum * 3.0f; // Boost for visibility

    }



    void updateLogic() {

        m_hue = (m_hue + 2) % 360;

        if(m_mode == 31) {

            m_cycleCounter++;

            if(m_cycleCounter > 100) { m_cycleCounter = 0; m_randomModeIndex = rand() % 31; }

        }

        

        // Particles (Reactive)

        if(m_mode == 15 || (m_mode == 31 && m_randomModeIndex == 15)) {

            float kick = getReactionLevel();

            if(kick > 0.2f) { // Threshold

                for(int k=0; k<3; k++) {

                    Particle p;

                    p.x = rand() % width();

                    p.y = height()/2 + (rand()%100 - 50);

                    p.vx = (rand()%10 - 5) * 0.1f; p.vy = (rand()%5 + 1); p.life = 50;

                    p.color = QColor::fromHsv(m_hue, 200, 255);

                    m_particles.push_back(p);

                }

            }

            for(auto &p : m_particles) { p.x += p.vx; p.y += p.vy; p.life--; }

            m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle &p){ return p.life <= 0; }), m_particles.end());

        }

    }



    void setMode(int m) { m_mode = m; update(); }

    void setShowText(bool show) { m_showText = show; update(); }

    void setWiggleText(bool wiggle) { m_wiggleText = wiggle; update(); }

    void setRecState(bool rec) { m_isRecording = rec; if(rec) m_recFrameCount = 0; }

    void setRecResolution(int r) { m_recResolution = r; }

    void setReactionBand(int b) { m_reactionBand = b; }

    void setText(const QString &t) { m_text = t; update(); }



    void renderScene(QPainter &p, int w, int h) {

        p.fillRect(0, 0, w, h, Qt::black);

        int activeMode = (m_mode == 31) ? m_randomModeIndex : m_mode;



        switch (activeMode) {

        case 0: drawWaveform(p, w, h); break;

        case 1: drawLabSpectrum(p, w, h); break;

        case 2: drawPhaseScope(p, w, h); break;

        case 3: drawComplexPlane(p, w, h); break;

        case 4: drawTopFrequencies(p, w, h); break;

        case 5: drawColorOrgan(p, w, h); break;

        case 6: drawHypnoSpiral(p, w, h); break;

        case 7: drawFire(p, w, h); break;

        case 8: drawStarfield(p, w, h); break;

        case 9: drawData(p, w, h); break;

        case 10: drawDaftGrid(p, w, h); break;

        case 11: drawBinaryRain(p, w, h); break;

        case 12: drawGlitch(p, w, h); break;

        case 13: drawRetroSun(p, w, h); break;

        case 14: drawTunnel(p, w, h); break;

        case 15: drawParticles(p, w, h); break;

        case 16: drawWaveform2(p, w, h); break;

        case 17: drawScanline(p, w, h); break;

        case 18: drawDNA(p, w, h); break;

        case 19: drawNumbers(p, w, h); break;

        case 20: drawShadedCube(p, w, h); break;

        case 21: drawCopperBars(p, w, h); break;

        case 22: drawPlasma(p, w, h); break;

        case 23: drawVectorLandscape(p, w, h); break;

        case 24: drawAnalogVU(p, w, h); break;

        case 25: drawCircularSpectrum(p, w, h); break;

        case 26: drawKaleidoscope(p, w, h); break;

        case 27: drawHexHive(p, w, h); break;

        case 28: drawSierpinski(p, w, h); break;

        case 29: drawLissajous3D(p, w, h); break;

        case 30: drawRaindrops(p, w, h); break;

        default: drawWaveform(p, w, h); break;

        }



        if (m_showText) {

            drawScroller(p, w, h);

        }

    }



protected:

    void paintEvent(QPaintEvent *) override {

        QPainter p(this);

        renderScene(p, width(), height());

    }



private:

    void saveFrame() {

        int w = width(); int h = height();

        if(m_recResolution == 1) { w = 1280; h = 720; }

        else if(m_recResolution == 2) { w = 1920; h = 1080; }

        else if(m_recResolution == 3) { w = 3840; h = 2160; }



        QImage img(w, h, QImage::Format_ARGB32);

        QPainter p(&img);

        renderScene(p, w, h);

        p.end();



        QString path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

        path += "/LMMS_Visuals";

        QDir dir(path); if(!dir.exists()) dir.mkpath(".");

        img.save(path + "/" + QString("frame_%1.png").arg(m_recFrameCount++, 4, 10, QChar('0')));

    }



    // DRAW FUNCTIONS

    void drawWaveform(QPainter &p, int w, int h) {

        p.setPen(Qt::green);

        int mid = h/2;

        for (int i = 0; i < BUFFER_SIZE-1; i+=2) {

            p.drawLine((i*w)/BUFFER_SIZE, mid + (int)(g_bufferL[i] * h*0.5),

                       ((i+1)*w)/BUFFER_SIZE, mid + (int)(g_bufferL[i+1] * h*0.5));

        }

    }

    void drawLabSpectrum(QPainter &p, int w, int h) {

        p.setPen(QColor(0, 50, 0));

        for(int x=0; x<w; x+=20) p.drawLine(x, 0, x, h);

        for(int y=0; y<h; y+=20) p.drawLine(0, y, w, y);

        auto bins = computeComplexFFT(g_bufferL);

        int bars = bins.size(); float barW = (float)w / bars;

        p.setBrush(QColor(0, 255, 0, 150)); p.setPen(QColor(0, 255, 0));

        for(int i=0; i<bars; ++i) {

            float mag = std::abs(bins[i]) * 100.0f; if(mag>1) mag=1;

            int bh = (int)(mag * h);

            p.drawRect((int)(i*barW), h - bh, (int)barW - 1, bh);

        }

    }

    void drawPhaseScope(QPainter &p, int w, int h) {

        p.setPen(QColor(0, 255, 255, 100)); 

        int cx = w/2; int cy = h/2; int scale = std::min(cx, cy);

        p.drawLine(0, h, w, 0); p.drawLine(0, 0, w, h);

        QPoint last(cx, cy);

        for(int i=0; i<BUFFER_SIZE; i+=2) {

            QPoint pt(cx + (int)(g_bufferL[i] * scale), cy - (int)(g_bufferR[i] * scale));

            if(i>0 && (std::abs(pt.x()-last.x()) < 50)) p.drawLine(last, pt);

            last = pt;

        }

    }

    void drawComplexPlane(QPainter &p, int w, int h) {

        p.setPen(Qt::white); p.drawText(10, 20, "COMPLEX PLANE");

        int cx = w/2; int cy = h/2;

        p.setPen(Qt::gray); p.drawLine(cx, 0, cx, h); p.drawLine(0, cy, w, cy);

        auto bins = computeComplexFFT(g_bufferL);

        p.setPen(Qt::yellow);

        for(auto &c : bins) p.drawPoint(cx + (int)(c.real()*5000), cy - (int)(c.imag()*5000));

    }

    void drawTopFrequencies(QPainter &p, int w, int h) {

        p.setPen(Qt::green); QFont f = p.font(); f.setPointSize(12); f.setFamily("Courier"); p.setFont(f);

        auto bins = computeComplexFFT(g_bufferL);

        struct Peak { int idx; float mag; }; std::vector<Peak> peaks;

        for(size_t i=1; i<bins.size(); ++i) peaks.push_back({ (int)i, std::abs(bins[i]) });

        std::sort(peaks.begin(), peaks.end(), [](Peak a, Peak b){ return a.mag > b.mag; });

        p.drawText(10, 25, "DOMINANT FREQUENCIES:"); p.setPen(Qt::gray); p.drawLine(10, 30, 250, 30);

        for(int i=0; i<5 && i<peaks.size(); ++i) {

            float hz = peaks[i].idx * SAMPLE_RATE / BUFFER_SIZE;

            QString s = QString("#%1: %2 Hz (%3)").arg(i+1).arg((int)hz).arg(freqToNote(hz));

            int barW = (int)(peaks[i].mag * 5000.0f); if(barW > 200) barW = 200;

            p.setBrush(QColor(255, 100, 100)); p.setPen(Qt::NoPen);

            p.drawRect(10, 50 + i*30, barW, 15);

            p.setPen(Qt::green); p.drawText(230, 62 + i*30, s);

        }

    }

    void drawColorOrgan(QPainter &p, int w, int h) {

        auto bins = computeComplexFFT(g_bufferL);

        int maxIdx = 0; float maxMag = 0; float totalVol = 0;

        for(size_t i=1; i<bins.size(); ++i) {

            float m = std::abs(bins[i]); totalVol += m;

            if(m > maxMag) { maxMag = m; maxIdx = i; }

        }

        int hue = (maxIdx * 5) % 360; int sat = std::min(255, (int)(totalVol * 2000.0f));

        p.fillRect(0, 0, w, h, QColor::fromHsv(hue, sat, std::min(255, sat+50)));

    }

    void drawHypnoSpiral(QPainter &p, int w, int h) {

        int cx = w/2; int cy = h/2;

        float vol = getReactionLevel(); // REACTIVE

        QColor c; c.setHsv(m_hue, 255, 255); p.setPen(QPen(c, 2));

        for (int i = 0; i < 200; ++i) {

            float t = i * 0.1f - (m_hue * 0.05f);

            float r = i + (vol * 50.0f);

            p.drawPoint(cx + (int)(r * cos(t)), cy + (int)(r * sin(t)));

        }

    }

    void drawFire(QPainter &p, int w, int h) {

        const int fw = 40; const int fh = 25;

        // Reactive Fire: ignition depends on selected band level

        float ignition = getReactionLevel();

        for(int x=0; x<fw; ++x) {

            if((rand()%100) < (ignition * 200.0f)) m_fire[x][fh-1] = 255; else m_fire[x][fh-1] = 0;

        }

        for(int y=0; y < fh-1; ++y) {

            for(int x=0; x<fw; ++x) {

                int src = m_fire[x][y+1]; if(x>0) src += m_fire[x-1][y+1]; if(x<fw-1) src += m_fire[x+1][y+1];

                src = (src/3) - 2; if(src < 0) src = 0; m_fire[x][y] = src;

            }

        }

        int cellW = w / fw; int cellH = h / fh; p.setPen(Qt::NoPen);

        for(int y=0; y<fh; ++y) for(int x=0; x<fw; ++x) {

            int val = m_fire[x][y];

            if(val > 0) {

                QColor c; if(val > 180) c.setRgb(255, 255, 0); else if(val > 80) c.setRgb(255, val, 0); else c.setRgb(val*3, 0, 0);

                p.fillRect(x*cellW, y*cellH, cellW+1, cellH+1, c);

            }

        }

    }

    void drawStarfield(QPainter &p, int w, int h) {

        int cx = w/2; int cy = h/2; p.setPen(Qt::white);

        float speed = 2.0f + (getReactionLevel() * 10.0f); // REACTIVE SPEED

        for(auto &s : m_stars) {

            s.z -= speed; if(s.z <= 0) { s.z = 1000; s.x = (rand()%2000)-1000; s.y = (rand()%2000)-1000; }

            float k = 250.0f / s.z; int sz = (int)(2.0f * k); if(sz<1) sz=1;

            p.drawRect(cx + (int)(s.x * k), cy + (int)(s.y * k), sz, sz);

        }

    }

    void drawData(QPainter &p, int w, int h) {

        p.setPen(Qt::green); QFont f = p.font(); f.setPointSize(10); f.setFamily("Monospace"); p.setFont(f);

        float maxL=0, maxR=0, sumL=0; int zeroCross = 0; float last = 0;

        for(int i=0; i<BUFFER_SIZE; ++i) {

            if(std::abs(g_bufferL[i]) > maxL) maxL = std::abs(g_bufferL[i]);

            if(std::abs(g_bufferR[i]) > maxR) maxR = std::abs(g_bufferR[i]);

            sumL += g_bufferL[i]*g_bufferL[i];

            if(i>0 && ((last > 0 && g_bufferL[i] < 0) || (last < 0 && g_bufferL[i] > 0))) zeroCross++;

            last = g_bufferL[i];

        }

        float rms = sqrt(sumL/BUFFER_SIZE); float db = 20 * log10(rms + 0.0001f);

        int y = 20; auto line = [&](QString l, QString v) { p.drawText(10, y, l); p.drawText(120, y, v); y+=20; };

        line("STATUS:", "ONLINE");

        line("RMS (L):", QString::number(rms, 'f', 4));

        line("PEAK (L):", QString::number(maxL, 'f', 4));

        line("PEAK (R):", QString::number(maxR, 'f', 4));

        line("dB LEVEL:", QString::number(db, 'f', 1) + " dB");

        line("Z-CROSS:", QString::number(zeroCross));

    }

    void drawDaftGrid(QPainter &p, int w, int h) {

        int cols = 16; int rows = 10; int cw = w/cols; int ch = h/rows;

        auto bins = computeComplexFFT(g_bufferL);

        for(int x=0; x<cols; ++x) {

            float mag = std::abs(bins[x]) * 100.0f; int lit = (int)(mag * rows);

            for(int y=0; y<rows; ++y) {

                if((rows - y) < lit) p.setBrush(QColor(255, 0, 50)); else p.setBrush(QColor(50, 0, 0));

                p.setPen(Qt::black); p.drawEllipse(x*cw+2, y*ch+2, cw-4, ch-4);

            }

        }

    }

    void drawBinaryRain(QPainter &p, int w, int h) {

        p.setPen(Qt::green); QFont f = p.font(); f.setFamily("Monospace"); f.setBold(true); p.setFont(f);

        int cols = w / 15;

        for(int i=0; i<cols; ++i) {

            int idx = (i * BUFFER_SIZE) / cols;

            if(std::abs(g_bufferL[idx]) > 0.05f) {

                QString bin = (rand()%2 == 0) ? "0" : "1";

                int y = (m_hue * 5 + i * 20) % h;

                p.drawText(i * 15, y, bin);

            }

        }

    }

    void drawGlitch(QPainter &p, int w, int h) {

        drawWaveform(p, w, h);

        for(int i=0; i<10; ++i) {

            if((rand()%10) > 7) { 

                int y = rand() % h; int shift = (rand() % 40) - 20;

                p.setPen(Qt::red); p.drawLine(0, y, w, y+shift);

            }

        }

    }

    void drawRetroSun(QPainter &p, int w, int h) {

        p.setPen(QColor(255, 0, 255));

        int horizon = h/2 + 20;

        p.drawLine(0, horizon, w, horizon);

        for(int i=0; i<w; i+=40) p.drawLine(w/2, horizon, i - (w/2 - i)*2, h);

        for(int i=horizon; i<h; i+=20) p.drawLine(0, i, w, i);

        float kick = getReactionLevel() * 100.0f; // REACTIVE

        int r = 50 + (int)kick;

        QRadialGradient grad(w/2, horizon-40, r);

        grad.setColorAt(0, Qt::yellow); grad.setColorAt(1, QColor(255, 0, 100));

        p.setBrush(grad); p.setPen(Qt::NoPen); p.drawEllipse(QPoint(w/2, horizon-40), r, r);

    }

    void drawTunnel(QPainter &p, int w, int h) {

        float kick = getReactionLevel() * 50.0f; // REACTIVE

        p.setPen(Qt::white); p.setBrush(Qt::NoBrush);

        int cx = w/2; int cy = h/2;

        for(int i=1; i<10; ++i) {

            int r = (m_hue * 2 + i * 40) % 300; r += kick;

            p.drawEllipse(cx - r, cy - r, r*2, r*2);

        }

    }

    void drawParticles(QPainter &p, int w, int h) {

        for(auto &pt : m_particles) { 

            int x = (int)((pt.x / width()) * w);

            int y = (int)((pt.y / height()) * h);

            p.setPen(pt.color); p.drawPoint(x, y); 

        }

    }

    void drawWaveform2(QPainter &p, int w, int h) {

        p.setPen(Qt::NoPen); p.setBrush(QColor(0, 255, 255, 100));

        QPolygon poly; poly << QPoint(0, h/2);

        for(int i=0; i<BUFFER_SIZE; ++i) poly << QPoint((i*w)/BUFFER_SIZE, h/2 - std::abs(g_bufferL[i])*h/2);

        poly << QPoint(w, h/2);

        for(int i=BUFFER_SIZE-1; i>=0; --i) poly << QPoint((i*w)/BUFFER_SIZE, h/2 + std::abs(g_bufferL[i])*h/2);

        p.drawPolygon(poly);

    }

    void drawScanline(QPainter &p, int w, int h) {

        drawWaveform(p, w, h);

        p.setPen(QColor(0, 0, 0, 100)); for(int y=0; y<h; y+=4) p.drawLine(0, y, w, y);

        int roll = (m_hue * 5) % h;

        p.setBrush(QColor(255, 255, 255, 50)); p.setPen(Qt::NoPen); p.drawRect(0, roll, w, 30);

    }

    void drawDNA(QPainter &p, int w, int h) {

        for(int i=0; i<w; i+=5) {

            float phase = (i * 0.05f) + (m_hue * 0.1f);

            int y1 = h/2 + (int)(sin(phase) * 50); int y2 = h/2 + (int)(cos(phase) * 50); 

            int t = 2 + (int)(std::abs(g_bufferL[(i*BUFFER_SIZE)/w]) * 10);

            p.setPen(QPen(Qt::cyan, t)); p.drawPoint(i, y1);

            p.setPen(QPen(Qt::magenta, t)); p.drawPoint(i, y2);

            if(i%20 == 0) { p.setPen(Qt::gray); p.drawLine(i, y1, i, y2); } 

        }

    }

    void drawNumbers(QPainter &p, int w, int h) {

        p.setPen(Qt::green); QFont f = p.font(); f.setFamily("Courier");

        f.setPointSize(10); p.setFont(f);

        int cols = 20; int rows = 15;

        for(int y=0; y<rows; ++y) for(int x=0; x<cols; ++x) 

            if((rand()%100) > 90) p.drawText(x*(w/cols), y*(h/rows), QString::number(rand()%9));

    }

    void drawShadedCube(QPainter &p, int w, int h) {

        Point3D verts[8] = {{-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1}, {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}};

        int faces[6][4] = {{0,1,2,3}, {4,5,6,7}, {0,1,5,4}, {2,3,7,6}, {0,3,7,4}, {1,2,6,5}};

        float angleX = m_hue*0.02f; float angleY = m_hue*0.03f; 

        

        float kick = 1.0f + getReactionLevel(); // REACTIVE

        

        int cx = w/2; int cy = h/2; QPoint p2d[8];

        for(int i=0; i<8; ++i) {

            float x = verts[i].x*kick; float y = verts[i].y*kick; float z = verts[i].z*kick;

            float tx = x*cos(angleY)-z*sin(angleY); float tz = x*sin(angleY)+z*cos(angleY); x=tx; z=tz;

            float ty = y*cos(angleX)-z*sin(angleX); z = y*sin(angleX)+z*cos(angleX); y=ty;

            float dist = 4.0f + z; if(dist < 0.1f) dist = 0.1f; float scale = 300.0f / dist;

            p2d[i] = QPoint(cx + (int)(x*scale), cy + (int)(y*scale));

        }

        for(int i=0; i<6; ++i) {

            QPolygon poly; for(int j=0; j<4; ++j) poly << p2d[faces[i][j]];

            int hue = (m_hue + i * 40) % 360;

            p.setBrush(QColor::fromHsv(hue, 200, 200, 150)); p.setPen(Qt::white); p.drawPolygon(poly);

        }

    }

    void drawCopperBars(QPainter &p, int w, int h) {

        for(int i=0; i<10; ++i) {

            int y = (m_hue * 3 + i * 50) % h;

            for(int j=0; j<30; ++j) {

                int col = 255 - std::abs(j-15)*15;

                p.setPen(QColor(col, 0, 0)); p.drawLine(0, y+j, w, y+j);

            }

        }

        drawWaveform(p, w, h);

    }

    void drawPlasma(QPainter &p, int w, int h) {

        int cell = 10; p.setPen(Qt::NoPen); float t = m_hue * 0.1f;

        for(int y=0; y<h; y+=cell) for(int x=0; x<w; x+=cell) {

            float v = sin(x*0.01f + t) + sin(y*0.01f + t) + sin((x+y)*0.01f + t) + sin(sqrt(x*x+y*y)*0.01f + t);

            int c = (int)((v + 4.0f) * 32.0f); p.setBrush(QColor::fromHsv(c % 255, 255, 255));

            p.drawRect(x, y, cell, cell);

        }

    }

    void drawVectorLandscape(QPainter &p, int w, int h) {

        p.setPen(Qt::cyan); int cx = w/2; int cy = h/2; int horizon = cy;

        p.drawLine(0, horizon, w, horizon); float speed = m_hue % 20;

        for(int x=-500; x<w+500; x+=100) p.drawLine(cx, horizon, x, h);

        for(int z=1; z<10; ++z) { int y = horizon + (z * z * 5) + (int)speed; if(y < h) p.drawLine(0, y, w, y); }

    }

    void drawAnalogVU(QPainter &p, int w, int h) {

        p.setBrush(Qt::white); p.setPen(Qt::black);

        QRect box(50, 50, w-100, h-100); p.drawRect(box);

        p.setPen(QPen(Qt::black, 2)); p.drawArc(box.left()+20, box.top()+20, box.width()-40, box.height()*2-40, 45*16, 90*16);

        float rms = 0; for(int i=0; i<BUFFER_SIZE; ++i) rms += g_bufferL[i]*g_bufferL[i];

        rms = sqrt(rms/BUFFER_SIZE) * 5.0f; if(rms>1) rms=1;

        float angle = 135.0f - (rms * 90.0f); float rad = angle * M_PI / 180.0f;

        int cx = w/2; int cy = box.bottom()-20; int len = box.height() - 40;

        p.setPen(QPen(Qt::red, 3)); p.drawLine(cx, cy, cx + (int)(len*cos(rad)), cy - (int)(len*sin(rad)));

    }

    void drawCircularSpectrum(QPainter &p, int w, int h) {

        auto bins = computeComplexFFT(g_bufferL); int cx = w/2; int cy = h/2; p.setPen(Qt::green);

        for(size_t i=0; i<bins.size(); ++i) {

            float mag = std::abs(bins[i]) * 100.0f; float angle = (i * 2.0f * M_PI) / bins.size();

            float r1 = 50.0f; float r2 = 50.0f + mag * 2.0f;

            p.drawLine(cx + (int)(r1*cos(angle)), cy + (int)(r1*sin(angle)), cx + (int)(r2*cos(angle)), cy + (int)(r2*sin(angle)));

        }

    }

    void drawKaleidoscope(QPainter &p, int w, int h) {

        p.setPen(Qt::magenta); int cx = w/2; int cy = h/2;

        for(int i=0; i<BUFFER_SIZE-1; i+=2) {

            float r = 50.0f + g_bufferL[i] * 50.0f; float a = (i * 2.0f * M_PI) / BUFFER_SIZE;

            int x = (int)(r * cos(a)); int y = (int)(r * sin(a));

            p.drawPoint(cx+x, cy+y); p.drawPoint(cx-x, cy+y); p.drawPoint(cx+x, cy-y); p.drawPoint(cx-x, cy-y);

        }

    }

    void drawHexHive(QPainter &p, int w, int h) {

        int r = 30; float hh = r * sqrt(3.0f); int cols = w / (r*1.5f); int rows = h / hh;

        auto bins = computeComplexFFT(g_bufferL);

        for(int y=0; y<rows; ++y) for(int x=0; x<cols; ++x) {

            int idx = (x + y*cols) % bins.size(); float mag = std::abs(bins[idx]) * 50.0f;

            int px = x * r * 1.5f; int py = y * hh + (x%2)*hh/2;

            if(mag > 0.5f) p.setBrush(Qt::yellow); else p.setBrush(Qt::NoBrush); p.setPen(Qt::yellow);

            QPolygon poly; for(int i=0; i<6; ++i) { float ang = i * 60 * M_PI / 180; poly << QPoint(px + r*cos(ang), py + r*sin(ang)); }

            p.drawPolygon(poly);

        }

    }

    void drawSierpinski(QPainter &p, int w, int h) {

        float kick = 1.0f + getReactionLevel(); // REACTIVE

        QPoint p1(w/2, 10); QPoint p2(10, h-10); QPoint p3(w-10, h-10);

        std::function<void(QPoint, QPoint, QPoint, int)> drawTri = [&](QPoint a, QPoint b, QPoint c, int d) {

            if(d == 0) { QPolygon poly; poly << a << b << c; p.setBrush(QColor(0, 255*kick/2, 0)); p.drawPolygon(poly); }

            else { QPoint ab((a.x()+b.x())/2, (a.y()+b.y())/2); QPoint bc((b.x()+c.x())/2, (b.y()+c.y())/2); QPoint ca((c.x()+a.x())/2, (c.y()+a.y())/2);

                   drawTri(a, ab, ca, d-1); drawTri(ab, b, bc, d-1); drawTri(ca, bc, c, d-1); }

        };

        drawTri(p1, p2, p3, 4);

    }

    void drawLissajous3D(QPainter &p, int w, int h) {

        int cx = w/2; int cy = h/2; p.setPen(Qt::cyan); float t = m_hue * 0.05f;

        for(int i=0; i<200; ++i) {

            float u = i * 0.1f; float x = 100 * sin(u + t); float y = 100 * sin(2*u + t); float z = 100 * sin(3*u + t);

            float scale = 300.0f / (400.0f + z); p.drawPoint(cx + x*scale, cy + y*scale);

        }

    }

    void drawRaindrops(QPainter &p, int w, int h) {

        static std::vector<QPoint> drops; static std::vector<int> radius;

        if(std::abs(g_bufferL[0]) > 0.5f) { drops.push_back(QPoint(rand()%w, rand()%h)); radius.push_back(1); }

        p.setPen(Qt::blue); p.setBrush(Qt::NoBrush);

        for(size_t i=0; i<drops.size(); ++i) { p.drawEllipse(drops[i], radius[i], radius[i]); radius[i] += 2; }

        if(drops.size() > 20) { drops.erase(drops.begin()); radius.erase(radius.begin()); }

    }

    void drawScroller(QPainter &p, int w, int h) {

        p.setPen(Qt::white); QFont f = p.font(); f.setPointSize(20); f.setBold(true); p.setFont(f);

        QString text = m_text.isEmpty() ? QString::fromUtf8("YOUR TEXT") : m_text; QFontMetrics fm(f);

        static int scrollX = 0; scrollX = (scrollX + 4);

        if(scrollX > w + fm.horizontalAdvance(text) + 50) scrollX = -w; 

        int startX = w - scrollX; int cy = h / 2;

        if(!m_wiggleText) { p.drawText(startX, cy + 10, text); } else {

            int currentX = startX; for(int i=0; i<text.length(); ++i) {

                QChar c = text.at(i); float time = m_hue * 0.1f; 

                int yOffset = (int)(30.0f * sin((currentX * 0.02f) + time));

                p.drawText(currentX, cy + 10 + yOffset, QString(c)); currentX += fm.horizontalAdvance(c);

            }

        }

    }



    int m_mode;

    bool m_showText;

    bool m_wiggleText;

    int m_hue;

    int m_randomModeIndex; 

    int m_cycleCounter;    

    bool m_isRecording;

    int m_recResolution;

    int m_recFrameCount;

    int m_reactionBand;

    QString m_text;

    std::vector<Star> m_stars;

    int m_fire[40][25];

    std::vector<Particle> m_particles;        

};



// ================================================================

// EFFECT CONTROLS

// ================================================================

void VisualiserEffectControls::saveSettings(QDomDocument &doc, QDomElement &parent) {

    QDomElement e = doc.createElement(nodeName());

    e.setAttribute("mode", m_mode);

    e.setAttribute("showText", m_showText ? 1 : 0);

    e.setAttribute("wiggleText", m_wiggleText ? 1 : 0);

    e.setAttribute("resolution", m_recResolution);

    e.setAttribute("band", m_reactionBand);

    e.setAttribute("text", m_text);

    parent.appendChild(e);

}



void VisualiserEffectControls::loadSettings(const QDomElement &parent) {

    QDomElement e = parent.firstChildElement(nodeName());

    if (!e.isNull()) {

        m_mode = e.attribute("mode", "0").toInt();

        m_showText = e.attribute("showText", "1").toInt() != 0;

        m_wiggleText = e.attribute("wiggleText", "0").toInt() != 0;

        m_recResolution = e.attribute("resolution", "0").toInt();

        m_reactionBand = e.attribute("band", "0").toInt();

        m_text = e.attribute("text", QString::fromUtf8("YOUR TEXT"));

    }

}



lmms::gui::EffectControlDialog *VisualiserEffectControls::createView() {

    return new VisualiserControlDialog(this);

}



// ================================================================

// CONTROL DIALOG

// ================================================================

VisualiserControlDialog::VisualiserControlDialog(EffectControls *controls)

    : EffectControlDialog(controls)

    , m_controls(dynamic_cast<VisualiserEffectControls*>(controls))

{

    setWindowTitle("Visualiser");

    resize(600, 400); 

    QVBoxLayout *main = new QVBoxLayout(this);

    m_screen = new VisualiserScreen(this);

    main->addWidget(m_screen);



    QHBoxLayout *ml = new QHBoxLayout;

    m_modeCombo = new QComboBox(this);

    m_modeCombo->addItem("0. Waveform");

    m_modeCombo->addItem("1. Lab Spectrum");

    m_modeCombo->addItem("2. Phase Scope");

    m_modeCombo->addItem("3. Complex Plane");

    m_modeCombo->addItem("4. Top Frequencies");

    m_modeCombo->addItem("5. Color Organ");

    m_modeCombo->addItem("6. Hypno Spiral");

    m_modeCombo->addItem("7. Amiga Fire");

    m_modeCombo->addItem("8. Starfield");

    m_modeCombo->addItem("9. Data Stats");

    m_modeCombo->addItem("10. Daft Grid");

    m_modeCombo->addItem("11. Binary Rain");

    m_modeCombo->addItem("12. Glitch/Moshing");

    m_modeCombo->addItem("13. Retro Sun");

    m_modeCombo->addItem("14. Tunnel");

    m_modeCombo->addItem("15. Particles");

    m_modeCombo->addItem("16. Waveform 2"); 

    m_modeCombo->addItem("17. Scanline TV");

    m_modeCombo->addItem("18. DNA Helix");

    m_modeCombo->addItem("19. Numbers (Background)");

    m_modeCombo->addItem("20. Shaded Cube");

    m_modeCombo->addItem("21. Copper Bars");

    m_modeCombo->addItem("22. Plasma");

    m_modeCombo->addItem("23. Vector Landscape");

    m_modeCombo->addItem("24. Analog VU");

    m_modeCombo->addItem("25. Circular Spectrum");

    m_modeCombo->addItem("26. Kaleidoscope");

    m_modeCombo->addItem("27. Hex Hive");

    m_modeCombo->addItem("28. Sierpinski");

    m_modeCombo->addItem("29. Lissajous 3D");

    m_modeCombo->addItem("30. Raindrops");

    m_modeCombo->addItem("31. Random Cycle"); 



    ml->addWidget(new QLabel("Mode:", this));

    ml->addWidget(m_modeCombo);

    main->addLayout(ml);



    QHBoxLayout *tl = new QHBoxLayout;

    m_textCheck = new QCheckBox("Text", this);

    m_wiggleCheck = new QCheckBox("Wiggle", this); 

    tl->addWidget(m_textCheck);

    tl->addWidget(m_wiggleCheck);

    m_textBox = new QLineEdit(this);

    tl->addWidget(new QLabel("Text:", this));

    tl->addWidget(m_textBox);

    main->addLayout(tl);

    

    // EXPORT & BAND ROW

    QHBoxLayout *rl = new QHBoxLayout;

    m_recCheck = new QCheckBox("RECORD", this);

    m_resCombo = new QComboBox(this);

    m_resCombo->addItem("Window"); m_resCombo->addItem("720p"); m_resCombo->addItem("1080p"); m_resCombo->addItem("4K");

    

    m_bandCombo = new QComboBox(this);

    m_bandCombo->addItem("All"); m_bandCombo->addItem("Bass"); m_bandCombo->addItem("Mids"); m_bandCombo->addItem("Treble");



    rl->addWidget(new QLabel("Export:", this)); rl->addWidget(m_recCheck);

    rl->addWidget(new QLabel("Res:", this)); rl->addWidget(m_resCombo);

    rl->addWidget(new QLabel("React:", this)); rl->addWidget(m_bandCombo);

    main->addLayout(rl);



    if (m_controls) {

        m_modeCombo->setCurrentIndex(m_controls->mode());

        m_textCheck->setChecked(m_controls->showText());

        m_wiggleCheck->setChecked(m_controls->wiggleText());

        m_resCombo->setCurrentIndex(m_controls->recResolution());

        m_bandCombo->setCurrentIndex(m_controls->reactionBand());

        m_textBox->setText(m_controls->text());

        m_screen->setMode(m_controls->mode());

        m_screen->setShowText(m_controls->showText());

        m_screen->setWiggleText(m_controls->wiggleText());

        m_screen->setRecResolution(m_controls->recResolution());

        m_screen->setReactionBand(m_controls->reactionBand());

        m_screen->setText(m_controls->text());

    }



    connect(m_modeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onModeChanged(int)));

    connect(m_textCheck, SIGNAL(toggled(bool)), this, SLOT(onShowTextChanged(bool)));

    connect(m_wiggleCheck, SIGNAL(toggled(bool)), this, SLOT(onWiggleTextChanged(bool)));

    connect(m_recCheck, SIGNAL(toggled(bool)), this, SLOT(onRecToggled(bool)));

    connect(m_resCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onResChanged(int)));

    connect(m_bandCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onBandChanged(int)));

    connect(m_textBox, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));

}



void VisualiserControlDialog::onModeChanged(int index) {

    if (m_controls) m_controls->setMode(index);

    if (m_screen) m_screen->setMode(index);

}

void VisualiserControlDialog::onShowTextChanged(bool checked) {

    if (m_controls) m_controls->setShowText(checked);

    if (m_screen) m_screen->setShowText(checked);

}

void VisualiserControlDialog::onWiggleTextChanged(bool checked) {

    if (m_controls) m_controls->setWiggleText(checked);

    if (m_screen) m_screen->setWiggleText(checked);

}

void VisualiserControlDialog::onRecToggled(bool checked) {

    if (m_screen) m_screen->setRecState(checked);

}

void VisualiserControlDialog::onResChanged(int index) {

    if (m_controls) m_controls->setRecResolution(index);

    if (m_screen) m_screen->setRecResolution(index);

}

void VisualiserControlDialog::onBandChanged(int index) {

    if (m_controls) m_controls->setReactionBand(index);

    if (m_screen) m_screen->setReactionBand(index);

}

void VisualiserControlDialog::onTextChanged(const QString &text) {

    if (m_controls) m_controls->setText(text);

    if (m_screen) m_screen->setText(text);

}



// ================================================================

// MAIN EFFECT CLASS

// ================================================================

class VisualiserEffect : public Effect

{

public:

    VisualiserEffect(Model *parent, const QDomElement &node)

        : Effect(&Visualiser_plugin_descriptor, parent, nullptr)

        , m_controls(this)

    {

        Q_UNUSED(node);

    }

    EffectControls *controls() override { return &m_controls; }

    ProcessStatus processImpl(SampleFrame *buffer, const fpp_t frames) override

    {

        int n = static_cast<int>(frames);

        if (n > BUFFER_SIZE) n = BUFFER_SIZE;

        int keep = BUFFER_SIZE - n;

        for (int i = 0; i < keep; ++i) {

            g_bufferL[i] = g_bufferL[i + n];

            g_bufferR[i] = g_bufferR[i + n];

        }

        for (int i = 0; i < n; ++i) {

            g_bufferL[keep + i] = buffer[i][0]; 

            g_bufferR[keep + i] = buffer[i][1]; 

        }

        return ProcessStatus::Continue;

    }

private:

    VisualiserEffectControls m_controls;

};



extern "C"

{

    LMMS_EXPORT Plugin::Descriptor Visualiser_plugin_descriptor =

    {

        "Visualiser",

        "Visualiser",

        QT_TRANSLATE_NOOP("PluginBrowser","Visualiser Plugin"),

        "Your Name",

        0x0100,

        Plugin::Type::Effect,

        nullptr,

        nullptr,

        nullptr

    };



    LMMS_EXPORT Plugin *lmms_plugin_main(lmms::Model *parent, void *data)

    {

        Q_UNUSED(data);

        return new VisualiserEffect(parent, QDomElement());

    }

}
