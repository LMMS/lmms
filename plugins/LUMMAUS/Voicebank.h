//
// Created by seledreams on 28/02/2021.
//

#ifndef LMMS_VOICEBANK_H
#define LMMS_VOICEBANK_H
#include <QString>
#include <QRegularExpression>
#include "Oto.h"
#include <QVector>
class Voicebank{
public:
    Voicebank();
    Voicebank(QString path);
    ~Voicebank();
    // Loads the informations required to identify the voicebank
    int Load(QString path);
    // Parses the given line to populate the variables
    void ParseLine(QString &line);
    inline QVector<Oto*> getOtos() const{
        return m_otos;
    }
    inline const OtoItem *getOtoData(const std::string &lyric){
    	for (int i = 0; i < m_otos.length();i++){
    		OtoItem *item = m_otos[i]->getOtoItems()[QString(lyric.c_str())];
			if (item){
				return item;
			}
    	}
    	return nullptr;
    }

    inline bool empty()
	{
    	return m_otos.empty();
	}
private:
    QRegularExpression regExp = QRegularExpression("(^.*?)=(.*?$)");
    QVector<Oto*> m_otos;
    QString m_directory;
    QString m_name;
    QString m_author;
    QString m_icon_name;
    QString m_website;
    QString m_sample;
};
#endif //LMMS_VOICEBANK_H
