//
// Created by seledreams on 06/03/2021.
//

#ifndef LUMMAUSINSTRUMENT_H
#define LUMMAUSINSTRUMENT_H
#include <Engine.h>
#include "VocalInstrument.h"
#include "InstrumentView.h"
#include "Voicebank.h"
#include "Oto.h"
#include "Song.h"
class VocalNote;
class LUMMAUSInstrument : public VocalInstrument
{
public:
	LUMMAUSInstrument(VocalInstrumentTrack *_vocal_instrument_track);
	virtual ~LUMMAUSInstrument();
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const;

	virtual PluginView * instantiateView( QWidget * _parent );
	virtual void bounceVocalPattern(VocalPattern *_pattern_to_bounce);
	virtual void bounceVocalPatterns(VocalPattern **_patterns_to_bounce,int _count);
	static std::vector<float> *getUTAUPitchBend(VocalNote *_vocal_note);

	void renderUTAU(VocalPattern *pattern);
	static std::string convertNumberTo12Bit(int number);

private:
	void CallResampler(const OtoItem *otoItem,VocalNote* note,const std::string &output_path, int id = 0);
	void CallWavtool(const OtoItem *otoItem,VocalNote *note,const std::string &output_path, const std::string &input_path, int id = 0, bool last = false);
	std::thread *renderingThread;
	Voicebank vb;
	double factor;
};

class LUMMAUSInstrumentView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	LUMMAUSInstrumentView(VocalInstrument *_vocal_instrument, QWidget *_parent);
	virtual ~LUMMAUSInstrumentView();
};
#endif //LUMMAUSINSTRUMENT_H
