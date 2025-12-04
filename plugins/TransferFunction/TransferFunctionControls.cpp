/*

 * TransferFunctionControls.cpp - controls for TransferFunction effect

 */



#include "TransferFunctionControls.h"

#include "TransferFunction.h"



// For XML save/load

#include <QDomElement>

#include <QDomDocument>



// For Bode plot math

#include <vector>

#include <cmath>

#include <string>

#include <cctype>



namespace lmms

{



// We already have typedef std::complex<float> Complex in TransferFunction.h



// Local PI constant for this file

static const float PI = 3.14159265358979f;



// A local copy of SimpleParser so we can evaluate H(freq) here as well.

class SimpleParser

{

	std::string str;

	size_t pos = 0;

	float currentFreq = 0.0f;



	char peek()

	{

		while (pos < str.length() && std::isspace(static_cast<unsigned char>(str[pos])))

		{

			++pos;

		}

		if (pos >= str.length())

		{

			return 0;

		}

		return str[pos];

	}



	char get()

	{

		char c = peek();

		if (pos < str.length())

		{

			++pos;

		}

		return c;

	}



	Complex parsePrimary()

	{

		char c = peek();



		// Parentheses

		if (c == '(')

		{

			get();

			Complex val = parseExpression();

			if (peek() == ')')

			{

				get();

			}

			return val;

		}



		// Numbers

		if (std::isdigit(static_cast<unsigned char>(c)) || c == '.')

		{

			size_t start = pos;

			while (pos < str.length() &&

			       (std::isdigit(static_cast<unsigned char>(str[pos])) || str[pos] == '.'))

			{

				++pos;

			}

			float val = std::stof(str.substr(start, pos - start));

			// "5j" syntax

			if (peek() == 'j')

			{

				get();

				return Complex(0.0f, val);

			}

			return Complex(val, 0.0f);

		}



		// Variables / functions

		if (std::isalpha(static_cast<unsigned char>(c)))

		{

			std::string token;

			while (pos < str.length() && std::isalpha(static_cast<unsigned char>(str[pos])))

			{

				token += str[pos++];

			}



			// VARIABLES

			if (token == "freq") return Complex(currentFreq, 0.0f);

			if (token == "w")    return Complex(2.0f * PI * currentFreq, 0.0f);       // omega

			if (token == "s")    return Complex(0.0f, 2.0f * PI * currentFreq);       // s = j*w

			if (token == "j")    return Complex(0.0f, 1.0f);

			if (token == "pi")   return Complex(PI, 0.0f);



			// FUNCTIONS

			if (peek() == '(')

			{

				get();

				Complex arg = parseExpression();

				if (peek() == ')')

				{

					get();

				}



				if (token == "sqrt") return std::sqrt(arg);

				if (token == "exp")  return std::exp(arg);

				if (token == "sin")  return std::sin(arg);

				if (token == "cos")  return std::cos(arg);

				if (token == "tan")  return std::tan(arg);

				if (token == "abs")  return Complex(std::abs(arg), 0.0f);

				if (token == "log")  return std::log(arg);

			}



			// Unknown token → neutral

			return Complex(1.0f, 0.0f);

		}



		return Complex(0.0f, 0.0f);

	}



	Complex parsePower()

	{

		Complex lhs = parsePrimary();

		for (;;)

		{

			if (peek() == '^')

			{

				get();

				Complex rhs = parsePrimary();

				lhs = std::pow(lhs, rhs);

			}

			else

			{

				break;

			}

		}

		return lhs;

	}



	Complex parseTerm()

	{

		Complex lhs = parsePower();

		for (;;)

		{

			char op = peek();

			if (op == '*')

			{

				get();

				lhs *= parsePower();

			}

			else if (op == '/')

			{

				get();

				Complex rhs = parsePower();

				if (std::abs(rhs) < 1e-9f)

				{

					rhs = Complex(1e-9f, 0.0f);

				}

				lhs /= rhs;

			}

			else

			{

				break;

			}

		}

		return lhs;

	}



	Complex parseExpression()

	{

		Complex lhs = parseTerm();

		for (;;)

		{

			char op = peek();

			if (op == '+')

			{

				get();

				lhs += parseTerm();

			}

			else if (op == '-')

			{

				get();

				lhs -= parseTerm();

			}

			else

			{

				break;

			}

		}

		return lhs;

	}



public:

	Complex eval(const std::string& expression, float f)

	{

		str = expression;

		pos = 0;

		currentFreq = f;



		if (str.empty())

		{

			return Complex(1.0f, 0.0f);

		}

		return parseExpression();

	}

};





// =======================

//   CONSTRUCTOR

// =======================



TransferFunctionControls::TransferFunctionControls(TransferFunctionEffect* effect) :

	EffectControls(effect),

	m_effect(effect),

	m_volumeModel(1.0f, 1.0f, 17.0f, 1.0f, this, tr("Preset")),

	m_formulaString("1.0 / (1.0 + 1j * (freq / 800.0))"),

	m_bodeMagModel(0.0f, 1.0f, 512, this),

	m_bodePhaseModel(0.0f, 1.0f, 512, this)

{

	// CONNECT KNOB TO PLOT

	// When the preset knob changes, update the graph

	connect(&m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateBodePlot()));



	updateBodePlot();

}





// =======================

//   LOAD / SAVE

// =======================



void TransferFunctionControls::loadSettings(const QDomElement& parent)

{

	m_volumeModel.loadSettings(parent, "volume");



	if (parent.hasAttribute("formula"))

	{

		m_formulaString = parent.attribute("formula");

	}



	updateBodePlot();

}





void TransferFunctionControls::saveSettings(QDomDocument& doc, QDomElement& parent)

{

	m_volumeModel.saveSettings(doc, parent, "volume");

	parent.setAttribute("formula", m_formulaString);

}





// =======================

//   BODE PLOT GENERATION (PRESETS + LOG SCALE)

// =======================



void TransferFunctionControls::updateBodePlot()

{

	const int N = 512;       // Graph Resolution

	const float fs = 44100.0f;

	const float minFreq = 20.0f;        

	const float maxFreq = fs * 0.5f;    



	// Get current preset

	int presetIdx = static_cast<int>(m_volumeModel.value());

	if (presetIdx < 1) presetIdx = 1;

	if (presetIdx > 17) presetIdx = 17;



	std::vector<float> mag(N);

	std::vector<float> phase(N);



	SimpleParser parser;



	for (int i = 0; i < N; ++i)

	{

		// LOGARITHMIC FREQUENCY MAPPING

		float percent = static_cast<float>(i) / static_cast<float>(N - 1);

		float freq = minFreq * std::pow(maxFreq / minFreq, percent);



		Complex H(1.0f, 0.0f);

		float fc, bw, delay; // Temp vars for presets



		if (presetIdx == 1)

		{

			// MODE 1: Custom Text Parser

			H = parser.eval(m_formulaString.toStdString(), freq);

		}

		else

		{

			// MODE 2-17: Hardcoded Presets (Mirrors TransferFunction.cpp)

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



		float m = std::abs(H);

		float p = std::arg(H); // -pi..pi



		// Convert magnitude: -40..+40 dB mapped to 0..1

		float db = 20.0f * std::log10(m + 1e-12f);

		float magNorm = (db + 40.0f) / 80.0f;

		if (magNorm < 0.0f) magNorm = 0.0f;

		if (magNorm > 1.0f) magNorm = 1.0f;

		mag[i] = magNorm;



		// Convert phase: -pi..pi mapped to 0..1

		float phaseNorm = (p + PI) / (2.0f * PI);

		if (phaseNorm < 0.0f) phaseNorm = 0.0f;

		if (phaseNorm > 1.0f) phaseNorm = 1.0f;

		phase[i] = phaseNorm;

	}



	m_bodeMagModel.setSamples(mag.data());

	m_bodePhaseModel.setSamples(phase.data());

}



// =======================

//   SLOTS

// =======================



void TransferFunctionControls::setFormula(const QString& text)

{

	if (m_formulaString != text)

	{

		m_formulaString = text;

		updateBodePlot();

	}

}



} // namespace lmms
