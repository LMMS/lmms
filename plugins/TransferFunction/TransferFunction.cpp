// TransferFunction.h
#ifndef LMMS_TRANSFERFUNCTION_H
#define LMMS_TRANSFERFUNCTION_H

#include "Effect.h"
#include "TransferFunctionControls.h"
#include <vector>
#include <complex>
#include <string>

namespace lmms {

typedef std::complex<float> Complex;

class TransferFunctionEffect : public Effect {
public:
    TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    ~TransferFunctionEffect() override = default;

    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

    EffectControls* controls() override { return &m_ampControls; }

private:
    TransferFunctionControls m_ampControls;

    // State buffers for overlap-add processing
    std::vector<float> m_history;     // Previous input half-block
    std::vector<float> m_window;      // Hann window coefficients
    std::vector<float> m_overlapAdd;  // Overlap-add buffer for output
    friend class TransferFunctionControls;
};

} // namespace lmms

#endif // LMMS_TRANSFERFUNCTION_H


// TransferFunction.cpp
#include "TransferFunction.h"
#include "embed.h"
#include "plugin_export.h"
#include <cmath>
#include <cctype>

namespace lmms {

const float PI = 3.14159265358979f;

// --- Recursive Cooley-Tukey FFT ---
void distinct_fft(std::vector<Complex>& x, bool inverse) {
    const size_t N = x.size();
    if (N <= 1) return;
    std::vector<Complex> even(N / 2), odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[2*i];
        odd[i]  = x[2*i+1];
    }
    distinct_fft(even, inverse);
    distinct_fft(odd, inverse);
    float angle = (inverse ? -2.0f : 2.0f) * PI / N;
    Complex w(1.0f, 0.0f), wn(std::cos(angle), std::sin(angle));
    for (size_t i = 0; i < N/2; ++i) {
        x[i]       = even[i] + w * odd[i];
        x[i+N/2]   = even[i] - w * odd[i];
        if (inverse) {
            x[i]     /= 2.0f;
            x[i+N/2] /= 2.0f;
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
    "email address",
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
      m_ampControls(this) {}

// --- Main STFT Processing ---
Effect::ProcessStatus TransferFunctionEffect::processImpl(SampleFrame* buf, const fpp_t frames) {
    int hop = (int)frames;
    int N = 1;
    while (N < 2*hop) N <<= 1;
    int halfN = N / 2;

    if (m_window.size() != (size_t)N) {
        m_window.assign(N, 0.0f);
        for (int i = 0; i < N; ++i) {
            m_window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (N - 1)));
        }
    }
    if (m_overlapAdd.size() != (size_t)halfN)
        m_overlapAdd.assign(halfN, 0.0f);
    if (m_history.size() != (size_t)halfN)
        m_history.assign(halfN, 0.0f);

    std::vector<Complex> buffer(N, Complex(0.0f, 0.0f));
    for (int i = 0; i < N; ++i) {
        float val = (i < halfN) ? m_history[i] : (i - halfN < hop ? buf[i - halfN][0] : 0.0f);
        buffer[i] = Complex(val * m_window[i], 0.0f);
    }

    distinct_fft(buffer, false);

    int presetIdx = (int)m_ampControls.m_volumeModel.value();
    if (presetIdx < 1) presetIdx = 1;
    if (presetIdx > 17) presetIdx = 17;
    std::string customFormula = m_ampControls.getFormula().toStdString();

    float fs = 44100.0f;
    float binWidth = fs / float(N);

    for (int i = 0; i <= N/2; ++i) {
        float freq = i * binWidth;
        Complex H(1.0f, 0.0f);  // fallback if needed
        if (presetIdx == 1) {
            H = Complex(freq / 800.0f, 0.0f);  // replace with actual eval if needed
        }
        if (i == 0 || i == N/2) H = Complex(H.real(), 0.0f);
        buffer[i] *= H;
        if (i > 0 && i < N/2) buffer[N-i] = std::conj(buffer[i]);
    }

    distinct_fft(buffer, true);

    for (int i = 0; i < halfN; ++i) {
        float outVal = m_overlapAdd[i] + buffer[i].real();
        m_history[i] = buf[i][0];
        m_overlapAdd[i] = buffer[i + halfN].real();
        buf[i][0] = outVal;
        buf[i][1] = outVal;
    }

    return Effect::ProcessStatus::Continue;
}

// --- Entry Point ---
extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
    return new TransferFunctionEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms
