//
// Created by seledreams on 28/02/2021.
//

#include <QFile>
#include <QTextStream>
#include "Oto.h"
#include <QDebug>
int Oto::Load(QString path)
{
    bool oto_exists = QFile::exists(path + "/oto.ini");
    m_directory = path;
    if (!oto_exists){
        return EXIT_FAILURE;
    }
    QFile file(path + "/oto.ini");
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
    return EXIT_SUCCESS;
}

void Oto::ParseLine(QString &line)
{
    QRegularExpressionMatchIterator match = regExp.globalMatch(line);
    if (!match.isValid())
        return;
    while (match.hasNext()){
        QRegularExpressionMatch i = match.next();
        QString key = i.captured(1);
        QString values = i.captured(2);
        if (values.contains(",")){
            QStringList params = values.split(",",QString::SplitBehavior::KeepEmptyParts);
            OtoItem *item = new OtoItem();
            item->sample = key;
            item->alias = params[0];
            item->offset = params[1].toDouble();
            item->consonant = params[2].toDouble();
            item->cutoff = params[3].toDouble();
            item->preutterance = params[4].toDouble();
            item->overlap = params[5].toDouble();
            item->oto = this;
            qDebug() << key + " : alias="
            + QString(item->alias)
            + " offset="
            + QString::number(item->offset)
            + " consonant="
            + QString::number(item->consonant)
            + " cutoff="
            + QString::number(item->cutoff)
            + " preutterance="
            + QString::number(item->preutterance)
            + " overlap="
            + QString::number(item->overlap);
            m_oto_items.insert(QString(item->alias),item);
        }
    }
}

Oto::Oto()
{
}

Oto::Oto(QString &path) : Oto()
{
    Load(path);
}

Oto::~Oto()
{
    for(auto item : m_oto_items){
        delete item;
        item = nullptr;
    }
}
