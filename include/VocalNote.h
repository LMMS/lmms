//
// Created by seledreams on 02/03/2021.
//

#ifndef VOCALNOTE_H
#define VOCALNOTE_H
#include "Note.h"
#include "SampleBuffer.h"
#include "DetuningHelper.h"

class LMMS_EXPORT VocalNote : public Note{
public:
	VocalNote();

	VocalNote(const Note &note);

	void ResetVocalNote();

	void setLength( const TimePos & newLength ) override;

	inline const std::string &getLyric() const
	{
		return m_lyric;
	}

	void setLyric(const std::string &newLyric)
	{
		m_lyric = newLyric;
	}

	inline const std::string &getPhonemes() const
	{
		return m_phonemes;
	}

	void setPhonemes(const std::string &newPhonemes)
	{
		m_phonemes = newPhonemes;
	}

	inline const std::string &getResamplerFlags() const
	{
		return m_resamplerFlags;
	}

	void setResamplerFlags(const std::string &newResamplerFLags)
	{
		m_resamplerFlags = newResamplerFLags;
	}

	inline const std::string &getCachesSample() const
	{
		return m_cachedSample;
	}

	void setCachedSample(const std::string &newCachedSample)
	{
		m_cachedSample = newCachedSample;
	}
	inline const std::array<double,5> &getEnvelopeWidth() const
	{
		return m_envelopeWidth;
	}

	inline const std::array<double,5> &getEnvelopeHeight() const
	{
		return m_envelopeHeight;
	}

	inline const std::array<int,10> &getVibrato() const
	{
		return m_vibrato;
	}

	inline double getOverlap() const
	{
		return m_overlap;
	}

	void setOverlap(double newOverlap)
	{
		m_overlap = newOverlap;
	}

	inline double getPreutterance() const
	{
		return m_preutterance;
	}

	void setPreutterance(double newPreutterance)
	{
		m_preutterance = newPreutterance;
	}

	inline double getVelocity() const
	{
		return getVolume();
	}

	void setVelocity(double newVelocity)
	{
		setVolume(newVelocity);
	}

	inline double getModulation() const
	{
		return m_modulation;
	}

	void setModulation(double newModulation)
	{
		m_modulation = newModulation;
	}

	static inline const char *getDefaultLyric(){
		return m_defaultlyric;
	}

private:
	static constexpr const char* m_defaultlyric {"la"};
	std::string m_lyric {m_defaultlyric};
	std::string m_phonemes{};
	std::string m_resamplerFlags{};
	std::string m_cachedSample{};
	std::array<double,5> m_envelopeWidth{};
	std::array<double,5> m_envelopeHeight{};
	std::array<int,10> m_vibrato{}; // temporary, will probably be replaced by two values from an automation pattern
	double m_overlap{};
	double m_preutterance{};
	double m_velocity{};
	double m_modulation{};
};
#endif //VOCALNOTE_H
