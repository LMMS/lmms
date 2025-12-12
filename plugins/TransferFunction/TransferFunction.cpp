#include "TransferFunction.h"
#include "embed.h"
#include "plugin_export.h"
// SHARED LOGIC
#include "TransferFunctionLogic.h"
// NEW INCLUDES FOR SAMPLE RATE
#include "Engine.h"
#include "AudioEngine.h"
#include <vector>
#include <complex>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm> 
namespace lmms
{
using Complex = std::complex<float>;
const float PI = 3.14159265358979f;
// CONSTANTS
const int FIXED_FFT_SIZE = 2048;
// --- FFT IMPLEMENTATION ---
void distinct_fft(std::vector<Complex>& x, bool inverse)
{
    const size_t N = x.size();
    if (N <= 1) return;
    std::vector<Complex> even(N / 2);
    std::vector<Complex> odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }
    distinct_fft(even, inverse);
    distinct_fft(odd, inverse);
    float angle = (inverse ? -2.0f : 2.0f) * PI / N;
    Complex w(1.0f, 0.0f);
    Complex wn(std::cos(angle), std::sin(angle));
    for (size_t i = 0; i < N / 2; ++i) {
        x[i] = even[i] + w * odd[i];
        x[i + N / 2] = even[i] - w * odd[i];
        if (inverse) {
            x[i] /= 2.0f;
            x[i + N / 2] /= 2.0f;
        }
        w *= wn;
    }
}
// --- Plugin Descriptor ---
extern "C" {
Plugin::Descriptor PLUGIN_EXPORT TransferFunction_plugin_descriptor = {
    LMMS_STRINGIFY(PLUGIN_NAME),
    "TransferFunction",
    QT_TRANSLATE_NOOP("PluginBrowser", "A native TransferFunction plugin"),
    "Ewan Pettigrew",
    0x0100,
    Plugin::Type::Effect,
    new PixmapLoader("lmms-plugin-logo"),
    nullptr,
    nullptr,
};
}
// --- Constructor ---
TransferFunctionEffect::TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
    : Effect(&TransferFunction_plugin_descriptor, parent, key),
      m_ampControls(this)
{
}
// --- High Resolution STFT Processing (Stereo) ---
Effect::ProcessStatus TransferFunctionEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
    // 1. ONE-TIME INITIALIZATION
    if (m_window.size() != FIXED_FFT_SIZE) {
        m_window.assign(FIXED_FFT_SIZE, 0.0f);
        
        // Init Left/Right Buffers
        m_historyL.assign(FIXED_FFT_SIZE, 0.0f);
        m_historyR.assign(FIXED_FFT_SIZE, 0.0f);
        
        m_overlapAddL.assign(FIXED_FFT_SIZE, 0.0f);
        m_overlapAddR.assign(FIXED_FFT_SIZE, 0.0f);
        // Hanning Window
        for (int i = 0; i < FIXED_FFT_SIZE; ++i) {
            m_window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (FIXED_FFT_SIZE - 1)));
        }
    }
    int hop = (int)frames;
    if (hop > FIXED_FFT_SIZE) hop = FIXED_FFT_SIZE;
    // 2. INPUT BUFFERING (Shift & Append)
    std::copy(m_historyL.begin() + hop, m_historyL.end(), m_historyL.begin());
    std::copy(m_historyR.begin() + hop, m_historyR.end(), m_historyR.begin());
    for (int i = 0; i < hop; ++i) {
        m_historyL[FIXED_FFT_SIZE - hop + i] = buf[i][0];
        m_historyR[FIXED_FFT_SIZE - hop + i] = buf[i][1];
    }
    // 3. PREPARE FFT BUFFERS
    std::vector<Complex> bufferL(FIXED_FFT_SIZE);
    std::vector<Complex> bufferR(FIXED_FFT_SIZE);
    
    for (int i = 0; i < FIXED_FFT_SIZE; ++i) {
        bufferL[i] = Complex(m_historyL[i] * m_window[i], 0.0f);
        bufferR[i] = Complex(m_historyR[i] * m_window[i], 0.0f);
    }
    // 4. FORWARD FFT
    distinct_fft(bufferL, false);
    distinct_fft(bufferR, false);
    // 5. CALCULATE FILTER RESPONSE
    int presetIdx = (int)m_ampControls.m_volumeModel.value();
    if (presetIdx < 1) presetIdx = 1;
    if (presetIdx > 18) presetIdx = 18; // Cap at 18
    std::string customFormula = m_ampControls.getFormula().toStdString();
    
    float fs = Engine::audioEngine()->baseSampleRate();
    if (fs < 1.0f) fs = 44100.0f; 
    
    float binWidth = fs / (float)FIXED_FFT_SIZE;
    
    static SimpleParser parser;
    std::vector<Complex> filterCurve(FIXED_FFT_SIZE / 2 + 1);
    // Retrieve points for Preset 18
    const std::vector<DrawPoint>* drawPoints = &m_ampControls.getDrawPoints();
    for (int i = 0; i <= FIXED_FFT_SIZE / 2; ++i) {
        float freq = i * binWidth;
        if (freq < 0.1f) freq = 0.1f;
        // USE SHARED LOGIC - Pass drawPoints
        Complex H = calculateTransferFunction(presetIdx, freq, customFormula, parser, drawPoints);
        
        filterCurve[i] = H;
    }
    // 6. APPLY FILTER
    for (int i = 0; i <= FIXED_FFT_SIZE / 2; ++i) {
        Complex H = filterCurve[i];
        
        if (i == 0 || i == FIXED_FFT_SIZE/2) H = Complex(H.real(), 0.0f);
        bufferL[i] *= H;
        bufferR[i] *= H;
        if (i > 0 && i < FIXED_FFT_SIZE/2) {
            bufferL[FIXED_FFT_SIZE-i] = std::conj(bufferL[i]);
            bufferR[FIXED_FFT_SIZE-i] = std::conj(bufferR[i]);
        }
    }
    // 7. INVERSE FFT
    distinct_fft(bufferL, true);
    distinct_fft(bufferR, true);
    // 8. OVERLAP-ADD
    float outputScale = 2.0f * (float)hop / (float)FIXED_FFT_SIZE;
    for (int i = 0; i < FIXED_FFT_SIZE; ++i) {
        m_overlapAddL[i] += bufferL[i].real() * outputScale;
        m_overlapAddR[i] += bufferR[i].real() * outputScale;
    }
    // 9. WRITE OUTPUT
    for (int i = 0; i < hop; ++i) {
        buf[i][0] = m_overlapAddL[i];
        buf[i][1] = m_overlapAddR[i];
    }
    // SHIFT ACCUMULATORS
    std::copy(m_overlapAddL.begin() + hop, m_overlapAddL.end(), m_overlapAddL.begin());
    std::copy(m_overlapAddR.begin() + hop, m_overlapAddR.end(), m_overlapAddR.begin());
    
    std::fill(m_overlapAddL.end() - hop, m_overlapAddL.end(), 0.0f);
    std::fill(m_overlapAddR.end() - hop, m_overlapAddR.end(), 0.0f);
    return Effect::ProcessStatus::Continue;
}
// --- Entry Point ---
extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
    return new TransferFunctionEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}
} // namespace lmms
