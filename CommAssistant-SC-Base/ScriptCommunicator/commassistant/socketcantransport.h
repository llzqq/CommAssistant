#ifndef COMMASSISTANT_SOCKETCANTRANSPORT_H
#define COMMASSISTANT_SOCKETCANTRANSPORT_H

#include "icantransport.h"
#include <QtSerialBus/QCanBusDevice>
#include <QtSerialBus/QCanBusFrame>

class SocketCanTransport : public ICanTransport
{
    Q_OBJECT
public:
    explicit SocketCanTransport(QObject* parent = 0);
    ~SocketCanTransport() override;

    bool open(const QString& channel, int bitrate, QString* errorText) override;
    void close() override;
    bool sendFrame(const CanFrame& frame, QString* errorText) override;
    QString name() const override { return "SocketCAN"; }

private slots:
    void onFramesReceived();

private:
    QCanBusDevice* m_device;
};

#endif
