//
// Created by seledreams on 28/02/2021.
//

#ifndef LMMS_OTO_H
#define LMMS_OTO_H
#include <QString>
#include <QRegularExpression>
#include <QMap>

struct OtoItem;
class Oto{
public:
    Oto();
    Oto(QString &path);
    ~Oto();
    // Loads the informations required to identify the voicebank
    int Load(QString path);
    // Parses the given line to populate the variables
    void ParseLine(QString &line);

    inline QMap<QString,OtoItem*> getOtoItems() const{
        return m_oto_items;
    }

    inline QString getDirectory() const{
        return m_directory;
    }
private:
    QString m_directory;
    QMap<QString,OtoItem*> m_oto_items;
    QRegularExpression regExp = QRegularExpression("(^.*?)=(.*?$)");
};
struct OtoItem{
	QString sample;
	QString alias;
	double offset;
	double consonant;
	double cutoff;
	double preutterance;
	double overlap;
	Oto *oto;
};
#endif //LMMS_OTO_H
