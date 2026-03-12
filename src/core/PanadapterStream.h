#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QVector>

namespace AetherSDR {

class RadioConnection;

// Receives VITA-49 UDP datagrams from the radio's panadapter stream,
// decodes 16-bit FFT bins, and emits spectrumReady() for the spectrum widget.
//
// Protocol:
//   1. Call start(conn) — binds an OS-assigned UDP port, tells the radio via
//      "client set udpport=<port>" (must be called after "client gui").
//   2. The radio sends VITA-49 IF-Data datagrams to that port at ~25 fps.
//   3. Datagrams whose VITA-49 stream ID matches 0x4xxxxxxx (panadapter) are
//      decoded; audio and waterfall streams are ignored.
//   4. spectrumReady(binsDbm) is emitted for each decoded frame.

class PanadapterStream : public QObject {
    Q_OBJECT

public:
    static constexpr int VITA49_HEADER_BYTES = 28;

    explicit PanadapterStream(QObject* parent = nullptr);

    // Bind a local UDP port (OS-chosen) and register it with the radio.
    // conn must remain valid for the lifetime of this stream.
    bool start(RadioConnection* conn);
    void stop();

    quint16 localPort() const { return m_localPort; }
    bool    isRunning() const;

signals:
    void spectrumReady(const QVector<float>& binsDbm);

private slots:
    void onDatagramReady();

private:
    void processDatagram(const QByteArray& data);

    QUdpSocket m_socket;
    quint16    m_localPort{0};
};

} // namespace AetherSDR
