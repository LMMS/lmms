//
// Created by seledreams on 02/03/2021.
//

#ifndef VOCALPATTERN_H
#define VOCALPATTERN_H
#include "Pattern.h"
#include <thread>
#include <SampleBuffer.h>
class VocalNote;
class LMMS_EXPORT VocalPattern : public Pattern
{
public:
	VocalPattern(InstrumentTrack *track);
	VocalPattern(Pattern &pattern);
	Note * addNote( const Note & _new_note, const bool _quant_pos = true ) override;
	void ResetAllLyrics();
	void ResetAllNotes();
	inline bool isPlaying() const{
		return m_playing;
	}
	void setIsPlaying(bool newState){
		m_playing = newState;
	}
	inline int getRenderingProcessesCalled() const{
		return m_renderingProcessesCalled;
	}
	void setRenderingProcessesCalled(int newProcesses){
		m_renderingProcessesCalled = newProcesses;
	}

	void setLyric(VocalNote *note,const std::string &lyric);
private:
	bool m_playing;
	std::string m_bouncePath;
	int m_renderingProcessesCalled;
};
#endif //VOCALPATTERN_H
