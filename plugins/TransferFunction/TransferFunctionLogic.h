#ifndef TRANSFER_FUNCTION_LOGIC_H
#define TRANSFER_FUNCTION_LOGIC_H
#include <complex>
#include <string>
#include <cmath>
#include <vector>
#include <cctype>
#include <algorithm>
namespace lmms
{
using Complex = std::complex<float>;
static const float PI_LOGIC = 3.14159265358979f;
// =============================================================
// NEW: DRAWING DATA STRUCTURES
// =============================================================
struct DrawPoint {
    float freq; 
    float mag;  
    bool operator<(const DrawPoint& other) const { return freq < other.freq; }
};
inline float getDrawnMagnitude(float freq, const std::vector<DrawPoint>& points) {
    if (points.empty()) return 1.0f; 
    if (freq <= points.front().freq) return points.front().mag;
    if (freq >= points.back().freq) return points.back().mag;
    auto it = std::upper_bound(points.begin(), points.end(), DrawPoint{freq, 0.0f});
    const DrawPoint& p2 = *it;
    const DrawPoint& p1 = *(--it);
    float t = (freq - p1.freq) / (p2.freq - p1.freq);
    return p1.mag + t * (p2.mag - p1.mag);
}
// =============================================================
// FORMULA STRING HELPER (MISSING FUNCTION ADDED HERE)
// =============================================================
inline std::string getPresetFormulaString(int presetIdx) {
    switch(presetIdx) {
        case 1: return "Custom (See Entry Box)";
        case 2: return "Lowpass 4th Order: 1 / sqrt(1 + (freq/800)^8)";
        case 3: return "Highpass 4th Order: 1 / sqrt(1 + (500/freq)^8)";
        case 4: return "Low Shelf: 1 + 1 / (1 + (freq/500)^2)";
        case 5: return "High Shelf: 1 + 1 / (1 + (2000/(freq+1))^2)";
        case 6: return "Telephone: Bandpass 300Hz - 3000Hz";
        case 7: return "Notch: 1 - exp(-(freq-600)^2 / (2*40^2))";
        case 8: return "Resonator: 1 + 8 * exp(-(20*(freq-900)/900)^2)";
        case 9: return "Comb: 1 + exp(-j * 2*pi*freq * 0.002)";
        case 10: return "Comb Notch: 1 - exp(-j * 2*pi*freq * 0.003)";
        case 11: return "Allpass: (1 - j*(freq/1200)) / (1 + j*(freq/1200))";
        case 12: return "Bitcrusher: Square Wave (Freq % 500 < 250)";
        case 13: return "Spectral Decay: exp(-0.0004 * freq)";
        case 14: return "Gate Sweep: exp(-0.0008*(freq-500)) * sin(...)";
        case 15: return "Formants: Sum of 3 Gaussian peaks";
        case 16: return "Odd Booster: 1 + 0.8 * sin(pi*freq/200)";
        case 17: return "Phase Tilt: exp(j * 0.0005 * freq)";
        case 18: return "Custom Draw: Linear Interpolated Curve";
        default: return "Unknown";
    }
}
// =============================================================
// SHARED PARSER CLASS
// =============================================================
class SimpleParser
{
    std::string str;
    size_t pos;
    float currentFreq;
    char peek() {
        while (pos < str.length() && std::isspace(static_cast<unsigned char>(str[pos]))) pos++;
        if (pos >= str.length()) return 0;
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
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            size_t start = pos;
            while (pos < str.length() && (std::isdigit(static_cast<unsigned char>(str[pos])) || str[pos] == '.')) pos++;
            float val = std::stof(str.substr(start, pos - start));
            if (peek() == 'j') { get(); return Complex(0.0f, val); }
            return Complex(val, 0.0f);
        }
        if (std::isalpha(static_cast<unsigned char>(c))) {
            std::string token;
            while (pos < str.length() && std::isalpha(static_cast<unsigned char>(str[pos]))) token += str[pos++];
            if (token == "freq") return Complex(currentFreq, 0.0f);
            if (token == "w")    return Complex(2.0f * PI_LOGIC * currentFreq, 0.0f);
            if (token == "s")    return Complex(0.0f, 2.0f * PI_LOGIC * currentFreq);
            if (token == "j")    return Complex(0.0f, 1.0f);
            if (token == "pi")   return Complex(PI_LOGIC, 0.0f);
            if (peek() == '(') {
                get();
                Complex arg = parseExpression();
                if (peek() == ')') get();
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
// =============================================================
// SHARED EVALUATION FUNCTION (SSOT)
// =============================================================
inline Complex calculateTransferFunction(int presetIdx, float freq, 
                                         const std::string& customFormula, 
                                         SimpleParser& parser,
                                         const std::vector<DrawPoint>* drawPoints = nullptr)
{
    Complex H(1.0f, 0.0f);
    float fc, bw, delay; 
    if (presetIdx == 1)
    {
        H = parser.eval(customFormula, freq);
    }
    else if (presetIdx == 18) 
    {
        if (drawPoints && !drawPoints->empty()) {
            float normMag = getDrawnMagnitude(freq, *drawPoints);
            float db = normMag * 80.0f - 40.0f;
            float linear = std::pow(10.0f, db / 20.0f);
            H = Complex(linear, 0.0f); 
        } else {
            H = Complex(1.0f, 0.0f);
        }
    }
    else
    {
        switch(presetIdx) {
            case 2: // Lowpass 4th Order
                H = 1.0f / std::sqrt(1.0f + std::pow(freq / 800.0f, 8.0f)); 
                break;
            case 3: // Highpass 4th Order
                if (freq < 1.0f) freq = 1.0f;
                H = 1.0f / std::sqrt(1.0f + std::pow(500.0f / freq, 8.0f)); 
                break;
            case 4: // Low Shelf
                H = 1.0f + 1.0f / (1.0f + std::pow(freq / 500.0f, 2.0f)); 
                break;
            case 5: // High Shelf
                H = 1.0f + 1.0f / (1.0f + std::pow(2000.0f / (freq + 1.0f), 2.0f)); 
                break;
            case 6: // Telephone
                if (freq > 300.0f && freq < 3000.0f) { H = Complex(1.0f, 0.0f); } 
                else { H = Complex(0.0f, 0.0f); }
                break;
            case 7: // Notch
                fc = 600.0f; bw = 40.0f;
                H = 1.0f - std::exp(-std::pow(freq - fc, 2.0f) / (2.0f * bw * bw)); 
                break;
            case 8: // Resonator
                fc = 900.0f; 
                H = 1.0f + 8.0f * std::exp(-std::pow(20.0f * (freq - fc) / fc, 2.0f)); 
                break;
            case 9: // Comb
                delay = 0.002f;
                H = 1.0f + std::exp(Complex(0, -1) * 2.0f * PI_LOGIC * freq * delay); 
                break;
            case 10: // Comb Notch
                delay = 0.003f;
                H = 1.0f - std::exp(Complex(0, -1) * 2.0f * PI_LOGIC * freq * delay); 
                break;
            case 11: // Allpass
                fc = 1200.0f;
                H = (1.0f - Complex(0,1) * (freq/fc)) / (1.0f + Complex(0,1) * (freq/fc)); 
                break;
            case 12: // Bitcrusher
                H = (std::fmod(freq, 500.0f) < 250.0f) ? 1.0f : -1.0f; 
                break;
            case 13: // Spectral Decay
                H = std::exp(-0.0004f * freq); 
                break;
            case 14: // Spectral Gate Sweep
                if (freq > 500.0f) { H = std::exp(-0.0008f * (freq - 500.0f)) * std::sin(freq * 0.005f); } 
                else { H = 0.0f; }
                break;
            case 15: // Formants
                H = std::exp(-std::pow(freq-800.0f, 2.0f)/(2.0f*6400.0f)) +
                    std::exp(-std::pow(freq-1500.0f, 2.0f)/(2.0f*22500.0f)) +
                    std::exp(-std::pow(freq-2500.0f, 2.0f)/(2.0f*40000.0f)); 
                break;
            case 16: // Odd Harmonic Booster
                H = 1.0f + 0.8f * std::sin(PI_LOGIC * freq / 200.0f); 
                break;
            case 17: // Phase Tilt
                H = std::exp(Complex(0, 1) * 0.0005f * freq); 
                break;
            default: 
                H = Complex(1.0f, 0.0f); 
                break;
        }
    }
    return H;
}
} // namespace lmms
#endif // TRANSFER_FUNCTION_LOGIC_H
