/* TransferFunction.cpp */
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

typedef std::complex<float> Complex;
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
	Complex w(1);
	Complex wn(std::cos(angle), std::sin(angle));
	for (size_t i = 0; i < N / 2; ++i) {
		x[i] = even[i] + w * odd[i];
		x[i + N / 2] = even[i] - w * odd[i];
		if (inverse) {
			x[i] /= 2;
			x[i + N / 2] /= 2;
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
            get(); 
            Complex val = parseExpression();
            if (peek() == ')') get(); 
            return val;
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
            if (token == "j") return Complex(0.0f, 1.0f);
            if (token == "pi") return Complex(PI, 0.0f);
            if (peek() == '(') {
                get(); Complex arg = parseExpression(); if (peek() == ')') get();
                if (token == "sqrt") return std::sqrt(arg);
                if (token == "exp")  return std::exp(arg);
                if (token == "sin")  return std::sin(arg);
                if (token == "cos")  return std::cos(arg);
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
            else if (op == '/') { get(); Complex rhs = parsePower(); if(std::abs(rhs)<1e-9f) rhs=Complex(1e-9f,0); lhs /= rhs; } 
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

Complex calculateTransferH(int preset, float freq, const std::string& customFormula)
{
	if (freq < 0.1f) freq = 0.1f; 

	// Preset 1: Custom String Parser
	if (preset == 1) {
		static SimpleParser parser;
		return parser.eval(customFormula, freq);
	}

	// HARDCODED PRESETS (Matching MATLAB Proof of Concept)
	float fc, bw, delay; // temp vars (removed 'gain' to fix warning)

	switch(preset) {
		case 2: // Lowpass
			return 1.0f / std::sqrt(1.0f + std::pow(freq / 800.0f, 2.0f));

		case 3: // Highpass
			return std::sqrt(1.0f + std::pow(freq / 500.0f, 2.0f));

		case 4: // Low Shelf
			return 1.0f + (1.0f) / (1.0f + std::pow(freq / 200.0f, 2.0f));

		case 5: // High Shelf
			return 1.0f + (1.0f) / (1.0f + std::pow(2000.0f / (freq+1.0f), 2.0f));

		case 6: // Telephone (Bandpass logic)
			if (freq > 300.0f && freq < 3000.0f) return Complex(1.0f, 0.0f);
			return Complex(0.0f, 0.0f);

		case 7: // Notch Hole
			fc = 600.0f; bw = 40.0f;
			return 1.0f - std::exp(-std::pow(freq - fc, 2.0f) / (2.0f * bw * bw));

		case 8: // Resonator
			fc = 900.0f; 
			return 1.0f + 8.0f * std::exp(-std::pow(20.0f * (freq - fc) / fc, 2.0f));

		case 9: // Comb
			delay = 0.002f;
			return 1.0f + std::exp(Complex(0, -1) * 2.0f * PI * freq * delay);

		case 10: // Comb Notch
			delay = 0.003f;
			return 1.0f - std::exp(Complex(0, -1) * 2.0f * PI * freq * delay);

		case 11: // Allpass
			fc = 1200.0f;
			return (1.0f - Complex(0,1) * (freq/fc)) / (1.0f + Complex(0,1) * (freq/fc));

		case 12: // Bitcrusher
			return (std::fmod(freq, 500.0f) < 250.0f) ? 1.0f : -1.0f;

		case 13: // Spectral Decay
			return std::exp(-0.0004f * freq);

		case 14: // Spectral Gate Sweep
			if (freq > 500.0f) {
				return std::exp(-0.0008f * (freq - 500.0f)) * std::sin(freq * 0.005f);
			}
			return 0.0f;

		case 15: // Formants
			return std::exp(-std::pow(freq-800.0f, 2.0f)/(2.0f*6400.0f)) +
				   std::exp(-std::pow(freq-1500.0f, 2.0f)/(2.0f*22500.0f)) +
				   std::exp(-std::pow(freq-2500.0f, 2.0f)/(2.0f*40000.0f));

		case 16: // Odd Harmonic Booster
			return 1.0f + 0.8f * std::sin(PI * freq / 200.0f);

		case 17: // Phase Tilt
			return std::exp(Complex(0, 1) * 0.0005f * freq);

		default: 
			return Complex(1.0f, 0.0f);
	}
}


extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT TransferFunction_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"TransferFunction",
	QT_TRANSLATE_NOOP("PluginBrowser", "A native TransferFunction plugin"),
	"email address",
	0x0100,
	Plugin::Type::Effect,
	new PixmapLoader("lmms-plugin-logo"),
	nullptr,
	nullptr,
} ;
}

TransferFunctionEffect::TransferFunctionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&TransferFunction_plugin_descriptor, parent, key),
	m_ampControls(this)
{
}

Effect::ProcessStatus TransferFunctionEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	// --- OVERLAP-SAVE INITIALIZATION ---
	// 1. Ensure History buffer matches current block size
	if (m_history.size() != frames) {
		m_history.assign(frames, 0.0f);
	}

	// 2. Setup FFT Size (Double the frame size for proper overlap)
	int N = 1;
	while (N < (int)(frames * 2)) N *= 2; 

	std::vector<Complex> buffer(N);

	// 3. Fill Buffer: [History] + [Current Audio] + [Padding]
	// This creates a smooth window into the past
	for( int i = 0; i < N; ++i ) {
		float val = 0.0f;
		if (i < (int)frames) {
			val = m_history[i]; // First half: Old audio
		} else if (i < (int)(frames * 2)) {
			val = buf[i - frames][0]; // Second half: New audio
		}
		buffer[i] = Complex(val, 0.0f); 
	}

	// 4. Perform FFT
	distinct_fft(buffer, false);

	// 5. Get Controls
	int presetIdx = (int)m_ampControls.m_volumeModel.value();
	if (presetIdx < 1) presetIdx = 1;
	if (presetIdx > 17) presetIdx = 17;

	std::string customFormula = m_ampControls.getFormula().toStdString();
	float fs = 44100.0f; 
	float binWidth = fs / (float)N;

	// 6. FILTER (0 to Nyquist)
	for( int i = 0; i <= N / 2; ++i ) 
	{
		float freq = i * binWidth;
		Complex H = calculateTransferH(presetIdx, freq, customFormula);

		// FIX 1: NYQUIST MUST BE REAL
		// If we are at the very last bin (Nyquist), imaginary part must be 0
		if (i == N / 2) {
			H = Complex(H.real(), 0.0f);
		}
        // FIX 2: DC MUST BE REAL (Usually is, but good to force)
        if (i == 0) {
            H = Complex(H.real(), 0.0f);
        }

		buffer[i] *= H;

		// Mirror to negative frequencies
		if (i > 0 && i < N / 2) {
			buffer[N - i] = std::conj(buffer[i]); // FORCE exact conjugate
		}
	}

	// 7. Inverse FFT
	distinct_fft(buffer, true);

	// 8. Output & Update History
	// In Overlap-Save, the "valid" linear convolution is the *second half* of the buffer.
	for( int i = 0; i < (int)frames; ++i ) {
		// Save current raw input for next time
		m_history[i] = buf[i][0]; 

		// Output the valid filtered part (from the second half of FFT buffer)
		// We offset by 'frames' because that's where the "current" valid data lies in Overlap-Save
		float outVal = buffer[i + frames].real();
		
		buf[i][0] = outVal; 
		buf[i][1] = outVal; 
	}

	return Effect::ProcessStatus::Continue;
}

extern "C"
{
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new TransferFunctionEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}

} // namespace lmms
