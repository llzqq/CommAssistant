#ifndef COMMASSISTANT_DIALOG_H
#define COMMASSISTANT_DIALOG_H

#include <QDialog>
#include <QDateTime>
#include <QPointer>
#include <QSerialPort>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <memory>

#include "icantransport.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QPlainTextEdit;
class QStackedWidget;

class CommAssistantDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CommAssistantDialog(QWidget* parent = 0);
    ~CommAssistantDialog() override;

    bool isBridgeActive() const;
    bool isCanBridgeActive() const;
    bool sendFromMainWindow(const QByteArray& payload, QString* errorText);

signals:
    void bridgeStateChanged(bool active, bool isCanMode);
    void bridgePayloadReceived(const QByteArray& payload, bool isCanMode);
    void bridgePayloadSent(const QByteArray& payload, bool success, bool isCanMode);

private slots:
    void onModeChanged(int index);
    void onApplyConfigClicked();
    void onStartTestClicked();

    void onSerialReadyRead();
    void onTcpReadyRead();
    void onUdpReadyRead();
    void onTcpNewConnection();
    void onHandshakeTimeout();

private:
    enum CommMode
    {
        MODE_RS422 = 0,
        MODE_CAN = 1,
        MODE_ETHERNET = 2
    };

    void buildUi();
    QString configFilePath() const;
    void loadProfiles();
    void saveProfiles();
    void closeAllTransports();
    bool hasActiveBridgeConnection() const;
    void updateBridgeState();
    void setStatus(const QString& text);
    void appendLog(const QString& text);
    void processReceivedPayload(const QByteArray& payload, const QString& sourceTag);
    bool ensureCanTransport(QString* errorText);
    bool sendCanFrame(const CanFrame& frame, QString* errorText);
    bool sendPayload(const QByteArray& payload, QString* errorText);
    bool sendHello(QString* errorText);
    QByteArray ackBytes() const { return QByteArray("ACK"); }

    QComboBox* m_modeCombo;
    QStackedWidget* m_modeStack;

    QLineEdit* m_rsPortEdit;
    QComboBox* m_rsBaudCombo;
    QComboBox* m_rsDataBitsCombo;
    QComboBox* m_rsParityCombo;
    QComboBox* m_rsStopBitsCombo;

    QComboBox* m_canTypeCombo;
    QLineEdit* m_canChannelEdit;
    QComboBox* m_canBitrateCombo;
    QComboBox* m_canIdTypeCombo;
    QLineEdit* m_canIdEdit;

    QComboBox* m_ethProtocolCombo;
    QComboBox* m_ethRoleCombo;
    QLineEdit* m_ethLocalIpEdit;
    QLineEdit* m_ethLocalPortEdit;
    QLineEdit* m_ethRemoteIpEdit;
    QLineEdit* m_ethRemotePortEdit;

    QLabel* m_connectionStatusLabel;
    QLabel* m_handshakeStatusLabel;
    QLabel* m_lastRxLabel;
    QLabel* m_lastTxLabel;
    QLabel* m_errorLabel;
    QPlainTextEdit* m_logEdit;

    QSerialPort m_serial;
    QTcpServer m_tcpServer;
    QPointer<QTcpSocket> m_tcpSocket;
    QUdpSocket m_udpSocket;
    std::unique_ptr<ICanTransport> m_canTransport;

    QTimer m_handshakeTimer;
    bool m_waitingAck;
    bool m_bridgeActive;
};

#endif
