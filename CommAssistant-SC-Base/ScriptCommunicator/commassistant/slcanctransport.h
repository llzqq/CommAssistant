#ifndef COMMASSISTANT_SLCANCANTRANSPORT_H
#define COMMASSISTANT_SLCANCANTRANSPORT_H

#include "icantransport.h"
#include <QSerialPort>
#include <QTimer>

class SlcanCanTransport : public ICanTransport
{
    Q_OBJECT
public:
    explicit SlcanCanTransport(QObject* parent = 0);
    ~SlcanCanTransport() override;

    bool open(const QString& channel, int bitrate, QString* errorText) override;
    void close() override;
    bool sendFrame(const CanFrame& frame, QString* errorText) override;
    QString name() const override { return "SLCAN"; }

private slots:
    void onReadyRead();

private:
    bool writeCommand(const QByteArray& command, QString* errorText);
    bool configureBitrate(int bitrate, QString* errorText);
    bool encodeFrame(const CanFrame& frame, QByteArray* out, QString* errorText) const;
    bool decodeLine(const QByteArray& line, CanFrame* frame) const;

    QSerialPort m_serial;
    QByteArray m_rxBuffer;
};

#endif
