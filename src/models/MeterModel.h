#pragma once

#include <QObject>
#include <QMap>
#include <QString>

namespace AetherSDR {

// Describes a single meter definition received from the radio via TCP status.
// Format: "meter N N.src=SLC N.num=0 N.nam=LEVEL N.unit=dBm N.low=-150 N.hi=20"
struct MeterDef {
    int     index{-1};
    QString source;       // "SLC", "RAD", "AMP", "TX", ...
    int     sourceIndex{0};
    QString name;         // "LEVEL", "FWDPWR", "SWR", "PATEMP", ...
    QString unit;         // "dBm", "dB", "dBFS", "SWR", "Volts", "Amps", "degC", "degF", "Watts", "Percent"
    double  low{0.0};
    double  high{0.0};
    QString description;
};

// Central meter value store.
//
// The radio defines meters via TCP status messages (parsed by RadioModel)
// and streams real-time values via VITA-49 PCC 0x8002 UDP packets.
//
// VITA-49 meter payload: N pairs of (uint16 meter_id, int16 raw_value).
// Conversion (from FlexLib Meter.cs UpdateValue):
//   dBm/dB/dBFS/SWR → raw / 128.0
//   Volts/Amps       → raw / 256.0 (firmware >= 1.11; 1024 for older)
//   degF/degC        → raw / 64.0
//   default          → raw (no scaling)
class MeterModel : public QObject {
    Q_OBJECT

public:
    explicit MeterModel(QObject* parent = nullptr);

    // Register or update a meter definition from a TCP status message.
    void defineMeter(const MeterDef& def);

    // Remove a meter by index.
    void removeMeter(int index);

    // Process a batch of raw meter values from a VITA-49 packet.
    // ids[i] is the meter index, vals[i] is the raw int16 value.
    void updateValues(const QVector<quint16>& ids, const QVector<qint16>& vals);

    // Lookup a meter definition by index.
    const MeterDef* meterDef(int index) const;

    // Find the meter index for a given source+name (e.g. "SLC", "LEVEL").
    int findMeter(const QString& source, const QString& name, int sourceIndex = -1) const;

    // Current converted value for a meter index. Returns 0 if unknown.
    float value(int index) const;

    // Convenience: S-meter (slice LEVEL meter) in dBm.
    float sLevel() const { return m_sLevel; }

    // Convenience: forward power in watts.
    float fwdPower() const { return m_fwdPower; }

    // Convenience: SWR.
    float swr() const { return m_swr; }

signals:
    // Emitted when the S-meter value changes (dBm).
    void sLevelChanged(float dbm);

    // Emitted when TX meters change (power, SWR).
    void txMetersChanged(float fwdPower, float swr);

    // Emitted when any meter value changes (for debug/generic display).
    void meterUpdated(int index, float value);

private:
    float convertRaw(const MeterDef& def, qint16 raw) const;

    QMap<int, MeterDef> m_defs;        // meter index → definition
    QMap<int, float>    m_values;      // meter index → last converted value

    // Cached indices for fast lookup of important meters
    int m_sLevelIdx{-1};     // "SLC" / "LEVEL"
    int m_fwdPwrIdx{-1};     // "FWDPWR"
    int m_swrIdx{-1};        // "SWR"

    // Cached values
    float m_sLevel{-130.0f};
    float m_fwdPower{0.0f};
    float m_swr{1.0f};
};

} // namespace AetherSDR
