#include "TransferFunction.h"

#include "embed.h"

#include "plugin_export.h"

#include <vector>

#include <complex>

#include <cmath>

#include <string>

#include <cctype>



namespace lmms

{



// 1. DEFINE COMPLEX (Missing in your original file)

using Complex = std::complex<float>;

const float PI = 3.14159265358979f;



// --- FFT IMPLEMENTATION ---

void distinct_fft(std::vector<Complex>& x, bool inverse) {

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

        

        // Parentheses

        if (c == '(') {

            get(); 

            Complex val = parseExpression();

            if (peek() == ')') get(); 

            return val;

        }

        

        // Numbers

        if (std::isdigit(c) || c == '.') {

            size_t start = pos;

            while (pos < str.length() && (std::isdigit(str[pos]) || str[pos] == '.')) pos++;

            float val = std::stof(str.substr(start, pos - start));

            // Support "5j" syntax

            if (peek() == 'j') { get(); return Complex(0.0f, val); }

            return Complex(val, 0.0f);

        }

        

        // Variables and Functions

        if (std::isalpha(c)) {

            std::string token;

            while (pos < str.length() && std::isalpha(str[pos])) token += str[pos++];

            

            // VARIABLES (Added s and w for better Transfer Functions)

            if (token == "freq") return Complex(currentFreq, 0.0f);

            if (token == "w")    return Complex(2.0f * PI * currentFreq, 0.0f); // Omega

            if (token == "s")    return Complex(0.0f, 2.0f * PI * currentFreq); // s = j*w

            if (token == "j")    return Complex(0.0f, 1.0f);

            if (token == "pi")   return Complex(PI, 0.0f);



            // FUNCTIONS

            if (peek() == '(') {

                get(); Complex arg = parseExpression(); if (peek() == ')') get();

                if (token == "sqrt") return std::sqrt(arg);

                if (token == "exp")  return std::exp(arg);

                if (token == "sin")  return std::sin(arg);

                if (token == "cos")  return std::cos(arg);

                if (token == "tan")  return std::tan(arg); // Added tan

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

            else if (op == '/') { 

                get(); 

                Complex rhs = parsePower(); 

                if(std::abs(rhs) < 1e-9f) rhs = Complex(1e-9f, 0.0f); 

                lhs /= rhs; 

            } 

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

      m_ampControls(this) {}



// --- Main STFT Processing ---

Effect::ProcessStatus TransferFunctionEffect::processImpl(SampleFrame* buf, const fpp_t frames) {

    

    // 1. SETUP BUFFERS (STFT Architecture)

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



    // 2. FILL INPUT BUFFER

    std::vector<Complex> buffer(N, Complex(0.0f, 0.0f));

    for (int i = 0; i < N; ++i) {

        float val = (i < halfN) ? m_history[i] : (i - halfN < hop ? buf[i - halfN][0] : 0.0f);

        buffer[i] = Complex(val * m_window[i], 0.0f);

    }



    // 3. FORWARD FFT

    distinct_fft(buffer, false);



    // 4. PREPARE CONTROL VALUES

    int presetIdx = (int)m_ampControls.m_volumeModel.value();

    if (presetIdx < 1) presetIdx = 1;

    if (presetIdx > 17) presetIdx = 17;

    

    std::string customFormula = m_ampControls.getFormula().toStdString();

    float fs = 44100.0f; // Static sample rate

    float binWidth = fs / float(N);



    // Declare Parser (static to avoid reallocation)

    static SimpleParser parser;



    // 5. FREQUENCY DOMAIN PROCESSING LOOP

    for (int i = 0; i <= N/2; ++i) {

        float freq = i * binWidth;

        if (freq < 0.1f) freq = 0.1f;



        Complex H(1.0f, 0.0f);

        float fc, bw, delay; // Temp vars



        if (presetIdx == 1) {

            // MODE 1: Custom Text Parser

            H = parser.eval(customFormula, freq);

        }

        else {

            // MODE 2-17: Hardcoded Presets

            switch(presetIdx) {

                case 2: // Lowpass

                    H = 1.0f / std::sqrt(1.0f + std::pow(freq / 800.0f, 2.0f)); break;

                case 3: // Highpass

                    H = std::sqrt(1.0f + std::pow(freq / 500.0f, 2.0f)); break;

                case 4: // Low Shelf

                    H = (freq / 500.0f) / std::sqrt(1.0f + std::pow(freq / 500.0f, 2.0f)); break;

                case 5: // High Shelf

                    H = 1.0f + (1.0f) / (1.0f + std::pow(2000.0f / (freq+1.0f), 2.0f)); break;

                case 6: // Telephone

                    if (freq > 300.0f && freq < 3000.0f) H = Complex(1.0f, 0.0f);

                    else H = Complex(0.0f, 0.0f);

                    break;

                case 7: // Notch Hole

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

                case 13: // Spectral Decay

                    H = std::exp(-0.0004f * freq); break;

                case 14: // Spectral Gate Sweep

                    if (freq > 500.0f) H = std::exp(-0.0008f * (freq - 500.0f)) * std::sin(freq * 0.005f);

                    else H = 0.0f; 

                    break;

                case 15: // Formants

                    H = std::exp(-std::pow(freq-800.0f, 2.0f)/(2.0f*6400.0f)) +

                        std::exp(-std::pow(freq-1500.0f, 2.0f)/(2.0f*22500.0f)) +

                        std::exp(-std::pow(freq-2500.0f, 2.0f)/(2.0f*40000.0f)); break;

                case 16: // Odd Harmonic Booster

                    H = 1.0f + 0.8f * std::sin(PI * freq / 200.0f); break;

                case 17: // Phase Tilt

                    H = std::exp(Complex(0, 1) * 0.0005f * freq); break;

                default: 

                    H = Complex(1.0f, 0.0f); break;

            }

        }



        // Apply Transfer Function

        if (i == 0 || i == N/2) H = Complex(H.real(), 0.0f);

        buffer[i] *= H;



        // Conjugate Symmetry (Mirror for Real Audio)

        if (i > 0 && i < N/2) buffer[N-i] = std::conj(buffer[i]);

    }



    // 6. INVERSE FFT

    distinct_fft(buffer, true);



    // 7. OVERLAP-ADD OUTPUT

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
