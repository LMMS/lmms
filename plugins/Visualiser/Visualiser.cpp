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
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <random> // Added for thread-safe random generation

namespace lmms
{
extern "C" LMMS_EXPORT Plugin::Descriptor Visualiser_plugin_descriptor;

#define BUFFER_SIZE 512
static float g_bufferL[BUFFER_SIZE];
static float g_bufferR[BUFFER_SIZE];
#define SAMPLE_RATE 44100.0f

// ================================================================
// SIMPLE FFT
// ================================================================
std::vector<std::complex<float>> computeComplexFFT(float* data)
{
    int bins = 128;
    std::vector<std::complex<float>> output(bins);
    for (int k = 0; k < bins; ++k)
    {
        std::complex<float> sum(0, 0);
        for (int t = 0; t < BUFFER_SIZE; t += 2)
        {
            float angle = 2.0f * M_PI * t * k / BUFFER_SIZE;
            sum += std::complex<float>(data[t] * std::cos(angle),
                                       data[t] * -std::sin(angle));
        }
        output[k] = sum / (float)BUFFER_SIZE;
    }
    return output;
}

// ================================================================
// FREQ -> NOTE
// ================================================================
QString freqToNote(float freq)
{
    if (freq < 20.0f) return "--";
    int midi = std::round(69 + 12 * std::log2(freq / 440.0f));
    const char* notes[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    int noteIndex = midi % 12;
    int octave = (midi / 12) - 1;
    if (noteIndex < 0) noteIndex = 0;
    return QString("%1%2").arg(notes[noteIndex]).arg(octave);
}

// ================================================================
// SIMPLE 2D MAP FOR WOLFENSTEIN-STYLE DUNGEON
// ================================================================
static const int MAP_W = 16;
static const int MAP_H = 16;
static const int g_wolfMap[MAP_H][MAP_W] ={
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1,1,1,0,1,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,1,0,1,0,1,0,1},
    {1,0,1,0,1,1,1,0,0,1,0,0,0,1,0,1},
    {1,0,0,0,0,0,1,0,0,1,1,1,0,0,0,1},
    {1,0,1,0,1,0,1,0,0,0,0,1,0,1,0,1},
    {1,0,1,0,1,0,0,0,1,1,0,0,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,1,0,1,0,0,0,1},
    {1,0,1,0,1,1,1,0,0,1,0,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// ================================================================
// SCREEN WIDGET
// ================================================================
class VisualiserScreen : public QWidget
{
public:
    VisualiserScreen(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_mode(47) // Default to Cycle FFT (Mode 47)
        , m_showText(false) // Default text OFF
        , m_wiggleText(false)
        , m_hue(0)
        , m_randomModeIndex(0)
        , m_cycleCounter(0)
        , m_isRecording(false)
        , m_recResolution(0)
        , m_recFrameCount(0)
        , m_reactionBand(0)
        , m_text(QString::fromUtf8("YOUR TEXT")) // Default text content
        , m_camX(3.5f)
        , m_camY(3.5f)
        , m_dirX(1.0f)
        , m_dirY(0.0f)
        , m_planeX(0.0f)
        , m_planeY(0.66f)
    {
        // Initialize Random Number Generator securely
        std::random_device rd;
        m_rng.seed(rd());

        setMinimumSize(300, 250);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        for (int i = 0; i < 100; ++i)
        {
            m_stars.push_back({
                (float)(randomRange(-1000, 999)),
                (float)(randomRange(-1000, 999)),
                (float)(randomRange(1, 1000))
            });
        }
        
        // zero fire buffer safely
        ::memset(m_fire, 0, sizeof(m_fire));
        QTimer *t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [this]()
        {
            updateLogic();
            if (m_isRecording) saveFrame();
            update();
        });
        t->start(30);
        
        // --- Init New Members ---
        m_prevFrameValid = false;
        m_voxelZ = 0.0f;
        m_fftCycleScene = 0;
        m_lastEnergy = 0.0f;
        m_framesSinceChange = 0;
    }

    // Helper for thread-safe random integers (inclusive)
    int randomRange(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(m_rng);
    }

    float getReactionLevel()
    {
        if (m_reactionBand == 0)
            return std::abs(g_bufferL[0]);
        auto bins = computeComplexFFT(g_bufferL);
        float sum = 0.0f;
        int start = 0, end = 128;
        if (m_reactionBand == 1)      { start = 0;  end = 6;   }  // Low
        else if (m_reactionBand == 2) { start = 6;  end = 30;  }  // Mid
        else if (m_reactionBand == 3) { start = 30; end = 100; }  // High
        for (int i = start; i < end && i < (int)bins.size(); ++i)
            sum += std::abs(bins[i]);
        return sum * 3.0f;
    }

    void updateLogic()
    {
        m_hue = (m_hue + 2) % 360;
        // Old random-cycle mode (31)
        if (m_mode == 31)
        {
            m_cycleCounter++;
            if (m_cycleCounter > 100)
            {
                m_cycleCounter = 0;
                m_randomModeIndex = randomRange(0, 30);
            }
        }
        // particles (mode 15)
        if (m_mode == 15 || (m_mode == 31 && m_randomModeIndex == 15))
        {
            float kick = getReactionLevel();
            if (kick > 0.2f)
            {
                for (int k = 0; k < 3; k++)
                {
                    Particle p;
                    p.x = randomRange(0, width() - 1);
                    p.y = height()/2 + (randomRange(-50, 49));
                    p.vx = (randomRange(-5, 4)) * 0.1f;
                    p.vy = (randomRange(1, 5));
                    p.life = 50;
                    p.color = QColor::fromHsv(m_hue, 200, 255);
                    m_particles.push_back(p);
                }
            }
            for (auto &p : m_particles)
            {
                p.x += p.vx;
                p.y += p.vy;
                p.life--;
            }
            m_particles.erase(
                std::remove_if(m_particles.begin(), m_particles.end(),
                               [](const Particle &p){ return p.life <= 0; }),
                m_particles.end());
        }
        // Wolf dungeon auto-movement
        int effectiveMode = (m_mode == 31) ? m_randomModeIndex : m_mode;
        // Also check if cycle mode (47) picked wolf (32)
        if (m_mode == 47 && m_fftCycleScene == 32) effectiveMode = 32;
        if (effectiveMode == 32)
        {
            float speedBase = 0.03f;
            float speedBoost = getReactionLevel() * 0.2f;  // react to bass
            float moveSpeed = speedBase + speedBoost;
            float nextX = m_camX + m_dirX * moveSpeed;
            float nextY = m_camY + m_dirY * moveSpeed;
            if (nextX >= 0 && nextX < MAP_W &&
                nextY >= 0 && nextY < MAP_H &&
                g_wolfMap[(int)nextY][(int)nextX] == 0)
            {
                m_camX = nextX;
                m_camY = nextY;
            }
            else
            {
                // simple fixed turn when hitting a wall
                float angle = 0.4f; // ~23 degrees
                float oldDirX = m_dirX;
                m_dirX = m_dirX * std::cos(angle) - m_dirY * std::sin(angle);
                m_dirY = oldDirX * std::sin(angle) + m_dirY * std::cos(angle);
                float oldPlaneX = m_planeX;
                m_planeX = m_planeX * std::cos(angle) - m_planeY * std::sin(angle);
                m_planeY = oldPlaneX * std::sin(angle) + m_planeY * std::cos(angle);
            }
        }
        
        // FFT-based random cycle mode (Mode 47)
        if (m_mode == 47)
        {
            float energy = getReactionLevel();
            m_framesSinceChange++;
            float diff = energy - m_lastEnergy;
            m_lastEnergy = energy * 0.8f + m_lastEnergy * 0.2f;
            if (diff > 0.15f && m_framesSinceChange > 10)
            {
                // Pick any scene 0..46, avoiding cycle modes (31 and 47)
                int newScene;
                do {
                    newScene = randomRange(0, 46); 
                } while (newScene == 31 || newScene == 47);
                m_fftCycleScene = newScene;
                m_framesSinceChange = 0;
            }
        }
    }

    void setMode(int m)          { m_mode = m; update(); }
    void setShowText(bool show)  { m_showText = show; update(); }
    void setWiggleText(bool wig) { m_wiggleText = wig; update(); }
    void setRecState(bool rec)   { m_isRecording = rec; if (rec) m_recFrameCount = 0; }
    void setRecResolution(int r) { m_recResolution = r; }
    void setReactionBand(int b)  { m_reactionBand = b; }
    void setText(const QString &t) { m_text = t; update(); }

    void renderScene(QPainter &p, int w, int h)
    {
        // Compute active mode
        int activeMode;
        if (m_mode == 31)
            activeMode = m_randomModeIndex;   // legacy random cycle 0..30
        else if (m_mode == 47)
            activeMode = m_fftCycleScene;     // FFT-based random scene
        else
            activeMode = m_mode;
            
        switch (activeMode)
        {
        case 0:  drawWaveform(p, w, h);          break;
        case 1:  drawLabSpectrum(p, w, h);       break;
        case 2:  drawPhaseScope(p, w, h);        break;
        case 3:  drawComplexPlane(p, w, h);      break;
        case 4:  drawTopFrequencies(p, w, h);    break;
        case 5:  drawColorOrgan(p, w, h);        break;
        case 6:  drawHypnoSpiral(p, w, h);       break;
        case 7:  drawFire(p, w, h);              break;
        case 8:  drawStarfield(p, w, h);         break;
        case 9:  drawData(p, w, h);              break;
        case 10: drawDaftGrid(p, w, h);          break;
        case 11: drawBinaryRain(p, w, h);        break;
        case 12: drawGlitch(p, w, h);            break;
        case 13: drawRetroSun(p, w, h);          break;
        case 14: drawTunnel(p, w, h);            break;
        case 15: drawParticles(p, w, h);         break;
        case 16: drawWaveform2(p, w, h);         break;
        case 17: drawScanline(p, w, h);          break;
        case 18: drawDNA(p, w, h);               break;
        case 19: drawNumbers(p, w, h);           break;
        case 20: drawShadedCube(p, w, h);        break;
        case 21: drawCopperBars(p, w, h);        break;
        case 22: drawPlasma(p, w, h);            break;
        case 23: drawVectorLandscape(p, w, h);   break;
        case 24: drawAnalogVU(p, w, h);          break;
        case 25: drawCircularSpectrum(p, w, h);  break;
        case 26: drawKaleidoscope(p, w, h);      break;
        case 27: drawHexHive(p, w, h);           break;
        case 28: drawSierpinski(p, w, h);        break;
        case 29: drawLissajous3D(p, w, h);       break;
        case 30: drawRaindrops(p, w, h);         break;
        case 32: drawWolfDungeon(p, w, h);       break; 
        
        // NEW & RENUMBERED SCENES
        case 33: drawVHSDamage(p, w, h);         break;
        case 34: drawInfiniteShapeTunnel(p, w, h);   break;
        case 35: drawRecursiveSpinFractal(p, w, h);  break;
        case 36: drawEuroHyperColor(p, w, h);    break;
        case 37: drawEuroRasterBarsXL(p, w, h);  break;
        case 38: drawEuroPolygonCyclotron(p, w, h);  break;
        case 39: drawEuroHardGlitch(p, w, h);    break;
        case 40: drawEuroRotozoom(p, w, h);      break;
        case 41: drawEuroFFTMatrix(p, w, h);     break;
        case 42: drawEuroTextScroller(p, w, h);  break;
        case 43: drawEuroGalaxyTunnel(p, w, h);  break;
        case 44: drawAnalogVideoFeedback(p, w, h);   break;
        case 45: drawRetroCheckerboard(p, w, h);     break;
        case 46: drawIndustrialScope(p, w, h);       break;
        default: drawWaveform(p, w, h);          break;
        }
        if (m_showText)
            drawScroller(p, w, h);
    }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        renderScene(p, width(), height());
    }
private:
    // ============================================================
    // HELPER FOR FEEDBACK / BUFFERS
    // ============================================================
    void ensurePrevFrame(int w, int h)
    {
        if (!m_prevFrameValid || m_prevFrame.size() != QSize(w, h))
        {
            m_prevFrame = QImage(w, h, QImage::Format_ARGB32);
            m_prevFrame.fill(Qt::black);
            m_prevFrameValid = true;
        }
    }
    // ============================================================
    // FRAME EXPORT
    // ============================================================
    void saveFrame()
    {
        int w = width();
        int h = height();
        if      (m_recResolution == 1) { w = 1280; h = 720;  }
        else if (m_recResolution == 2) { w = 1920; h = 1080; }
        else if (m_recResolution == 3) { w = 3840; h = 2160; }
        QImage img(w, h, QImage::Format_ARGB32);
        QPainter p(&img);
        renderScene(p, w, h);
        p.end();
        QString path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        path += "/LMMS_Visuals";
        QDir dir(path);
        if (!dir.exists()) dir.mkpath(".");
        img.save(path + "/" + QString("frame_%1.png").arg(m_recFrameCount++, 4, 10, QChar('0')));
    }
    // ============================================================
    // DRAW FUNCTIONS
    // ============================================================
    void drawWaveform(QPainter &p, int w, int h)
    {
        p.setPen(Qt::green);
        int mid = h/2;
        for (int i = 0; i < BUFFER_SIZE-1; i += 2)
        {
            p.drawLine((i*w)/BUFFER_SIZE,     mid + (int)(g_bufferL[i]   * h*0.5),
                       ((i+1)*w)/BUFFER_SIZE, mid + (int)(g_bufferL[i+1] * h*0.5));
        }
    }
    void drawLabSpectrum(QPainter &p, int w, int h)
    {
        p.setPen(QColor(0, 50, 0));
        for (int x = 0; x < w; x += 20) p.drawLine(x, 0, x, h);
        for (int y = 0; y < h; y += 20) p.drawLine(0, y, w, y);
        auto bins = computeComplexFFT(g_bufferL);
        int bars = (int)bins.size();
        float barW = (float)w / bars;
        p.setBrush(QColor(0, 255, 0, 150));
        p.setPen(QColor(0, 255, 0));
        for (int i = 0; i < bars; ++i)
        {
            float mag = std::abs(bins[i]) * 100.0f;
            if (mag > 1) mag = 1;
            int bh = (int)(mag * h);
            p.drawRect((int)(i*barW), h - bh, (int)barW - 1, bh);
        }
    }
    void drawPhaseScope(QPainter &p, int w, int h)
    {
        p.setPen(QColor(0, 255, 255, 100));
        int cx = w/2; int cy = h/2;
        int scale = std::min(cx, cy);
        p.drawLine(0, h, w, 0);
        p.drawLine(0, 0, w, h);
        QPoint last(cx, cy);
        for (int i = 0; i < BUFFER_SIZE; i += 2)
        {
            QPoint pt(cx + (int)(g_bufferL[i] * scale),
                      cy - (int)(g_bufferR[i] * scale));
            if (i > 0 && (std::abs(pt.x()-last.x()) < 50))
                p.drawLine(last, pt);
            last = pt;
        }
    }
    void drawComplexPlane(QPainter &p, int w, int h)
    {
        p.setPen(Qt::white);
        p.drawText(10, 20, "COMPLEX PLANE");
        int cx = w/2; int cy = h/2;
        p.setPen(Qt::gray);
        p.drawLine(cx, 0, cx, h);
        p.drawLine(0, cy, w, cy);
        auto bins = computeComplexFFT(g_bufferL);
        p.setPen(Qt::yellow);
        for (auto &c : bins)
            p.drawPoint(cx + (int)(c.real()*5000),
                        cy - (int)(c.imag()*5000));
    }
    void drawTopFrequencies(QPainter &p, int w, int h)
    {
        p.setPen(Qt::green);
        QFont f = p.font();
        f.setPointSize(12);
        f.setFamily("Courier");
        p.setFont(f);
        auto bins = computeComplexFFT(g_bufferL);
        struct Peak { int idx; float mag; };
        std::vector<Peak> peaks;
        for (size_t i = 1; i < bins.size(); ++i)
            peaks.push_back({ (int)i, std::abs(bins[i]) });
        std::sort(peaks.begin(), peaks.end(),
                  [](Peak a, Peak b){ return a.mag > b.mag; });
        p.drawText(10, 25, "DOMINANT FREQUENCIES:");
        p.setPen(Qt::gray);
        p.drawLine(10, 30, 250, 30);
        for (int i = 0; i < 5 && i < (int)peaks.size(); ++i)
        {
            float hz = peaks[i].idx * SAMPLE_RATE / BUFFER_SIZE;
            QString s = QString("#%1: %2 Hz (%3)")
                        .arg(i+1)
                        .arg((int)hz)
                        .arg(freqToNote(hz));
            int barW = (int)(peaks[i].mag * 5000.0f);
            if (barW > 200) barW = 200;
            p.setBrush(QColor(255, 100, 100));
            p.setPen(Qt::NoPen);
            p.drawRect(10, 50 + i*30, barW, 15);
            p.setPen(Qt::green);
            p.drawText(230, 62 + i*30, s);
        }
    }
    void drawColorOrgan(QPainter &p, int w, int h)
    {
        auto bins = computeComplexFFT(g_bufferL);
        int maxIdx = 0;
        float maxMag = 0.0f;
        float totalVol = 0.0f;
        for (size_t i = 1; i < bins.size(); ++i)
        {
            float m = std::abs(bins[i]);
            totalVol += m;
            if (m > maxMag)
            {
                maxMag = m;
                maxIdx = (int)i;
            }
        }
        int hue = (maxIdx * 5) % 360;
        int sat = std::min(255, (int)(totalVol * 2000.0f));
        p.fillRect(0, 0, w, h,
                   QColor::fromHsv(hue, sat,
                                   std::min(255, sat+50)));
    }
    void drawHypnoSpiral(QPainter &p, int w, int h)
    {
        int cx = w/2; int cy = h/2;
        float vol = getReactionLevel();
        QColor c; c.setHsv(m_hue, 255, 255);
        p.setPen(QPen(c, 2));
        for (int i = 0; i < 200; ++i)
        {
            float t = i * 0.1f - (m_hue * 0.05f);
            float r = i + (vol * 50.0f);
            p.drawPoint(cx + (int)(r * std::cos(t)),
                        cy + (int)(r * std::sin(t)));
        }
    }
    void drawFire(QPainter &p, int w, int h)
    {
        const int fw = 40;
        const int fh = 25;
        float ignition = getReactionLevel();
        for (int x = 0; x < fw; ++x)
        {
            if (randomRange(0, 99) < (ignition * 200.0f))
                m_fire[x][fh-1] = 255;
            else
                m_fire[x][fh-1] = 0;
        }
        for (int y = 0; y < fh-1; ++y)
        {
            for (int x = 0; x < fw; ++x)
            {
                int src = m_fire[x][y+1];
                if (x > 0)      src += m_fire[x-1][y+1];
                if (x < fw-1)   src += m_fire[x+1][y+1];
                src = (src/3) - 2;
                if (src < 0) src = 0;
                m_fire[x][y] = src;
            }
        }
        int cellW = w / fw;
        int cellH = h / fh;
        p.setPen(Qt::NoPen);
        for (int y = 0; y < fh; ++y)
        {
            for (int x = 0; x < fw; ++x)
            {
                int val = m_fire[x][y];
                if (val > 0)
                {
                    QColor c;
                    if (val > 180)      c.setRgb(255, 255, 0);
                    else if (val > 80) c.setRgb(255, val, 0);
                    else               c.setRgb(val*3, 0, 0);
                    p.fillRect(x*cellW, y*cellH, cellW+1, cellH+1, c);
                }
            }
        }
    }
    void drawStarfield(QPainter &p, int w, int h)
    {
        int cx = w/2; int cy = h/2;
        p.setPen(Qt::white);
        float speed = 2.0f + (getReactionLevel() * 10.0f);
        for (auto &s : m_stars)
        {
            s.z -= speed;
            if (s.z <= 0)
            {
                s.z = 1000;
                s.x = randomRange(-1000, 999);
                s.y = randomRange(-1000, 999);
            }
            float k = 250.0f / s.z;
            int sz = (int)(2.0f * k);
            if (sz < 1) sz = 1;
            p.drawRect(cx + (int)(s.x * k),
                       cy + (int)(s.y * k),
                       sz, sz);
        }
    }
    void drawData(QPainter &p, int w, int h)
    {
        p.setPen(Qt::green);
        QFont f = p.font();
        f.setPointSize(10);
        f.setFamily("Monospace");
        p.setFont(f);
        float maxL = 0, maxR = 0, sumL = 0;
        int zeroCross = 0;
        float last = 0;
        for (int i = 0; i < BUFFER_SIZE; ++i)
        {
            if (std::abs(g_bufferL[i]) > maxL) maxL = std::abs(g_bufferL[i]);
            if (std::abs(g_bufferR[i]) > maxR) maxR = std::abs(g_bufferR[i]);
            sumL += g_bufferL[i]*g_bufferL[i];
            if (i > 0 &&
                ((last > 0 && g_bufferL[i] < 0) ||
                 (last < 0 && g_bufferL[i] > 0))) zeroCross++;
            last = g_bufferL[i];
        }
        float rms = std::sqrt(sumL/BUFFER_SIZE);
        float db = 20 * std::log10(rms + 0.0001f);
        int y = 20;
        auto line = [&](QString l, QString v)
        {
            p.drawText(10, y, l);
            p.drawText(120, y, v);
            y += 20;
        };
        line("STATUS:", "ONLINE");
        line("RMS (L):", QString::number(rms, 'f', 4));
        line("PEAK (L):", QString::number(maxL, 'f', 4));
        line("PEAK (R):", QString::number(maxR, 'f', 4));
        line("dB LEVEL:", QString::number(db, 'f', 1) + " dB");
        line("Z-CROSS:", QString::number(zeroCross));
    }
    void drawDaftGrid(QPainter &p, int w, int h)
    {
        int cols = 16;
        int rows = 10;
        int cw = w/cols;
        int ch = h/rows;
        auto bins = computeComplexFFT(g_bufferL);
        for (int x = 0; x < cols; ++x)
        {
            float mag = std::abs(bins[x]) * 100.0f;
            int lit = (int)(mag * rows);
            for (int y = 0; y < rows; ++y)
            {
                if ((rows - y) < lit)
                    p.setBrush(QColor(255, 0, 50));
                else
                    p.setBrush(QColor(50, 0, 0));
                p.setPen(Qt::black);
                p.drawEllipse(x*cw+2, y*ch+2, cw-4, ch-4);
            }
        }
    }
    void drawBinaryRain(QPainter &p, int w, int h)
    {
        p.setPen(Qt::green);
        QFont f = p.font();
        f.setFamily("Monospace");
        f.setBold(true);
        p.setFont(f);
        int cols = w / 15;
        for (int i = 0; i < cols; ++i)
        {
            int idx = (i * BUFFER_SIZE) / cols;
            if (std::abs(g_bufferL[idx]) > 0.05f)
            {
                QString bin = (randomRange(0, 1) == 0) ? "0" : "1";
                int y = (m_hue * 5 + i * 20) % h;
                p.drawText(i * 15, y, bin);
            }
        }
    }
    void drawGlitch(QPainter &p, int w, int h)
    {
        drawWaveform(p, w, h);
        for (int i = 0; i < 10; ++i)
        {
            if (randomRange(0, 9) > 7)
            {
                int y = randomRange(0, h - 1);
                int shift = randomRange(-20, 19);
                p.setPen(Qt::red);
                p.drawLine(0, y, w, y+shift);
            }
        }
    }
    void drawRetroSun(QPainter &p, int w, int h)
    {
        p.setPen(QColor(255, 0, 255));
        int horizon = h/2 + 20;
        p.drawLine(0, horizon, w, horizon);
        for (int i = 0; i < w; i += 40)
            p.drawLine(w/2, horizon, i - (w/2 - i)*2, h);
        for (int i = horizon; i < h; i += 20)
            p.drawLine(0, i, w, i);
        float kick = getReactionLevel() * 100.0f;
        int r = 50 + (int)kick;
        QRadialGradient grad(w/2, horizon-40, r);
        grad.setColorAt(0, Qt::yellow);
        grad.setColorAt(1, QColor(255, 0, 100));
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(w/2, horizon-40), r, r);
    }
    void drawTunnel(QPainter &p, int w, int h)
    {
        float kick = getReactionLevel() * 50.0f;
        p.setPen(Qt::white);
        p.setBrush(Qt::NoBrush);
        int cx = w/2; int cy = h/2;
        for (int i = 1; i < 10; ++i)
        {
            int r = (m_hue * 2 + i * 40) % 300;
            r += kick;
            p.drawEllipse(cx - r, cy - r, r*2, r*2);
        }
    }
    void drawParticles(QPainter &p, int w, int h)
    {
        for (auto &pt : m_particles)
        {
            int x = (int)((pt.x / width()) * w);
            int y = (int)((pt.y / height()) * h);
            p.setPen(pt.color);
            p.drawPoint(x, y);
        }
    }
    void drawWaveform2(QPainter &p, int w, int h)
    {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 255, 255, 100));
        QPolygon poly;
        poly << QPoint(0, h/2);
        for (int i = 0; i < BUFFER_SIZE; ++i)
            poly << QPoint((i*w)/BUFFER_SIZE,
                           h/2 - std::abs(g_bufferL[i])*h/2);
        poly << QPoint(w, h/2);
        for (int i = BUFFER_SIZE-1; i >= 0; --i)
            poly << QPoint((i*w)/BUFFER_SIZE,
                           h/2 + std::abs(g_bufferL[i])*h/2);
        p.drawPolygon(poly);
    }
    void drawScanline(QPainter &p, int w, int h)
    {
        drawWaveform(p, w, h);
        p.setPen(QColor(0, 0, 0, 100));
        for (int y = 0; y < h; y += 4) p.drawLine(0, y, w, y);
        int roll = (m_hue * 5) % h;
        p.setBrush(QColor(255, 255, 255, 50));
        p.setPen(Qt::NoPen);
        p.drawRect(0, roll, w, 30);
    }
    void drawDNA(QPainter &p, int w, int h)
    {
        for (int i = 0; i < w; i += 5)
        {
            float phase = (i * 0.05f) + (m_hue * 0.1f);
            int y1 = h/2 + (int)(std::sin(phase) * 50);
            int y2 = h/2 + (int)(std::cos(phase) * 50);
            int t = 2 + (int)(std::abs(g_bufferL[(i*BUFFER_SIZE)/w]) * 10);
            p.setPen(QPen(Qt::cyan,    t)); p.drawPoint(i, y1);
            p.setPen(QPen(Qt::magenta, t)); p.drawPoint(i, y2);
            if (i % 20 == 0)
            {
                p.setPen(Qt::gray);
                p.drawLine(i, y1, i, y2);
            }
        }
    }
    void drawNumbers(QPainter &p, int w, int h)
    {
        p.setPen(Qt::green);
        QFont f = p.font();
        f.setFamily("Courier");
        f.setPointSize(10);
        p.setFont(f);
        int cols = 20;
        int rows = 15;
        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {
                if (randomRange(0, 99) > 90)
                    p.drawText(x*(w/cols), y*(h/rows),
                               QString::number(randomRange(0, 8)));
            }
        }
    }
    void drawShadedCube(QPainter &p, int w, int h)
    {
        Point3D verts[8] = {
            {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
            {-1,-1,1},  {1,-1,1},  {1,1,1},  {-1,1,1}
        };
        int faces[6][4] = {
            {0,1,2,3}, {4,5,6,7}, {0,1,5,4},
            {2,3,7,6}, {0,3,7,4}, {1,2,6,5}
        };
        float angleX = m_hue*0.02f;
        float angleY = m_hue*0.03f;
        float kick   = 1.0f + getReactionLevel();
        int cx = w/2; int cy = h/2;
        QPoint p2d[8];
        for (int i = 0; i < 8; ++i)
        {
            float x = verts[i].x*kick;
            float y = verts[i].y*kick;
            float z = verts[i].z*kick;
            float tx = x*std::cos(angleY) - z*std::sin(angleY);
            float tz = x*std::sin(angleY) + z*std::cos(angleY);
            x = tx; z = tz;
            float ty = y*std::cos(angleX) - z*std::sin(angleX);
            z = y*std::sin(angleX) + z*std::cos(angleX);
            y = ty;
            float dist = 4.0f + z;
            if (dist < 0.1f) dist = 0.1f;
            float scale = 300.0f / dist;
            p2d[i] = QPoint(cx + (int)(x*scale),
                            cy + (int)(y*scale));
        }
        for (int i = 0; i < 6; ++i)
        {
            QPolygon poly;
            for (int j = 0; j < 4; ++j)
                poly << p2d[faces[i][j]];
            int hue = (m_hue + i*40) % 360;
            p.setBrush(QColor::fromHsv(hue, 200, 200, 150));
            p.setPen(Qt::white);
            p.drawPolygon(poly);
        }
    }
    void drawCopperBars(QPainter &p, int w, int h)
    {
        for (int i = 0; i < 10; ++i)
        {
            int y = (m_hue * 3 + i * 50) % h;
            for (int j = 0; j < 30; ++j)
            {
                int col = 255 - std::abs(j-15)*15;
                p.setPen(QColor(col, 0, 0));
                p.drawLine(0, y+j, w, y+j);
            }
        }
        drawWaveform(p, w, h);
    }
    void drawPlasma(QPainter &p, int w, int h)
    {
        int cell = 10;
        p.setPen(Qt::NoPen);
        float t = m_hue * 0.1f;
        for (int y = 0; y < h; y += cell)
        {
            for (int x = 0; x < w; x += cell)
            {
                float v =
                    std::sin(x*0.01f + t) +
                    std::sin(y*0.01f + t) +
                    std::sin((x+y)*0.01f + t) +
                    std::sin(std::sqrt((float)x*x+y*y)*0.01f + t);
                int c = (int)((v + 4.0f) * 32.0f);
                p.setBrush(QColor::fromHsv(c % 255, 255, 255));
                p.drawRect(x, y, cell, cell);
            }
        }
    }
    void drawVectorLandscape(QPainter &p, int w, int h)
    {
        p.setPen(Qt::cyan);
        int cx = w/2; int cy = h/2;
        int horizon = cy;
        p.drawLine(0, horizon, w, horizon);
        float speed = m_hue % 20;
        for (int x = -500; x < w+500; x += 100)
            p.drawLine(cx, horizon, x, h);
        for (int z = 1; z < 10; ++z)
        {
            int y = horizon + (z * z * 5) + (int)speed;
            if (y < h) p.drawLine(0, y, w, y);
        }
    }
    void drawAnalogVU(QPainter &p, int w, int h)
    {
        p.setBrush(Qt::white);
        p.setPen(Qt::black);
        QRect box(50, 50, w-100, h-100);
        p.drawRect(box);
        p.setPen(QPen(Qt::black, 2));
        p.drawArc(box.left()+20, box.top()+20,
                  box.width()-40, box.height()*2-40,
                  45*16, 90*16);
        float rms = 0;
        for (int i = 0; i < BUFFER_SIZE; ++i)
            rms += g_bufferL[i]*g_bufferL[i];
        rms = std::sqrt(rms/BUFFER_SIZE) * 5.0f;
        if (rms > 1) rms = 1;
        float angle = 135.0f - (rms * 90.0f);
        float rad   = angle * M_PI / 180.0f;
        int cx = w/2;
        int cy = box.bottom()-20;
        int len = box.height() - 40;
        p.setPen(QPen(Qt::red, 3));
        p.drawLine(cx, cy,
                   cx + (int)(len*std::cos(rad)),
                   cy - (int)(len*std::sin(rad)));
    }
    void drawCircularSpectrum(QPainter &p, int w, int h)
    {
        auto bins = computeComplexFFT(g_bufferL);
        int cx = w/2; int cy = h/2;
        p.setPen(Qt::green);
        for (size_t i = 0; i < bins.size(); ++i)
        {
            float mag   = std::abs(bins[i]) * 100.0f;
            float angle = (float)i * 2.0f * M_PI / bins.size();
            float r1 = 50.0f;
            float r2 = 50.0f + mag * 2.0f;
            p.drawLine(cx + (int)(r1*std::cos(angle)),
                       cy + (int)(r1*std::sin(angle)),
                       cx + (int)(r2*std::cos(angle)),
                       cy + (int)(r2*std::sin(angle)));
        }
    }
    void drawKaleidoscope(QPainter &p, int w, int h)
    {
        p.setPen(Qt::magenta);
        int cx = w/2; int cy = h/2;
        for (int i = 0; i < BUFFER_SIZE-1; i += 2)
        {
            float r = 50.0f + g_bufferL[i] * 50.0f;
            float a = (float)i * 2.0f * M_PI / BUFFER_SIZE;
            int x = (int)(r * std::cos(a));
            int y = (int)(r * std::sin(a));
            p.drawPoint(cx+x, cy+y);
            p.drawPoint(cx-x, cy+y);
            p.drawPoint(cx+x, cy-y);
            p.drawPoint(cx-x, cy-y);
        }
    }
    void drawHexHive(QPainter &p, int w, int h)
    {
        int r = 30;
        float hh = r * std::sqrt(3.0f);
        int cols = w / (int)(r*1.5f);
        int rows = (int)(h / hh);
        auto bins = computeComplexFFT(g_bufferL);
        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < cols; ++x)
            {
                int idx = (x + y*cols) % (int)bins.size();
                float mag = std::abs(bins[idx]) * 50.0f;
                int px = (int)(x * r * 1.5f);
                int py = (int)(y * hh + (x%2)*hh/2);
                if (mag > 0.5f) p.setBrush(Qt::yellow);
                else            p.setBrush(Qt::NoBrush);
                p.setPen(Qt::yellow);
                QPolygon poly;
                for (int i = 0; i < 6; ++i)
                {
                    float ang = i * 60 * M_PI / 180.0f;
                    poly << QPoint(px + (int)(r*std::cos(ang)),
                                   py + (int)(r*std::sin(ang)));
                }
                p.drawPolygon(poly);
            }
        }
    }
    void drawSierpinski(QPainter &p, int w, int h)
    {
        float kick = 1.0f + getReactionLevel();
        QPoint p1(w/2, 10);
        QPoint p2(10, h-10);
        QPoint p3(w-10, h-10);
        std::function<void(QPoint,QPoint,QPoint,int)> drawTri =
            [&](QPoint a, QPoint b, QPoint c, int d)
        {
            if (d == 0)
            {
                QPolygon poly;
                poly << a << b << c;
                p.setBrush(QColor(0, (int)(255*kick/2), 0));
                p.drawPolygon(poly);
            }
            else
            {
                QPoint ab((a.x()+b.x())/2, (a.y()+b.y())/2);
                QPoint bc((b.x()+c.x())/2, (b.y()+c.y())/2);
                QPoint ca((c.x()+a.x())/2, (c.y()+a.y())/2);
                drawTri(a,  ab, ca, d-1);
                drawTri(ab, b,  bc, d-1);
                drawTri(ca, bc, c,  d-1);
            }
        };
        drawTri(p1, p2, p3, 4);
    }
    void drawLissajous3D(QPainter &p, int w, int h)
    {
        int cx = w/2; int cy = h/2;
        p.setPen(Qt::cyan);
        float t = m_hue * 0.05f;
        for (int i = 0; i < 200; ++i)
        {
            float u = i * 0.1f;
            float x = 100 * std::sin(u + t);
            float y = 100 * std::sin(2*u + t);
            float z = 100 * std::sin(3*u + t);
            float scale = 300.0f / (400.0f + z);
            p.drawPoint(cx + (int)(x*scale),
                        cy + (int)(y*scale));
        }
    }
    void drawRaindrops(QPainter &p, int w, int h)
    {
        static std::vector<QPoint> drops;
        static std::vector<int> radius;
        if (std::abs(g_bufferL[0]) > 0.5f)
        {
            drops.push_back(QPoint(randomRange(0, w - 1), randomRange(0, h - 1)));
            radius.push_back(1);
        }
        p.setPen(Qt::blue);
        p.setBrush(Qt::NoBrush);
        for (size_t i = 0; i < drops.size(); ++i)
        {
            p.drawEllipse(drops[i], radius[i], radius[i]);
            radius[i] += 2;
        }
        if (drops.size() > 20)
        {
            drops.erase(drops.begin());
            radius.erase(radius.begin());
        }
    }
    void drawScroller(QPainter &p, int w, int h)
    {
        p.setPen(Qt::white);
        QFont f = p.font();
        f.setPointSize(20);
        f.setBold(true);
        p.setFont(f);
        QString text = m_text.isEmpty()
                       ? QString::fromUtf8("YOUR TEXT")
                       : m_text;
        QFontMetrics fm(f);
        static int scrollX = 0;
        scrollX = scrollX + 4;
        if (scrollX > w + fm.horizontalAdvance(text) + 50)
            scrollX = -w;
        int startX = w - scrollX;
        int cy = h / 2;
        if (!m_wiggleText)
        {
            p.drawText(startX, cy + 10, text);
        }
        else
        {
            int currentX = startX;
            for (int i = 0; i < text.length(); ++i)
            {
                QChar c = text.at(i);
                float time = m_hue * 0.1f;
                int yOffset = (int)(30.0f *
                                    std::sin((currentX * 0.02f) + time));
                p.drawText(currentX, cy + 10 + yOffset, QString(c));
                currentX += fm.horizontalAdvance(c);
            }
        }
    }
    // ============================================================
    // NEW: WOLFENSTEIN-STYLE DUNGEON RAYCASTER (MODE 32)
    // ============================================================
    void drawWolfDungeon(QPainter &p, int w, int h)
    {
        // floor colour
        p.fillRect(0, h/2, w, h/2, QColor(10, 10, 10));
        // simple vertical-ray casting per column
        for (int x = 0; x < w; ++x)
        {
            float cameraX = 2.0f * x / float(w) - 1.0f;
            float rayDirX = m_dirX + m_planeX * cameraX;
            float rayDirY = m_dirY + m_planeY * cameraX;
            int mapX = int(m_camX);
            int mapY = int(m_camY);
            float sideDistX;
            float sideDistY;
            float deltaDistX = (rayDirX == 0) ? 1e30f : std::fabs(1.0f / rayDirX);
            float deltaDistY = (rayDirY == 0) ? 1e30f : std::fabs(1.0f / rayDirY);
            float perpWallDist;
            int stepX;
            int stepY;
            int hit = 0;
            int side = 0;
            if (rayDirX < 0) {
                stepX = -1;
                sideDistX = (m_camX - mapX) * deltaDistX;
            } else {
                stepX = 1;
                sideDistX = (mapX + 1.0f - m_camX) * deltaDistX;
            }
            if (rayDirY < 0) {
                stepY = -1;
                sideDistY = (m_camY - mapY) * deltaDistY;
            } else {
                stepY = 1;
                sideDistY = (mapY + 1.0f - m_camY) * deltaDistY;
            }
            // DDA
            while (!hit)
            {
                if (sideDistX < sideDistY) {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                } else {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }
                if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H) {
                    hit = 1;
                    break;
                }
                if (g_wolfMap[mapY][mapX] > 0) {
                    hit = 1;
                }
            }
            if (side == 0)
                perpWallDist = (sideDistX - deltaDistX);
            else
                perpWallDist = (sideDistY - deltaDistY);
            if (perpWallDist <= 0.0001f) perpWallDist = 0.0001f;
            int lineHeight = int(h / perpWallDist);
            int drawStart = -lineHeight / 2 + h / 2;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + h / 2;
            if (drawEnd >= h) drawEnd = h - 1;
            float energy = getReactionLevel();
            int baseHue = (m_hue + int(energy * 300.0f)) % 360;
            int hue = (baseHue + (side ? 30 : 0)) % 360;
            int val = 120 + int(energy * 120.0f);
            if (val > 255) val = 255;
            QColor wallColor = QColor::fromHsv(hue, 255, val);
            p.setPen(wallColor);
            p.drawLine(x, drawStart, x, drawEnd);
        }
    }
    // ============================================================
    // NEW EURO / VHS / CHAOS DRAW FUNCTIONS
    // ============================================================
    void drawVHSDamage(QPainter &p, int w, int h)
    {
        ensurePrevFrame(w, h);
        // First, render a simple base (waveform + raster) into prevFrame
        {
            QPainter q(&m_prevFrame);
            q.fillRect(0, 0, w, h, QColor(5, 5, 5));
            drawWaveform(q, w, h);
            drawLabSpectrum(q, w, h);
        }
        // Now apply VHS-style jitter + noise as we blit to screen
        QImage src = m_prevFrame;
        p.fillRect(0, 0, w, h, Qt::black);
        float jitterAmount = 4.0f + getReactionLevel() * 20.0f;
        int trackingY = (m_hue * 3) % h; // rolling bar
        for (int y = 0; y < h; ++y)
        {
            int offset = (int)(std::sin(y * 0.1f + m_hue * 0.05f) * jitterAmount);
            if (randomRange(0, 99) < 2)
                offset += randomRange(-7, 7);
            int srcY = y;
            if (srcY < 0) srcY = 0;
            if (srcY >= h) srcY = h-1;
            QRgb *line = (QRgb*)src.scanLine(srcY);
            for (int x = 0; x < w; ++x)
            {
                int sx = x + offset;
                if (sx < 0) sx = 0;
                if (sx >= w) sx = w-1;
                QRgb col = line[sx];
                // add a bit of random noise
                int noise = randomRange(-7, 7);
                int r = qBound(0, qRed(col)   + noise, 255);
                int g = qBound(0, qGreen(col) + noise, 255);
                int b = qBound(0, qBlue(col)  + noise, 255);
                p.setPen(QColor(r, g, b));
                p.drawPoint(x, y);
            }
        }
        // tracking error bar
        p.fillRect(0, trackingY, w, 4, QColor(255, 255, 255, 40));
    }
    void drawInfiniteShapeTunnel(QPainter &p, int w, int h)
    {
        int cx = w/2;
        int cy = h/2;
        float t = m_hue * 0.03f;
        float bass = getReactionLevel();
        p.fillRect(0, 0, w, h, Qt::black);
        for (int i = 0; i < 20; ++i)
        {
            float r = (i + 1) * (w / 20.0f);
            float angle = t + i * 0.2f;
            float thickness = 6.0f - i * 0.15f + bass * 4.0f;
            if (thickness < 1.0f) thickness = 1.0f;
            QColor c = QColor::fromHsv((m_hue + i*10) % 360, 255, 200);
            p.setPen(QPen(c, thickness));
            QPolygon poly;
            for (int k = 0; k < 4; ++k)
            {
                float a = angle + k * M_PI_2;
                poly << QPoint(cx + (int)(r * std::cos(a)),
                               cy + (int)(r * std::sin(a)));
            }
            p.drawPolygon(poly);
        }
    }
    void drawRecursiveSpinFractal(QPainter &p, int w, int h)
    {
        int cx = w/2;
        int cy = h/2;
        float t = m_hue * 0.05f;
        int depth = 4;
        std::function<void(float,float,float,int)> drawRect =
            [&](float x, float y, float size, int d)
        {
            if (d < 0 || size < 5.0f) return;
            float angle = t + d * 0.5f;
            QTransform tr;
            tr.translate(cx + x, cy + y);
            tr.rotate(angle * 57.2958f);
            tr.scale(size, size);
            QPolygonF poly; // Use float polygon
            poly << tr.map(QPointF(-0.5, -0.5))
                 << tr.map(QPointF( 0.5, -0.5))
                 << tr.map(QPointF( 0.5,  0.5))
                 << tr.map(QPointF(-0.5,  0.5));
            QColor c = QColor::fromHsv((m_hue + d*30) % 360, 255, 200);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(c.red(), c.green(), c.blue(), 150));
            p.drawPolygon(poly);
            float newSize = size * 0.6f;
            drawRect(x + size*0.5f, y,            newSize, d-1);
            drawRect(x - size*0.5f, y,            newSize, d-1);
            drawRect(x,             y + size*0.5f,newSize, d-1);
            drawRect(x,             y - size*0.5f,newSize, d-1);
        };
        drawRect(0, 0, std::min(w,h)/3.0f, depth);
    }
void drawEuroHyperColor(QPainter &p, int w, int h)
    {
        float t = m_hue * 0.03f;
        float bass = getReactionLevel();
        
        for (int y = 0; y < h; ++y)
        {
            float ny = (float)y / h;
            for (int x = 0; x < w; x += 4)
            {
                float nx = (float)x / w;
                
                // v ranges roughly from -3.0 to +3.0
                float v = std::sin(nx*10 + t) + std::sin(ny*10 - t*1.5f);
                v += std::sin((nx+ny)*8 + t*2.0f);
                // Calculate Hue
                int rawHue = (int)((v * 40.0f) + m_hue + bass*100.0f);
                
                // FIX: Ensure positive modulo result
                int hue = rawHue % 360;
                if (hue < 0) hue += 360; 
                int sat = 200 + (int)(bass*55.0f);
                int val = 180 + (int)(bass*70.0f);
                // qBound protects Sat and Val, but we fixed Hue manually above
                p.setPen(QColor::fromHsv(hue, qBound(0,sat,255), qBound(0,val,255)));
                p.drawLine(x, y, x+3, y);
            }
        }
    }
    void drawEuroRasterBarsXL(QPainter &p, int w, int h)
    {
        float t = m_hue * 0.05f;
        float bass = getReactionLevel();
        int numBars = 12;
        for (int i = 0; i < numBars; ++i)
        {
            float phase = t + i * 0.5f;
            int y = h/2 + (int)(std::sin(phase) * (h/3));
            int thickness = 20 + (int)(bass * 60.0f);
            QLinearGradient grad(0, y, 0, y+thickness);
            int hue = (m_hue + i*20) % 360;
            grad.setColorAt(0.0, QColor::fromHsv(hue, 255, 80));
            grad.setColorAt(0.5, QColor::fromHsv(hue, 255, 255));
            grad.setColorAt(1.0, QColor::fromHsv(hue, 255, 80));
            p.fillRect(0, y, w, thickness, grad);
        }
    }
    void drawEuroPolygonCyclotron(QPainter &p, int w, int h)
    {
        int cx = w/2;
        int cy = h/2;
        float t = m_hue * 0.04f;
        float mid = getReactionLevel();
        int layers = 14;
        for (int i = 0; i < layers; ++i)
        {
            float radius = (i+1) * (std::min(w,h) / (float)(layers+2));
            float angle  = t + i * 0.3f * (1.0f + mid*2.0f);
            QPolygon poly;
            int verts = 3 + (i % 4); // 3..6
            for (int k = 0; k < verts; ++k)
            {
                float a = angle + k * (2.0f * M_PI / verts);
                poly << QPoint(cx + (int)(radius * std::cos(a)),
                               cy + (int)(radius * std::sin(a)));
            }
            QColor c = QColor::fromHsv((m_hue + i*25) % 360, 255, 200);
            p.setPen(QPen(c, 2));
            p.setBrush(QColor(c.red(), c.green(), c.blue(), 80));
            p.drawPolygon(poly);
        }
    }
    void drawEuroHardGlitch(QPainter &p, int w, int h)
    {
        // start from waveform scene
        QImage base(w, h, QImage::Format_ARGB32);
        base.fill(Qt::black);
        {
            QPainter q(&base);
            drawWaveform(q, w, h);
        }
        p.drawImage(0, 0, base);
        float energy = getReactionLevel();
        int lines = 5 + (int)(energy * 30.0f);
        for (int i = 0; i < lines; ++i)
        {
            int y = randomRange(0, h - 1);
            int height = 1 + randomRange(0, 3);
            int shift = randomRange(-40, 39);
            p.fillRect(0, y, w, height, QColor(0,0,0,160));
            p.fillRect(shift, y, w, height, QColor(255,255,255,40));
        }
    }
    void drawEuroRotozoom(QPainter &p, int w, int h)
    {
        QImage tex(128, 128, QImage::Format_RGB32);
        for (int y = 0; y < tex.height(); ++y)
        {
            for (int x = 0; x < tex.width(); ++x)
            {
                float nx = x / 16.0f;
                float ny = y / 16.0f;
                float v = std::sin(nx) + std::sin(ny) + std::sin(nx+ny);
                
                // FIX: Ensure HSV parameters are valid
                int hue = (int)(v * 40.0f + m_hue);
                hue = hue % 360;
                if (hue < 0) hue += 360; // handle negative modulo result
                
                tex.setPixel(x, y, QColor::fromHsv(hue, 255, 200).rgb());
            }
        }
        float t = m_hue * 0.02f;
        float bass = getReactionLevel();
        float zoom = 0.5f + bass * 1.5f;
        float angle = t * 60.0f;
        QTransform tr;
        tr.translate(w/2.0, h/2.0);
        tr.rotate(angle);
        tr.scale(zoom, zoom);
        tr.translate(-tex.width()/2.0, -tex.height()/2.0);
        p.setTransform(tr);
        p.drawImage(0, 0, tex);
        p.resetTransform();
    }
    void drawEuroFFTMatrix(QPainter &p, int w, int h)
    {
        auto bins = computeComplexFFT(g_bufferL);
        int cols = 32;
        int rows = 16;
        float colW = (float)w / cols;
        float rowH = (float)h / rows;
        p.fillRect(0, 0, w, h, Qt::black);
        for (int x = 0; x < cols; ++x)
        {
            float mag = 0.0f;
            int idxStart = (x * bins.size()) / cols;
            int idxEnd = ((x+1) * bins.size()) / cols;
            for (int i = idxStart; i < idxEnd; ++i)
                mag += std::abs(bins[i]);
            mag /= (idxEnd - idxStart + 1);
            int litRows = std::min(rows, (int)(mag * 40.0f) + 1);
            int hue = (m_hue + x * 5) % 360;
            for (int y = 0; y < litRows; ++y)
            {
                QColor c = QColor::fromHsv(hue, 255, 150 + y*6);
                p.fillRect((int)(x*colW), h - (int)((y+1)*rowH),
                           (int)colW-1, (int)rowH-1, c);
            }
        }
    }
    void drawEuroTextScroller(QPainter &p, int w, int h)
    {
        // reuse scroller but add Euro-style per-letter colouring
        p.setPen(Qt::white);
        QFont f = p.font();
        f.setPointSize(18);
        f.setBold(true);
        p.setFont(f);
        QString text = m_text.isEmpty()
                       ? QString::fromUtf8("YOUR TEXT")
                       : m_text;
        QFontMetrics fm(f);
        static int scrollX2 = 0;
        scrollX2 += 6;
        if (scrollX2 > w + fm.horizontalAdvance(text) + 50)
            scrollX2 = -w;
        int startX = w - scrollX2;
        int cy = h - 40;
        int x = startX;
        for (int i = 0; i < text.length(); ++i)
        {
            QChar c = text.at(i);
            int hue = (m_hue + i*10) % 360;
            int yOffset = (int)(std::sin((x*0.03f) + m_hue*0.1f) * 15.0f);
            p.setPen(QColor::fromHsv(hue, 255, 255));
            p.drawText(x, cy + yOffset, QString(c));
            x += fm.horizontalAdvance(c);
        }
    }
    void drawEuroGalaxyTunnel(QPainter &p, int w, int h)
    {
        int cx = w/2;
        int cy = h/2;
        float t = m_hue * 0.04f;
        float bass = getReactionLevel();
        p.fillRect(0, 0, w, h, QColor(0, 0, 10));
        for (int i = 0; i < 80; ++i)
        {
            float depth = (float)i / 80.0f;
            float radius = depth * (std::min(w,h)/2.0f) * (1.0f + bass*0.5f);
            float angle = t * (1.0f + depth*3.0f) + i * 0.4f;
            QColor c = QColor::fromHsv((m_hue + i*4) % 360, 255, 180);
            p.setPen(c);
            int x = cx + (int)(radius * std::cos(angle));
            int y = cy + (int)(radius * std::sin(angle));
            p.drawPoint(x, y);
        }
    }
    // Mode 44: Analog Video Feedback
    void drawAnalogVideoFeedback(QPainter &p, int w, int h)
    {
        ensurePrevFrame(w, h);
        // 1. Draw new frame into a temporary image
        QImage temp(w, h, QImage::Format_ARGB32);
        temp.fill(Qt::black);
        {
            QPainter t(&temp);
            // Draw a high-contrast oscilloscope line
            t.setPen(QPen(QColor(0, 255, 200), 2));
            int mid = h/2;
            for (int i = 0; i < BUFFER_SIZE-1; i+=2)
            {
                int x1 = (i*w)/BUFFER_SIZE;
                int x2 = ((i+1)*w)/BUFFER_SIZE;
                int y1 = mid + (int)(g_bufferL[i] * h * 0.4f);
                int y2 = mid + (int)(g_bufferL[i+1] * h * 0.4f);
                t.drawLine(x1, y1, x2, y2);
            }
        }
        // 2. Mix previous frame (feedback) + new frame into m_prevFrame
        {
            QPainter fb(&m_prevFrame);
            // Fade out old frame (decay)
            fb.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            fb.fillRect(0, 0, w, h, QColor(0, 0, 0, 10)); // Slow fade
            
            // Zoom/Rotate feedback (Video Synth effect)
            fb.setCompositionMode(QPainter::CompositionMode_SourceOver);
            // (Simpler: just composite the new frame on top)
            fb.drawImage(0, 0, temp);
        }
        // 3. Draw result to screen
        p.drawImage(0, 0, m_prevFrame);
    }
    // Mode 45: Retro Checkerboard
    void drawRetroCheckerboard(QPainter &p, int w, int h)
    {
        // Draw a classic "boing ball" style background
        int checkSize = 40;
        float t = m_hue * 0.05f;
        
        for (int y = 0; y < h; y += checkSize)
        {
            for (int x = 0; x < w; x += checkSize)
            {
                int cx = x / checkSize;
                int cy = y / checkSize;
                bool isWhite = ((cx + cy) % 2 == 0);
                
                // Color cycle the "white" squares
                if (isWhite)
                {
                    QColor c = QColor::fromHsv((m_hue + cx*10) % 360, 200, 200);
                    p.fillRect(x, y, checkSize, checkSize, c);
                }
                else
                {
                    p.fillRect(x, y, checkSize, checkSize, QColor(50, 50, 50));
                }
            }
        }
        // Draw a primitive 3D wireframe triangle (Amiga demo style)
        QPoint center(w/2, h/2);
        float size = 100.0f + getReactionLevel() * 50.0f;
        QPolygon poly;
        for (int i = 0; i < 3; ++i)
        {
            float a = t + i * (2.0f * M_PI / 3.0f);
            poly << QPoint(center.x() + size * std::cos(a),
                           center.y() + size * std::sin(a));
        }
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(poly);
    }
    // Mode 46: Industrial Scope
    void drawIndustrialScope(QPainter &p, int w, int h)
    {
        p.fillRect(0, 0, w, h, Qt::white); // High contrast background
        
        // Jittery black waveform
        p.setPen(QPen(Qt::black, 3));
        int mid = h/2;
        float jitter = (randomRange(-5, 4));
        
        for (int i = 0; i < BUFFER_SIZE-1; i += 2)
        {
            int x1 = (i*w)/BUFFER_SIZE;
            int x2 = ((i+1)*w)/BUFFER_SIZE;
            // Add noise to the wave
            float s1 = g_bufferL[i] + (randomRange(0, 99) * 0.002f);
            float s2 = g_bufferL[i+1] + (randomRange(0, 99) * 0.002f);
            
            p.drawLine(x1, mid + s1*h*0.6f + jitter, 
                       x2, mid + s2*h*0.6f + jitter);
        }
        // "TV Static" overlay
        for (int i = 0; i < 100; ++i)
        {
            p.setPen(Qt::black);
            p.drawPoint(randomRange(0, w - 1), randomRange(0, h - 1));
        }
    }
    // ============================================================
    // MEMBERS
    // ============================================================
    int m_mode;
    bool m_showText;
    bool m_wiggleText;
    int m_hue;
    int m_randomModeIndex;
    int m_cycleCounter;
    bool m_isRecording;
    int  m_recResolution;
    int  m_recFrameCount;
    int  m_reactionBand;
    QString m_text;
    std::vector<Star> m_stars;
    int m_fire[40][25];
    std::vector<Particle> m_particles;
    // Wolf dungeon state
    float m_camX, m_camY;
    float m_dirX, m_dirY;
    float m_planeX, m_planeY;
    // --- new state for Euro/VHS/voxel/FFT cycle ---
    QImage m_prevFrame;
    bool   m_prevFrameValid = false;
    float  m_voxelZ = 0.0f;          // camera "forward" position for voxel flight
    int    m_fftCycleScene = 0;      // current scene for FFT-cycle mode
    float  m_lastEnergy    = 0.0f;
    int    m_framesSinceChange = 0;

    // --- NEW: Random Number Generator ---
    std::mt19937 m_rng;
};
// ================================================================
// EFFECT CONTROLS
// ================================================================
void VisualiserEffectControls::saveSettings(QDomDocument &doc, QDomElement &parent)
{
    QDomElement e = doc.createElement(nodeName());
    e.setAttribute("mode",       m_mode);
    e.setAttribute("showText",   m_showText ? 1 : 0);
    e.setAttribute("wiggleText", m_wiggleText ? 1 : 0);
    e.setAttribute("resolution", m_recResolution);
    e.setAttribute("band",       m_reactionBand);
    e.setAttribute("text",       m_text);
    parent.appendChild(e);
}
void VisualiserEffectControls::loadSettings(const QDomElement &parent)
{
    QDomElement e = parent.firstChildElement(nodeName());
    if (!e.isNull())
    {
        // Changed default from "0" to "47"
        m_mode         = e.attribute("mode",       "47").toInt();
        m_showText     = e.attribute("showText",   "0").toInt() != 0; // Default OFF
        m_wiggleText   = e.attribute("wiggleText", "0").toInt() != 0;
        m_recResolution= e.attribute("resolution", "0").toInt();
        m_reactionBand = e.attribute("band",       "0").toInt();
        m_text         = e.attribute("text", QString::fromUtf8("YOUR TEXT"));
    }
}
lmms::gui::EffectControlDialog *VisualiserEffectControls::createView()
{
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
    m_modeCombo->addItem("32. Wolf Dungeon (Raycaster)");
    
    // NEW ITEMS (RENUMBERED)
    m_modeCombo->addItem("33. VHS Damage");
    m_modeCombo->addItem("34. Infinite Shape Tunnel");
    m_modeCombo->addItem("35. Recursive Spin Fractal");
    m_modeCombo->addItem("36. Euro HyperColor Cycler");
    m_modeCombo->addItem("37. Euro Raster Bars XL");
    m_modeCombo->addItem("38. Euro Polygon Cyclotron");
    m_modeCombo->addItem("39. Euro Hard Glitch++");
    m_modeCombo->addItem("40. Euro Rotozoom Texture");
    m_modeCombo->addItem("41. Euro FFT Matrix");
    m_modeCombo->addItem("42. Euro Text Scroller");
    m_modeCombo->addItem("43. Euro Electro Galaxy Tunnel");
    
    // SEVERED HEADS TRIBUTE MODES (RENAMED)
    m_modeCombo->addItem("44. Analog Video Feedback");
    m_modeCombo->addItem("45. Retro Checkerboard");
    m_modeCombo->addItem("46. Industrial Scope");
    
    m_modeCombo->addItem("47. Cycle   FFT Energy");
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
    QHBoxLayout *rl = new QHBoxLayout;
    m_recCheck = new QCheckBox("RECORD", this);
    m_resCombo = new QComboBox(this);
    m_resCombo->addItem("Window");
    m_resCombo->addItem("720p");
    m_resCombo->addItem("1080p");
    m_resCombo->addItem("4K");
    m_bandCombo = new QComboBox(this);
    m_bandCombo->addItem("All");
    m_bandCombo->addItem("Bass");
    m_bandCombo->addItem("Mids");
    m_bandCombo->addItem("Treble");
    rl->addWidget(new QLabel("Export:", this));
    rl->addWidget(m_recCheck);
    rl->addWidget(new QLabel("Res:", this));
    rl->addWidget(m_resCombo);
    rl->addWidget(new QLabel("React:", this));
    rl->addWidget(m_bandCombo);
    main->addLayout(rl);
    if (m_controls)
    {
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
    connect(m_modeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onModeChanged(int)));
    connect(m_textCheck, SIGNAL(toggled(bool)),
            this, SLOT(onShowTextChanged(bool)));
    connect(m_wiggleCheck, SIGNAL(toggled(bool)),
            this, SLOT(onWiggleTextChanged(bool)));
    connect(m_recCheck, SIGNAL(toggled(bool)),
            this, SLOT(onRecToggled(bool)));
    connect(m_resCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onResChanged(int)));
    connect(m_bandCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onBandChanged(int)));
    connect(m_textBox, SIGNAL(textChanged(QString)),
            this, SLOT(onTextChanged(QString)));
}
void VisualiserControlDialog::onModeChanged(int index)
{
    if (m_controls) m_controls->setMode(index);
    if (m_screen)   m_screen->setMode(index);
}
void VisualiserControlDialog::onShowTextChanged(bool checked)
{
    if (m_controls) m_controls->setShowText(checked);
    if (m_screen)   m_screen->setShowText(checked);
}
void VisualiserControlDialog::onWiggleTextChanged(bool checked)
{
    if (m_controls) m_controls->setWiggleText(checked);
    if (m_screen)   m_screen->setWiggleText(checked);
}
void VisualiserControlDialog::onRecToggled(bool checked)
{
    if (m_screen) m_screen->setRecState(checked);
}
void VisualiserControlDialog::onResChanged(int index)
{
    if (m_controls) m_controls->setRecResolution(index);
    if (m_screen)   m_screen->setRecResolution(index);
}
void VisualiserControlDialog::onBandChanged(int index)
{
    if (m_controls) m_controls->setReactionBand(index);
    if (m_screen)   m_screen->setReactionBand(index);
}
void VisualiserControlDialog::onTextChanged(const QString &text)
{
    if (m_controls) m_controls->setText(text);
    if (m_screen)   m_screen->setText(text);
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
        for (int i = 0; i < keep; ++i)
        {
            g_bufferL[i] = g_bufferL[i + n];
            g_bufferR[i] = g_bufferR[i + n];
        }
        for (int i = 0; i < n; ++i)
        {
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
} // namespace lmms
