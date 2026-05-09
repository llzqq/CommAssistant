#include "slcanctransport.h"

#include <QSerialPortInfo>

SlcanCanTransport::SlcanCanTransport(QObject* parent)
    : ICanTransport(parent)
{
    connect(&m_serial, &QSerialPort::readyRead, this, &SlcanCanTransport::onReadyRead);
}

SlcanCanTransport::~SlcanCanTransport()
{
    close();
}

bool SlcanCanTransport::open(const QString& channel, int bitrate, QString* errorText)
{
    if (m_serial.isOpen())
        close();

    m_serial.setPortName(channel);
    m_serial.setBaudRate(QSerialPort::Baud115200);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadWrite)) {
        if (errorText) *errorText = m_serial.errorString();
        return false;
    }

    if (!writeCommand("C\r", errorText))
        return false;
    if (!configureBitrate(bitrate, errorText))
        return false;
    if (!writeCommand("O\r", errorText))
        return false;
    return true;
}

void SlcanCanTransport::close()
{
    if (m_serial.isOpen()) {
        writeCommand("C\r", nullptr);
        m_serial.close();
    }
}

bool SlcanCanTransport::sendFrame(const CanFrame& frame, QString* errorText)
{
    QByteArray encoded;
    if (!encodeFrame(frame, &encoded, errorText))
        return false;
    return writeCommand(encoded + "\r", errorText);
}

bool SlcanCanTransport::writeCommand(const QByteArray& command, QString* errorText)
{
    if (!m_serial.isOpen()) {
        if (errorText) *errorText = "serial port not open";
        return false;
    }

    if (m_serial.write(command) != command.size() || !m_serial.waitForBytesWritten(1000)) {
        if (errorText) *errorText = m_serial.errorString();
        return false;
    }
    return true;
}

bool SlcanCanTransport::configureBitrate(int bitrate, QString* errorText)
{
    const QByteArray cmd = [bitrate]() {
        switch (bitrate) {
        case 125000: return QByteArray("S4\r");
        case 250000: return QByteArray("S5\r");
        case 500000: return QByteArray("S6\r");
        case 1000000: return QByteArray("S8\r");
        default: return QByteArray("S6\r");
        }
    }();
    return writeCommand(cmd, errorText);
}

bool SlcanCanTransport::encodeFrame(const CanFrame& frame, QByteArray* out, QString* errorText) const
{
    if (!out)
        return false;

    if (frame.data.size() > 8) {
        if (errorText) *errorText = "SLCAN classic frame supports up to 8 bytes";
        return false;
    }

    QByteArray payload = frame.data.toHex().toUpper();
    while (payload.size() < frame.data.size() * 2)
        payload.prepend('0');

    QByteArray result;
    if (frame.extended) {
        result.append('T');
        result.append(QByteArray::number(frame.id, 16).toUpper().rightJustified(8, '0'));
    } else {
        result.append('t');
        result.append(QByteArray::number(frame.id, 16).toUpper().rightJustified(3, '0'));
    }
    result.append(QByteArray::number(frame.data.size(), 16).toUpper());
    result.append(payload);
    *out = result;
    return true;
}

bool SlcanCanTransport::decodeLine(const QByteArray& line, CanFrame* frame) const
{
    if (!frame || line.size() < 5)
        return false;

    const char type = line.at(0);
    const bool ext = (type == 'T' || type == 'R');
    const bool stdf = (type == 't' || type == 'r');
    if (!ext && !stdf)
        return false;

    const int idLen = ext ? 8 : 3;
    const int minLen = 1 + idLen + 1;
    if (line.size() < minLen)
        return false;

    bool ok = false;
    frame->id = line.mid(1, idLen).toUInt(&ok, 16);
    if (!ok)
        return false;
    frame->extended = ext;

    int dlc = line.mid(1 + idLen, 1).toInt(&ok, 16);
    if (!ok || dlc < 0 || dlc > 8)
        return false;

    const QByteArray payloadHex = line.mid(1 + idLen + 1, dlc * 2);
    frame->data = QByteArray::fromHex(payloadHex);
    return true;
}

void SlcanCanTransport::onReadyRead()
{
    m_rxBuffer.append(m_serial.readAll());
    int index = -1;
    while ((index = m_rxBuffer.indexOf('\r')) >= 0) {
        QByteArray line = m_rxBuffer.left(index);
        m_rxBuffer.remove(0, index + 1);
        CanFrame frame;
        if (decodeLine(line, &frame))
            emit frameReceived(frame);
    }
}
