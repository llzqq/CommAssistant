#include "commassistantdialog.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QMessageBox>
#include <QStackedWidget>
#include <QVariant>

#include "pcan/PCANBasic.h"
#include "slcanctransport.h"
#include "socketcantransport.h"

namespace
{
QString now()
{
    return QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
}

QString payloadForLog(const QByteArray& payload)
{
    const QString text = QString::fromUtf8(payload);
    return text.isEmpty() ? QString(payload.toHex(' ')).toUpper() : text;
}

QString addressForLog(const QHostAddress& address)
{
    return address.isNull() ? QString("<null>") : address.toString();
}

QString socketStateForLog(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState: return "Unconnected";
    case QAbstractSocket::HostLookupState: return "HostLookup";
    case QAbstractSocket::ConnectingState: return "Connecting";
    case QAbstractSocket::ConnectedState: return "Connected";
    case QAbstractSocket::BoundState: return "Bound";
    case QAbstractSocket::ListeningState: return "Listening";
    case QAbstractSocket::ClosingState: return "Closing";
    default: return "Unknown";
    }
}

QString socketErrorForLog(QAbstractSocket::SocketError error)
{
    return QString::number(static_cast<int>(error));
}

QString modeName(int index)
{
    switch (index) {
    case 0: return "RS422";
    case 1: return "CAN";
    case 2: return "Ethernet";
    default: return "Unknown";
    }
}

QString defaultRsPort()
{
#ifdef Q_OS_LINUX
    return "/dev/ttyUSB0";
#else
    return "COM3";
#endif
}

QString defaultCanDriver()
{
#ifdef Q_OS_LINUX
    return "SocketCAN";
#else
    return "SLCAN";
#endif
}

QString defaultCanChannel()
{
#ifdef Q_OS_LINUX
    return "can0";
#else
    return "COM4";
#endif
}
}

CommAssistantDialog::CommAssistantDialog(QWidget* parent)
    : QDialog(parent),
      m_modeCombo(nullptr),
      m_modeStack(nullptr),
      m_rsPortEdit(nullptr),
      m_rsBaudCombo(nullptr),
      m_rsDataBitsCombo(nullptr),
      m_rsParityCombo(nullptr),
      m_rsStopBitsCombo(nullptr),
      m_canTypeCombo(nullptr),
      m_canChannelEdit(nullptr),
      m_canBitrateCombo(nullptr),
      m_canIdTypeCombo(nullptr),
      m_canIdEdit(nullptr),
      m_ethProtocolCombo(nullptr),
      m_ethRoleCombo(nullptr),
      m_ethLocalIpEdit(nullptr),
      m_ethLocalPortEdit(nullptr),
      m_ethRemoteIpEdit(nullptr),
      m_ethRemotePortEdit(nullptr),
      m_connectionStatusLabel(nullptr),
      m_handshakeStatusLabel(nullptr),
      m_lastRxLabel(nullptr),
      m_lastTxLabel(nullptr),
      m_errorLabel(nullptr),
      m_logEdit(nullptr),
      m_waitingAck(false),
      m_bridgeActive(false)
{
    buildUi();
    loadProfiles();

    connect(&m_serial, &QSerialPort::readyRead, this, &CommAssistantDialog::onSerialReadyRead);
    connect(&m_tcpServer, &QTcpServer::newConnection, this, &CommAssistantDialog::onTcpNewConnection);
    connect(&m_udpSocket, &QUdpSocket::readyRead, this, &CommAssistantDialog::onUdpReadyRead);
    connect(&m_handshakeTimer, &QTimer::timeout, this, &CommAssistantDialog::onHandshakeTimeout);
    m_handshakeTimer.setSingleShot(true);

    onModeChanged(m_modeCombo->currentIndex());
    setStatus("idle");
}

CommAssistantDialog::~CommAssistantDialog()
{
    saveProfiles();
    closeAllTransports();
}

bool CommAssistantDialog::isBridgeActive() const
{
    return hasActiveBridgeConnection();
}

bool CommAssistantDialog::isCanBridgeActive() const
{
    return hasActiveBridgeConnection() && (m_modeCombo->currentIndex() == MODE_CAN);
}

bool CommAssistantDialog::sendFromMainWindow(const QByteArray& payload, QString* errorText)
{
    if (!hasActiveBridgeConnection()) {
        if (errorText) *errorText = "Comm Assistant link is not active";
        return false;
    }

    if (m_modeCombo->currentIndex() == MODE_CAN) {
        if (payload.size() < 5) {
            if (errorText) *errorText = "CAN payload is too short";
            return false;
        }

        const quint8 canType = static_cast<quint8>(payload.at(0));
        if ((canType & PCAN_MESSAGE_FD) || (canType & PCAN_MESSAGE_BRS) || (canType & PCAN_MESSAGE_RTR)) {
            if (errorText) *errorText = "Comm Assistant CAN bridge supports standard/extended data frames only";
            return false;
        }

        CanFrame frame;
        frame.extended = ((canType & 0x02U) != 0);
        frame.id = (static_cast<quint8>(payload.at(1)) << 24) |
                   (static_cast<quint8>(payload.at(2)) << 16) |
                   (static_cast<quint8>(payload.at(3)) << 8) |
                   static_cast<quint8>(payload.at(4));
        frame.data = payload.mid(5);
        return sendCanFrame(frame, errorText);
    }

    return sendPayload(payload, errorText);
}

void CommAssistantDialog::buildUi()
{
    setWindowTitle("Communications Assistant");
    resize(980, 720);

    auto *root = new QVBoxLayout(this);

    auto *modeRow = new QHBoxLayout();
    modeRow->addWidget(new QLabel("Mode:", this));
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItems(QStringList() << "RS422" << "CAN" << "Ethernet");
    modeRow->addWidget(m_modeCombo, 1);

    auto *btnApply = new QPushButton("Apply", this);
    auto *btnTest = new QPushButton("Start Test", this);
    modeRow->addWidget(btnApply);
    modeRow->addWidget(btnTest);
    root->addLayout(modeRow);

    m_modeStack = new QStackedWidget(this);

    auto *rsWidget = new QWidget(this);
    auto *rsForm = new QFormLayout(rsWidget);
    m_rsPortEdit = new QLineEdit(defaultRsPort(), rsWidget);
    m_rsBaudCombo = new QComboBox(rsWidget);
    m_rsBaudCombo->addItems(QStringList() << "9600" << "115200" << "921600");
    m_rsDataBitsCombo = new QComboBox(rsWidget);
    m_rsDataBitsCombo->addItems(QStringList() << "8" << "7");
    m_rsParityCombo = new QComboBox(rsWidget);
    m_rsParityCombo->addItems(QStringList() << "None" << "Even" << "Odd");
    m_rsStopBitsCombo = new QComboBox(rsWidget);
    m_rsStopBitsCombo->addItems(QStringList() << "1" << "2");
    rsForm->addRow("Port", m_rsPortEdit);
    rsForm->addRow("Baud", m_rsBaudCombo);
    rsForm->addRow("Data Bits", m_rsDataBitsCombo);
    rsForm->addRow("Parity", m_rsParityCombo);
    rsForm->addRow("Stop Bits", m_rsStopBitsCombo);

    auto *canWidget = new QWidget(this);
    auto *canForm = new QFormLayout(canWidget);
    m_canTypeCombo = new QComboBox(canWidget);
    m_canTypeCombo->addItems(QStringList() << "SLCAN" << "SocketCAN");
    m_canTypeCombo->setCurrentText(defaultCanDriver());
    m_canChannelEdit = new QLineEdit(defaultCanChannel(), canWidget);
    m_canBitrateCombo = new QComboBox(canWidget);
    m_canBitrateCombo->addItems(QStringList() << "125000" << "250000" << "500000" << "1000000");
    m_canIdTypeCombo = new QComboBox(canWidget);
    m_canIdTypeCombo->addItems(QStringList() << "Standard" << "Extended");
    m_canIdEdit = new QLineEdit("123", canWidget);
    canForm->addRow("Driver", m_canTypeCombo);
    canForm->addRow("Channel", m_canChannelEdit);
    canForm->addRow("Bitrate", m_canBitrateCombo);
    canForm->addRow("Frame Type", m_canIdTypeCombo);
    canForm->addRow("Frame ID", m_canIdEdit);

    auto *ethWidget = new QWidget(this);
    auto *ethForm = new QFormLayout(ethWidget);
    m_ethProtocolCombo = new QComboBox(ethWidget);
    m_ethProtocolCombo->addItems(QStringList() << "TCP" << "UDP");
    m_ethRoleCombo = new QComboBox(ethWidget);
    m_ethRoleCombo->addItems(QStringList() << "Client" << "Server");
    m_ethLocalIpEdit = new QLineEdit("0.0.0.0", ethWidget);
    m_ethLocalPortEdit = new QLineEdit("5000", ethWidget);
    m_ethRemoteIpEdit = new QLineEdit("127.0.0.1", ethWidget);
    m_ethRemotePortEdit = new QLineEdit("5000", ethWidget);
    ethForm->addRow("Protocol", m_ethProtocolCombo);
    ethForm->addRow("Role", m_ethRoleCombo);
    ethForm->addRow("Local IP", m_ethLocalIpEdit);
    ethForm->addRow("Local Port", m_ethLocalPortEdit);
    ethForm->addRow("Remote IP", m_ethRemoteIpEdit);
    ethForm->addRow("Remote Port", m_ethRemotePortEdit);

    m_modeStack->addWidget(rsWidget);
    m_modeStack->addWidget(canWidget);
    m_modeStack->addWidget(ethWidget);
    root->addWidget(m_modeStack);

    auto *statusBox = new QGroupBox("Status", this);
    auto *statusForm = new QFormLayout(statusBox);
    m_connectionStatusLabel = new QLabel("Idle", statusBox);
    m_handshakeStatusLabel = new QLabel("Not started", statusBox);
    m_lastRxLabel = new QLabel("-", statusBox);
    m_lastTxLabel = new QLabel("-", statusBox);
    m_errorLabel = new QLabel("-", statusBox);
    statusForm->addRow("Connection", m_connectionStatusLabel);
    statusForm->addRow("Handshake", m_handshakeStatusLabel);
    statusForm->addRow("Last RX", m_lastRxLabel);
    statusForm->addRow("Last TX", m_lastTxLabel);
    statusForm->addRow("Error", m_errorLabel);
    root->addWidget(statusBox);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    root->addWidget(m_logEdit, 1);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CommAssistantDialog::onModeChanged);
    connect(btnApply, &QPushButton::clicked, this, &CommAssistantDialog::onApplyConfigClicked);
    connect(btnTest, &QPushButton::clicked, this, &CommAssistantDialog::onStartTestClicked);
}

QString CommAssistantDialog::configFilePath() const
{
    QString base = QCoreApplication::applicationDirPath();
    QDir dir(base);
    if (!dir.exists("config"))
        dir.mkpath("config");
    return dir.filePath("config/comm_profiles.json");
}

void CommAssistantDialog::loadProfiles()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const auto doc = QJsonDocument::fromJson(file.readAll());
    const auto root = doc.object();
    const auto rs = root.value("rs422").toObject();
    if (!rs.isEmpty()) {
        m_rsPortEdit->setText(rs.value("port").toString(m_rsPortEdit->text()));
        m_rsBaudCombo->setCurrentText(QString::number(rs.value("baud").toInt(m_rsBaudCombo->currentText().toInt())));
    }
    const auto can = root.value("can").toObject();
    if (!can.isEmpty()) {
        m_canTypeCombo->setCurrentText(can.value("type").toString(m_canTypeCombo->currentText()));
        m_canChannelEdit->setText(can.value("channel").toString(m_canChannelEdit->text()));
        m_canBitrateCombo->setCurrentText(QString::number(can.value("bitrate").toInt(m_canBitrateCombo->currentText().toInt())));
        m_canIdTypeCombo->setCurrentText(can.value("idType").toString(m_canIdTypeCombo->currentText()));
        m_canIdEdit->setText(can.value("id").toString(m_canIdEdit->text()));
    }
    const auto eth = root.value("ethernet").toObject();
    if (!eth.isEmpty()) {
        m_ethProtocolCombo->setCurrentText(eth.value("protocol").toString(m_ethProtocolCombo->currentText()));
        m_ethRoleCombo->setCurrentText(eth.value("role").toString(m_ethRoleCombo->currentText()));
        m_ethLocalIpEdit->setText(eth.value("localIp").toString(m_ethLocalIpEdit->text()));
        m_ethLocalPortEdit->setText(QString::number(eth.value("localPort").toInt(m_ethLocalPortEdit->text().toInt())));
        m_ethRemoteIpEdit->setText(eth.value("remoteIp").toString(m_ethRemoteIpEdit->text()));
        m_ethRemotePortEdit->setText(QString::number(eth.value("remotePort").toInt(m_ethRemotePortEdit->text().toInt())));
    }
}

void CommAssistantDialog::saveProfiles()
{
    QJsonObject root;
    root["rs422"] = QJsonObject{
        {"port", m_rsPortEdit->text()},
        {"baud", m_rsBaudCombo->currentText().toInt()}
    };
    root["can"] = QJsonObject{
        {"type", m_canTypeCombo->currentText()},
        {"channel", m_canChannelEdit->text()},
        {"bitrate", m_canBitrateCombo->currentText().toInt()},
        {"idType", m_canIdTypeCombo->currentText()},
        {"id", m_canIdEdit->text()}
    };
    root["ethernet"] = QJsonObject{
        {"protocol", m_ethProtocolCombo->currentText()},
        {"role", m_ethRoleCombo->currentText()},
        {"localIp", m_ethLocalIpEdit->text()},
        {"localPort", m_ethLocalPortEdit->text().toInt()},
        {"remoteIp", m_ethRemoteIpEdit->text()},
        {"remotePort", m_ethRemotePortEdit->text().toInt()}
    };

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void CommAssistantDialog::closeAllTransports()
{
    m_handshakeTimer.stop();
    m_waitingAck = false;
    m_serial.close();
    if (m_tcpSocket) {
        m_tcpSocket->close();
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }
    m_tcpServer.close();
    m_udpSocket.close();
    if (m_canTransport)
        m_canTransport->close();
    m_canTransport.reset();
    m_bridgeActive = false;
    emit bridgeStateChanged(false, false);
}

bool CommAssistantDialog::hasActiveBridgeConnection() const
{
    const int mode = m_modeCombo->currentIndex();
    if (mode == MODE_RS422)
        return m_serial.isOpen();
    if (mode == MODE_CAN)
        return m_canTransport != nullptr;
    if (m_ethProtocolCombo->currentText() == "UDP")
        return m_udpSocket.state() == QAbstractSocket::BoundState;
    return (m_tcpSocket != nullptr) && (m_tcpSocket->state() == QAbstractSocket::ConnectedState);
}

void CommAssistantDialog::updateBridgeState()
{
    const bool active = hasActiveBridgeConnection();
    const int mode = m_modeCombo->currentIndex();

    if (m_bridgeActive == active)
        return;

    m_bridgeActive = active;
    emit bridgeStateChanged(m_bridgeActive, m_bridgeActive && mode == MODE_CAN);
}

void CommAssistantDialog::setStatus(const QString& text)
{
    m_connectionStatusLabel->setText(text);
}

void CommAssistantDialog::appendLog(const QString& text)
{
    m_logEdit->appendPlainText(QString("[%1] %2").arg(now(), text));
}

void CommAssistantDialog::onModeChanged(int index)
{
    m_modeStack->setCurrentIndex(index);
}

void CommAssistantDialog::onApplyConfigClicked()
{
    saveProfiles();
    closeAllTransports();

    QString errorText;
    bool ok = false;
    const int mode = m_modeCombo->currentIndex();
    if (mode == MODE_RS422) {
        m_serial.setPortName(m_rsPortEdit->text().trimmed());
        m_serial.setBaudRate(m_rsBaudCombo->currentText().toInt());
        m_serial.setDataBits(m_rsDataBitsCombo->currentText() == "7" ? QSerialPort::Data7 : QSerialPort::Data8);
        m_serial.setParity(m_rsParityCombo->currentText() == "Even" ? QSerialPort::EvenParity :
                           (m_rsParityCombo->currentText() == "Odd" ? QSerialPort::OddParity : QSerialPort::NoParity));
        m_serial.setStopBits(m_rsStopBitsCombo->currentText() == "2" ? QSerialPort::TwoStop : QSerialPort::OneStop);
        m_serial.setFlowControl(QSerialPort::NoFlowControl);
        ok = m_serial.open(QIODevice::ReadWrite);
        errorText = ok ? QString() : m_serial.errorString();
    } else if (mode == MODE_CAN) {
        ok = ensureCanTransport(&errorText);
        if (ok)
            ok = m_canTransport->open(m_canChannelEdit->text().trimmed(), m_canBitrateCombo->currentText().toInt(), &errorText);
    } else {
        const bool server = (m_ethRoleCombo->currentText() == "Server");
        const bool udp = (m_ethProtocolCombo->currentText() == "UDP");
        appendLog(QString("Ethernet apply: protocol=%1 role=%2 local=%3:%4 remote=%5:%6")
                  .arg(m_ethProtocolCombo->currentText(),
                       m_ethRoleCombo->currentText(),
                       m_ethLocalIpEdit->text().trimmed(),
                       m_ethLocalPortEdit->text().trimmed(),
                       m_ethRemoteIpEdit->text().trimmed(),
                       m_ethRemotePortEdit->text().trimmed()));
        if (udp) {
            const quint16 localPort = m_ethLocalPortEdit->text().toUShort();
            const QHostAddress localAddress(m_ethLocalIpEdit->text().trimmed());
            appendLog(QString("UDP bind try: localAddress=%1 parsed=%2 localPort=%3")
                      .arg(addressForLog(localAddress),
                           localAddress.isNull() ? "false" : "true",
                           QString::number(localPort)));
            ok = m_udpSocket.bind(localAddress, localPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
            errorText = ok ? QString() : m_udpSocket.errorString();
        } else if (server) {
            const QHostAddress listenAddress(m_ethLocalIpEdit->text().trimmed());
            const quint16 listenPort = m_ethLocalPortEdit->text().toUShort();
            appendLog(QString("TCP server listen try: localAddress=%1 parsed=%2 localPort=%3")
                      .arg(addressForLog(listenAddress),
                           listenAddress.isNull() ? "false" : "true",
                           QString::number(listenPort)));
            ok = m_tcpServer.listen(listenAddress, listenPort);
            errorText = ok ? QString() : m_tcpServer.errorString();
        } else {
            auto *sock = new QTcpSocket(this);
            connect(sock, &QTcpSocket::readyRead, this, &CommAssistantDialog::onTcpReadyRead);
            connect(sock, &QTcpSocket::errorOccurred, this, [this, sock](QAbstractSocket::SocketError error) {
                appendLog(QString("TCP socket error: code=%1 text=%2 state=%3 local=%4:%5 peer=%6:%7")
                          .arg(socketErrorForLog(error),
                               sock->errorString(),
                               socketStateForLog(sock->state()),
                               sock->localAddress().toString(),
                               QString::number(sock->localPort()),
                               sock->peerAddress().toString(),
                               QString::number(sock->peerPort())));
            });
            const QString remoteHostText = m_ethRemoteIpEdit->text().trimmed();
            const quint16 remotePort = m_ethRemotePortEdit->text().toUShort();
            const QString localIpText = m_ethLocalIpEdit->text().trimmed();
            const quint16 localPort = m_ethLocalPortEdit->text().toUShort();
            QHostAddress localAddress;
            const bool localAddressValid = localIpText.isEmpty() || localIpText == "0.0.0.0" || localAddress.setAddress(localIpText);
            QHostAddress remoteAddress;
            const bool remoteAddressValid = remoteAddress.setAddress(remoteHostText);
            appendLog(QString("TCP client parse: localInput=%1 localParsed=%2 remoteInput=%3 remoteParsed=%4 remoteValue=%5")
                      .arg(localIpText.isEmpty() ? "<empty>" : localIpText,
                           localAddressValid ? "true" : "false",
                           remoteHostText.isEmpty() ? "<empty>" : remoteHostText,
                           remoteAddressValid ? "true" : "false",
                           addressForLog(remoteAddress)));

            if (remoteHostText.isEmpty()) {
                errorText = "Remote IP is empty";
            } else if (!remoteAddressValid) {
                errorText = "Remote IP is invalid";
            } else if (remotePort == 0) {
                errorText = "Remote port is invalid";
            } else if (!localAddressValid) {
                errorText = "Local IP is invalid";
            } else {
                const QHostAddress bindAddress = (localIpText.isEmpty() || localIpText == "0.0.0.0")
                                                 ? QHostAddress::AnyIPv4 : localAddress;
                appendLog(QString("TCP client bind try: bindAddress=%1 localPort=%2")
                          .arg(addressForLog(bindAddress), QString::number(localPort)));
                if (!sock->bind(bindAddress, localPort)) {
                    errorText = QString("Bind failed: %1").arg(sock->errorString());
                    appendLog(QString("TCP client bind failed: state=%1 errorCode=%2")
                              .arg(socketStateForLog(sock->state()), socketErrorForLog(sock->error())));
                } else {
                    appendLog(QString("TCP client connect try: remoteAddress=%1 remotePort=%2")
                              .arg(addressForLog(remoteAddress), QString::number(remotePort)));
                    sock->connectToHost(remoteAddress, remotePort);
                    ok = sock->waitForConnected(3000);
                    if (ok) {
                        appendLog(QString("TCP client connected: local=%1:%2 peer=%3:%4 state=%5")
                                  .arg(sock->localAddress().toString(),
                                       QString::number(sock->localPort()),
                                       sock->peerAddress().toString(),
                                       QString::number(sock->peerPort()),
                                       socketStateForLog(sock->state())));
                    } else {
                        errorText = sock->errorString();
                        appendLog(QString("TCP client connect failed: state=%1 errorCode=%2 errorText=%3 local=%4:%5 peer=%6:%7")
                                  .arg(socketStateForLog(sock->state()),
                                       socketErrorForLog(sock->error()),
                                       sock->errorString(),
                                       sock->localAddress().toString(),
                                       QString::number(sock->localPort()),
                                       sock->peerAddress().toString(),
                                       QString::number(sock->peerPort())));
                    }
                }
            }
            if (ok)
                m_tcpSocket = sock;
            else
                sock->deleteLater();
        }
        if (udp && ok)
            ok = true;
    }

    if (ok) {
        if (mode == MODE_ETHERNET && m_ethProtocolCombo->currentText() == "TCP" && m_ethRoleCombo->currentText() == "Server")
            setStatus("Listening");
        else
            setStatus("Connected");
        appendLog(QString("%1 configured").arg(modeName(mode)));
        m_errorLabel->setText("-");
        updateBridgeState();
    } else {
        setStatus("Failed");
        m_errorLabel->setText(errorText);
        appendLog(QString("apply failed: %1").arg(errorText));
        updateBridgeState();
    }
}

bool CommAssistantDialog::ensureCanTransport(QString* errorText)
{
    if (m_canTransport)
        return true;
    if (m_canTypeCombo->currentText() == "SocketCAN")
        m_canTransport.reset(new SocketCanTransport(this));
    else
        m_canTransport.reset(new SlcanCanTransport(this));
    connect(m_canTransport.get(), &ICanTransport::frameReceived, this, [this](const CanFrame& frame) {
        processReceivedPayload(frame.data, QString("CAN RX %1").arg(frame.toLogString("RX")));
    });
    connect(m_canTransport.get(), &ICanTransport::errorOccurred, this, [this](const QString& message) {
        if (!message.isEmpty()) {
            m_errorLabel->setText(message);
            appendLog(message);
        }
    });
    Q_UNUSED(errorText)
    return true;
}

bool CommAssistantDialog::sendHello(QString* errorText)
{
    return sendPayload(QByteArray("HELLO"), errorText);
}

bool CommAssistantDialog::sendCanFrame(const CanFrame& frame, QString* errorText)
{
    if (!m_canTransport) {
        if (errorText) *errorText = "CAN transport not ready";
        return false;
    }
    const bool ok = m_canTransport->sendFrame(frame, errorText);
    if (ok) {
        m_lastTxLabel->setText(frame.toLogString("TX"));
        appendLog(frame.toLogString("TX"));
        emit bridgePayloadSent(frame.data, true, true);
    }
    return ok;
}

bool CommAssistantDialog::sendPayload(const QByteArray& payload, QString* errorText)
{
    const QString payloadText = payloadForLog(payload);
    const int mode = m_modeCombo->currentIndex();
    if (mode == MODE_RS422) {
        if (m_serial.write(payload) != payload.size() || !m_serial.waitForBytesWritten(500)) {
            if (errorText) *errorText = m_serial.errorString();
            return false;
        }
        m_lastTxLabel->setText(payloadText);
        appendLog(QString("RS422 TX %1").arg(payloadText));
        emit bridgePayloadSent(payload, true, false);
        return true;
    }
    if (mode == MODE_CAN) {
        CanFrame frame;
        frame.id = m_canIdEdit->text().toUInt(nullptr, 16);
        frame.extended = (m_canIdTypeCombo->currentText() == "Extended");
        frame.data = payload;
        return sendCanFrame(frame, errorText);
    }
    if (m_ethProtocolCombo->currentText() == "UDP") {
        const QByteArray packet = payload;
        const QHostAddress remoteIp(m_ethRemoteIpEdit->text());
        const quint16 remotePort = m_ethRemotePortEdit->text().toUShort();
        qint64 sent = m_udpSocket.writeDatagram(packet, remoteIp, remotePort);
        if (sent < 0) {
            if (errorText) *errorText = m_udpSocket.errorString();
            return false;
        }
        m_lastTxLabel->setText(payloadText);
        appendLog(QString("UDP TX %1").arg(payloadText));
        emit bridgePayloadSent(payload, true, false);
        return true;
    }

    if (!m_tcpSocket) {
        if (errorText) *errorText = "TCP socket not connected";
        return false;
    }
    if (m_tcpSocket->write(payload) < 0 || !m_tcpSocket->waitForBytesWritten(500)) {
        if (errorText) *errorText = m_tcpSocket->errorString();
        return false;
    }
    m_lastTxLabel->setText(payloadText);
    appendLog(QString("TCP TX %1").arg(payloadText));
    emit bridgePayloadSent(payload, true, false);
    return true;
}

void CommAssistantDialog::onStartTestClicked()
{
    QString errorText;
    if (!sendHello(&errorText)) {
        m_errorLabel->setText(errorText);
        m_handshakeStatusLabel->setText("TX failed");
        appendLog(QString("test failed: %1").arg(errorText));
        return;
    }
    m_waitingAck = true;
    m_handshakeStatusLabel->setText("Waiting ACK");
    m_handshakeTimer.start(1500);
}

void CommAssistantDialog::processReceivedPayload(const QByteArray& payload, const QString& sourceTag)
{
    const QString text = payloadForLog(payload);
    m_lastRxLabel->setText(QString("%1 %2").arg(sourceTag, text.isEmpty() ? QString(payload.toHex(' ')).toUpper() : text));
    appendLog(QString("%1 RX %2").arg(sourceTag, text.isEmpty() ? QString(payload.toHex(' ')).toUpper() : text));
    emit bridgePayloadReceived(payload, m_modeCombo->currentIndex() == MODE_CAN);
    if (payload.contains("HELLO")) {
        QString errorText;
        if (sendPayload(QByteArray("ACK"), &errorText)) {
            appendLog(QString("%1 TX ACK").arg(sourceTag));
            m_lastTxLabel->setText("ACK");
        } else if (!errorText.isEmpty()) {
            m_errorLabel->setText(errorText);
            appendLog(QString("auto ACK failed: %1").arg(errorText));
        }
    }
    if (m_waitingAck && payload.contains(ackBytes())) {
        m_waitingAck = false;
        m_handshakeTimer.stop();
        m_handshakeStatusLabel->setText("Handshake success");
        appendLog("handshake success");
    }
}

void CommAssistantDialog::onSerialReadyRead()
{
    processReceivedPayload(m_serial.readAll(), "RS422");
}

void CommAssistantDialog::onTcpReadyRead()
{
    if (m_tcpSocket)
        processReceivedPayload(m_tcpSocket->readAll(), "TCP");
}

void CommAssistantDialog::onUdpReadyRead()
{
    while (m_udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_udpSocket.pendingDatagramSize()));
        m_udpSocket.readDatagram(datagram.data(), datagram.size());
        processReceivedPayload(datagram, "UDP");
    }
}

void CommAssistantDialog::onTcpNewConnection()
{
    if (m_tcpSocket)
        m_tcpSocket->deleteLater();
    m_tcpSocket = m_tcpServer.nextPendingConnection();
    if (m_tcpSocket)
        connect(m_tcpSocket, &QTcpSocket::readyRead, this, &CommAssistantDialog::onTcpReadyRead);
    appendLog("TCP server accepted connection");
    setStatus("Connected");
    updateBridgeState();
}

void CommAssistantDialog::onHandshakeTimeout()
{
    if (!m_waitingAck)
        return;
    m_waitingAck = false;
    m_handshakeStatusLabel->setText("Handshake timeout");
    appendLog("handshake timeout");
}
