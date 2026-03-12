#include "PanadapterStream.h"
#include "RadioConnection.h"

#include <QNetworkDatagram>
#include <QtEndian>
#include <QDebug>

namespace AetherSDR {

// ─── VITA-49 header layout (28 bytes, big-endian) ─────────────────────────────
// Word 0 (bytes  0- 3): Packet header (type, flags, packet count, size in words)
// Word 1 (bytes  4- 7): Stream ID
// Word 2 (bytes  8-11): Class ID OUI / info type (upper)
// Word 3 (bytes 12-15): Class ID (lower)
// Word 4 (bytes 16-19): Integer timestamp (seconds, UTC)
// Word 5 (bytes 20-23): Fractional timestamp (upper)
// Word 6 (bytes 24-27): Fractional timestamp (lower)
// Byte 28+            : Payload — int16_t FFT bins, big-endian
//
// Stream ID patterns (FLEX radio):
//   0x4xxxxxxx — panadapter FFT frames
//   0x42xxxxxx — waterfall pixel rows
//   0x0001xxxx — audio (remote audio)

PanadapterStream::PanadapterStream(QObject* parent)
    : QObject(parent)
{
    connect(&m_socket, &QUdpSocket::readyRead,
            this, &PanadapterStream::onDatagramReady);
}

bool PanadapterStream::isRunning() const
{
    return m_socket.state() == QAbstractSocket::BoundState;
}

bool PanadapterStream::start(RadioConnection* conn)
{
    if (isRunning()) return true;

    // Port 0: let the OS pick an available ephemeral port.
    if (!m_socket.bind(QHostAddress::AnyIPv4, 0)) {
        qWarning() << "PanadapterStream: failed to bind UDP socket:"
                   << m_socket.errorString();
        return false;
    }

    m_localPort = m_socket.localPort();
    qDebug() << "PanadapterStream: bound to UDP port" << m_localPort;

    // Tell the radio where to send VITA-49 datagrams.
    // Must be called after "client gui" has been sent.
    conn->sendCommand(QString("client set udpport=%1").arg(m_localPort));

    return true;
}

void PanadapterStream::stop()
{
    m_socket.close();
    m_localPort = 0;
}

// ─── Datagram reception ───────────────────────────────────────────────────────

void PanadapterStream::onDatagramReady()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram dg = m_socket.receiveDatagram();
        if (!dg.isNull())
            processDatagram(dg.data());
    }
}

void PanadapterStream::processDatagram(const QByteArray& data)
{
    if (data.size() < VITA49_HEADER_BYTES + 2) return;

    const auto* raw = reinterpret_cast<const uchar*>(data.constData());

    // Read stream ID (word 1, bytes 4-7, big-endian).
    const quint32 streamId = qFromBigEndian<quint32>(raw + 4);

    // Accept panadapter frames (0x40000000–0x40FFFFFF).
    // Skip waterfall (0x42000000), audio (0x0001xxxx), and others.
    if ((streamId & 0xFF000000u) != 0x40000000u) return;

    // Payload: signed 16-bit FFT bins, big-endian.
    const int payloadBytes = data.size() - VITA49_HEADER_BYTES;
    const int binCount     = payloadBytes / 2;
    if (binCount <= 0) return;

    const uchar* payload = raw + VITA49_HEADER_BYTES;

    QVector<float> bins(binCount);
    for (int i = 0; i < binCount; ++i) {
        const qint16 sample = qFromBigEndian<qint16>(payload + i * 2);
        // Scale: raw value / 128.0 gives dBm (FLEX VITA-49 convention).
        bins[i] = static_cast<float>(sample) / 128.0f;
    }

    emit spectrumReady(bins);
}

} // namespace AetherSDR
