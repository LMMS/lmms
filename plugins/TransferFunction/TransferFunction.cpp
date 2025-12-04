#include "TransferFunction.h"

#include "embed.h"

#include "plugin_export.h"

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



// --- MATH PARSER ---

class SimpleParser {

    std::string str;

    size_t pos;

    float currentFreq;



    char peek() {

        while (pos < str.length() && std::isspace(str[pos])) pos++;

        if (pos == str.length()) return 0;

        return str[pos];

    }

    char get() {

        char c = peek();

        if (pos < str.length()) pos++;

        return c;

    }

    Complex parsePrimary() {

        char c = peek();

        if (c == '(') {

            get(); Complex val = parseExpression(); if (peek() == ')') get(); return val;

        }

        if (std::isdigit(c) || c == '.') {

            size_t start = pos;

            while (pos < str.length() && (std::isdigit(str[pos]) || str[pos] == '.')) pos++;

            float val = std::stof(str.substr(start, pos - start));

            if (peek() == 'j') { get(); return Complex(0.0f, val); }

            return Complex(val, 0.0f);

        }

        if (std::isalpha(c)) {

            std::string token;

            while (pos < str.length() && std::isalpha(str[pos])) token += str[pos++];

            if (token == "freq") return Complex(currentFreq, 0.0f);

            if (token == "w")    return Complex(2.0f * PI * currentFreq, 0.0f);

            if (token == "s")    return Complex(0.0f, 2.0f * PI * currentFreq);

            if (token == "j")    return Complex(0.0f, 1.0f);

            if (token == "pi")   return Complex(PI, 0.0f);

            if (peek() == '(') {

                get(); Complex arg = parseExpression(); if (peek() == ')') get();

                if (token == "sqrt") return std::sqrt(arg);

                if (token == "exp")  return std::exp(arg);

                if (token == "sin")  return std::sin(arg);

                if (token == "cos")  return std::cos(arg);

                if (token == "tan")  return std::tan(arg);

                if (token == "abs")  return Complex(std::abs(arg), 0.0f);

                if (token == "log")  return std::log(arg);

            }

            return Complex(1.0f, 0.0f);

        }

        return Complex(0.0f, 0.0f);

    }

    Complex parsePower() {

        Complex lhs = parsePrimary();

        while (true) {

            if (peek() == '^') { get(); Complex rhs = parsePrimary(); lhs = std::pow(lhs, rhs); }

            else break;

        }

        return lhs;

    }

    Complex parseTerm() {

        Complex lhs = parsePower();

        while (true) {

            char op = peek();

            if (op == '*') { get(); lhs *= parsePower(); }

            else if (op == '/') { get(); Complex rhs = parsePower(); if(std::abs(rhs)<1e-9f) rhs=Complex(1e-9f,0.0f); lhs/=rhs; }

            else break;

        }

        return lhs;

    }

    Complex parseExpression() {

        Complex lhs = parseTerm();

        while (true) {

            char op = peek();

            if (op == '+') { get(); lhs += parseTerm(); }

            else if (op == '-') { get(); lhs -= parseTerm(); }

            else break;

        }

        return lhs;

    }

public:

    Complex eval(const std::string& expression, float f) {

        str = expression; pos = 0; currentFreq = f;

        if (str.empty()) return Complex(1.0f, 0.0f);

        return parseExpression();

    }

};



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

    // Shift Left Channel

    std::copy(m_historyL.begin() + hop, m_historyL.end(), m_historyL.begin());

    // Shift Right Channel

    std::copy(m_historyR.begin() + hop, m_historyR.end(), m_historyR.begin());



    // Append new audio

    for (int i = 0; i < hop; ++i) {

        m_historyL[FIXED_FFT_SIZE - hop + i] = buf[i][0]; // Left

        m_historyR[FIXED_FFT_SIZE - hop + i] = buf[i][1]; // Right

    }



    // 3. PREPARE FFT BUFFERS

    std::vector<Complex> bufferL(FIXED_FFT_SIZE);

    std::vector<Complex> bufferR(FIXED_FFT_SIZE);

    

    for (int i = 0; i < FIXED_FFT_SIZE; ++i) {

        bufferL[i] = Complex(m_historyL[i] * m_window[i], 0.0f);

        bufferR[i] = Complex(m_historyR[i] * m_window[i], 0.0f);

    }



    // 4. FORWARD FFT (Both Channels)

    distinct_fft(bufferL, false);

    distinct_fft(bufferR, false);



    // 5. CALCULATE FILTER RESPONSE (Optimization: Do it once, apply to both)

    int presetIdx = (int)m_ampControls.m_volumeModel.value();

    if (presetIdx < 1) presetIdx = 1;

    if (presetIdx > 17) presetIdx = 17;



    std::string customFormula = m_ampControls.getFormula().toStdString();

    float fs = 44100.0f;

    float binWidth = fs / (float)FIXED_FFT_SIZE;

    static SimpleParser parser;



    // We store the calculated filter curve here

    std::vector<Complex> filterCurve(FIXED_FFT_SIZE / 2 + 1);



    for (int i = 0; i <= FIXED_FFT_SIZE / 2; ++i) {

        float freq = i * binWidth;

        if (freq < 0.1f) freq = 0.1f;



        Complex H(1.0f, 0.0f);

        float fc, bw, delay; 



        if (presetIdx == 1) {

            H = parser.eval(customFormula, freq);

        }

        else {

            switch(presetIdx) {

                case 2: H = 1.0f / std::sqrt(1.0f + std::pow(freq / 800.0f, 2.0f)); break;

 		case 3: // Highpass (Brick Wall - Same style as Case 6)
                    // If frequency is below 500Hz, silence it completely.
                    if (freq < 500.0f) H = Complex(0.0f, 0.0f);
                    else H = Complex(1.0f, 0.0f); 
                    break;

                case 4: H = 1.0f + 1.0f / (1.0f + std::pow(freq / 500.0f, 2.0f)); break;

                case 5: H = 1.0f + 1.0f / (1.0f + std::pow(2000.0f / (freq + 1.0f), 2.0f)); break;

                case 6: // Telephone

                    if (freq > 300.0f && freq < 3000.0f) H = Complex(1.0f, 0.0f);

                    else H = Complex(0.0f, 0.0f); break;

                case 7: // Notch

                    fc = 600.0f; bw = 40.0f;

                    H = 1.0f - std::exp(-std::pow(freq - fc, 2.0f) / (2.0f * bw * bw)); break;

                case 8: // Resonator

                    fc = 900.0f; 

                    H = 1.0f + 8.0f * std::exp(-std::pow(20.0f * (freq - fc) / fc, 2.0f)); break;

                case 9: // Comb

                    delay = 0.002f;

                    H = 1.0f + std::exp(Complex(0, -1) * 2.0f * PI * freq * delay); break;

                case 10: // Comb Notch

                    delay = 0.003f;

                    H = 1.0f - std::exp(Complex(0, -1) * 2.0f * PI * freq * delay); break;

                case 11: // Allpass

                    fc = 1200.0f;

                    H = (1.0f - Complex(0,1) * (freq/fc)) / (1.0f + Complex(0,1) * (freq/fc)); break;

                case 12: // Bitcrusher

                    H = (std::fmod(freq, 500.0f) < 250.0f) ? 1.0f : -1.0f; break;

                case 13: H = std::exp(-0.0004f * freq); break;

                case 14: // Gate Sweep

                    if (freq > 500.0f) H = std::exp(-0.0008f * (freq - 500.0f)) * std::sin(freq * 0.005f);

                    else H = 0.0f; break;

                case 15: // Formants

                    H = std::exp(-std::pow(freq-800.0f, 2.0f)/(2.0f*6400.0f)) +

                        std::exp(-std::pow(freq-1500.0f, 2.0f)/(2.0f*22500.0f)) +

                        std::exp(-std::pow(freq-2500.0f, 2.0f)/(2.0f*40000.0f)); break;

                case 16: H = 1.0f + 0.8f * std::sin(PI * freq / 200.0f); break;

                case 17: H = std::exp(Complex(0, 1) * 0.0005f * freq); break;

                default: H = Complex(1.0f, 0.0f); break;

            }

        }

        filterCurve[i] = H;

    }



    // 6. APPLY FILTER TO BOTH CHANNELS

    for (int i = 0; i <= FIXED_FFT_SIZE / 2; ++i) {

        Complex H = filterCurve[i];

        

        // Force DC/Nyquist to be Real

        if (i == 0 || i == FIXED_FFT_SIZE/2) H = Complex(H.real(), 0.0f);



        bufferL[i] *= H;

        bufferR[i] *= H;



        // Conjugate Symmetry

        if (i > 0 && i < FIXED_FFT_SIZE/2) {

            bufferL[FIXED_FFT_SIZE-i] = std::conj(bufferL[i]);

            bufferR[FIXED_FFT_SIZE-i] = std::conj(bufferR[i]);

        }

    }



    // 7. INVERSE FFT (Both)

    distinct_fft(bufferL, true);

    distinct_fft(bufferR, true);



    // 8. OVERLAP-ADD

    float outputScale = 2.0f * (float)hop / (float)FIXED_FFT_SIZE;



    for (int i = 0; i < FIXED_FFT_SIZE; ++i) {

        m_overlapAddL[i] += bufferL[i].real() * outputScale;

        m_overlapAddR[i] += bufferR[i].real() * outputScale;

    }



    // 9. WRITE OUTPUT & SHIFT

    for (int i = 0; i < hop; ++i) {

        buf[i][0] = m_overlapAddL[i];

        buf[i][1] = m_overlapAddR[i]; // Actual Right Channel Data

    }



    // Shift Accumulators

    std::copy(m_overlapAddL.begin() + hop, m_overlapAddL.end(), m_overlapAddL.begin());

    std::copy(m_overlapAddR.begin() + hop, m_overlapAddR.end(), m_overlapAddR.begin());

    

    // Clear Tail

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
