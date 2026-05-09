#ifndef COMMASSISTANT_ICANTRANSPORT_H
#define COMMASSISTANT_ICANTRANSPORT_H

#include <QObject>
#include "canframe.h"

class ICanTransport : public QObject
{
    Q_OBJECT
public:
    explicit ICanTransport(QObject* parent = 0) : QObject(parent) {}
    virtual ~ICanTransport() {}

    virtual bool open(const QString& channel, int bitrate, QString* errorText) = 0;
    virtual void close() = 0;
    virtual bool sendFrame(const CanFrame& frame, QString* errorText) = 0;
    virtual QString name() const = 0;

signals:
    void frameReceived(const CanFrame& frame);
    void errorOccurred(const QString& message);
};

#endif
