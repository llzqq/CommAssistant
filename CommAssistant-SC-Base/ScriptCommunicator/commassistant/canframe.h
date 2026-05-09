#ifndef COMMASSISTANT_CANFRAME_H
#define COMMASSISTANT_CANFRAME_H

#include <QByteArray>
#include <QString>

struct CanFrame
{
    quint32 id = 0;
    bool extended = false;
    QByteArray data;

    QString toLogString(const QString& direction) const
    {
        return QString("%1 ID=0x%2 DLC=%3 DATA=%4")
                .arg(direction)
                .arg(id, extended ? 8 : 3, 16, QLatin1Char('0'))
                .arg(data.size())
                .arg(QString(data.toHex(' ')).toUpper());
    }
};

#endif
