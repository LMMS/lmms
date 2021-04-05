//
// Created by seledreams on 28/02/2021.
//
#include "Voicebank.h"
#include <QFile>
#include <QRegularExpression>
#include <QTextDecoder>
#include <iostream>
#include <QTextStream>
#include <QDirIterator>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>
Voicebank::Voicebank() : m_name(""),m_directory("")
{

}

Voicebank::Voicebank(QString path) : Voicebank()
{
    Load(path);
}

Voicebank::~Voicebank()
{
    for (int i = 0;i < m_otos.count();i++){
        delete m_otos[i];
        m_otos[i] = nullptr;
    }
}


int Voicebank::Load(QString path)
{
	std::string spath = path.toStdString();
    bool character_exists = QFile::exists(path + "/character.txt");
    if (!character_exists){
        return EXIT_FAILURE;
    }
    QFile file(path + "/character.txt");
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            ParseLine(line);
        }
        file.close();
    }
    QDirIterator it(path, QStringList() << "oto.ini", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()){
        QString tempPath = it.next();
        QFileInfo file_info(tempPath);
        tempPath = file_info.absolutePath();
        Oto *oto = new Oto(tempPath);
        m_otos.push_back(oto);
        qDebug() << QString(tempPath + " Loaded");
    }
    return EXIT_SUCCESS;
}

void Voicebank::ParseLine(QString &line)
{
    QRegularExpressionMatchIterator match = regExp.globalMatch(line);
    if (!match.isValid())
        return;
    while (match.hasNext()){
        QRegularExpressionMatch i = match.next();
        QString key = i.captured(1);
        std::cout << key.toStdString() << endl;
        if (key == "name"){
            m_name = i.captured(2);
        }
        else if (key == "image"){
            m_icon_name = i.captured(2);
        }
        else if (key == "author"){
            m_author = i.captured(2);
        }
        else if (key == "website"){
            m_website = i.captured(2);
        }
        else if (key == "sample"){
            m_sample = i.captured(1);
        }
    }
}
