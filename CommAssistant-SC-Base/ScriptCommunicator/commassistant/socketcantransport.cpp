#include "socketcantransport.h"

#include <QtSerialBus/QCanBus>
#include <QVariant>

SocketCanTransport::SocketCanTransport(QObject* parent)
    : ICanTransport(parent), m_device(nullptr)
{
}

SocketCanTransport::~SocketCanTransport()
{
    close();
}

bool SocketCanTransport::open(const QString& channel, int bitrate, QString* errorText)
{
    close();

    QString plugin = "socketcan";
    m_device = QCanBus::instance()->createDevice(plugin, channel);
    if (!m_device) {
        if (errorText) *errorText = QString("createDevice failed for %1").arg(plugin);
        return false;
    }

    connect(m_device, &QCanBusDevice::framesReceived, this, &SocketCanTransport::onFramesReceived);
    connect(m_device, &QCanBusDevice::errorOccurred, this, [this](QCanBusDevice::CanBusError) {
        const QString message = m_device ? m_device->errorString() : QStringLiteral("SocketCAN error");
        emit errorOccurred(message);
    });

    m_device->setConfigurationParameter(QCanBusDevice::BitRateKey, QVariant(bitrate));

    if (!m_device->connectDevice()) {
        if (errorText) *errorText = m_device->errorString();
        close();
        return false;
    }

    return true;
}

void SocketCanTransport::close()
{
    if (!m_device)
        return;
    m_device->disconnectDevice();
    delete m_device;
    m_device = nullptr;
}

bool SocketCanTransport::sendFrame(const CanFrame& frame, QString* errorText)
{
    if (!m_device) {
        if (errorText) *errorText = "SocketCAN device not open";
        return false;
    }

    QCanBusFrame canFrame;
    canFrame.setFrameId(frame.id);
    canFrame.setExtendedFrameFormat(frame.extended);
    canFrame.setPayload(frame.data);
    if (!m_device->writeFrame(canFrame)) {
        if (errorText) *errorText = m_device->errorString();
        return false;
    }
    return true;
}

void SocketCanTransport::onFramesReceived()
{
    if (!m_device)
        return;
    while (m_device->framesAvailable() > 0) {
        const QCanBusFrame f = m_device->readFrame();
        CanFrame frame;
        frame.id = f.frameId();
        frame.extended = f.hasExtendedFrameFormat();
        frame.data = f.payload();
        emit frameReceived(frame);
    }
}
