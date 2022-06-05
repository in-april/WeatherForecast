#ifndef WEATHERTOOL_H
#define WEATHERTOOL_H
#include <QCoreApplication>
#include <QFile>
#include <map>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


class WeatherTool
{
public:
    WeatherTool()
    {
        QString filename = QCoreApplication::applicationDirPath();
        filename += "/citycode-2019-08-23.json";
        QFile file(filename);
        file.open(QIODevice::ReadOnly|QIODevice::Text);
        QByteArray json = file.readAll();
        file.close();
        QJsonParseError err;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(json, &err);
        QJsonArray citys = jsonDoc.array();
        for(int i = 0;i < citys.size(); i++)
        {
            QString cityname = citys.at(i).toObject().value("city_name").toString();
            QString citycode = citys.at(i).toObject().value("city_code").toString();
            if(citycode.size() > 0)
            {
                m_mapCity2Code.insert(std::pair<QString,QString>(cityname,citycode));
            }
        }
    }

    QString operator[](QString cityname)
    {
        auto it = m_mapCity2Code.find(cityname);
        if(it != m_mapCity2Code.end())
        {
            return it->second;
        }
        else
        {
            return "000000000";
        }
    }

private:
    std::map<QString, QString> m_mapCity2Code;
};

#endif // WEATHERTOOL_H
