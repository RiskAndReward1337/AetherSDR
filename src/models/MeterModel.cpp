#include "MeterModel.h"
#include <QDebug>

namespace AetherSDR {

MeterModel::MeterModel(QObject* parent)
    : QObject(parent)
{}

void MeterModel::defineMeter(const MeterDef& def)
{
    m_defs[def.index] = def;

    // Cache indices for high-frequency lookups
    if (def.source == "SLC" && def.name == "LEVEL")
        m_sLevelIdx = def.index;
    else if (def.name == "FWDPWR")
        m_fwdPwrIdx = def.index;
    else if (def.name == "SWR")
        m_swrIdx = def.index;

    qDebug() << "MeterModel: defined meter" << def.index
             << def.source << def.sourceIndex << def.name
             << def.unit << "[" << def.low << "->" << def.high << "]";
}

void MeterModel::removeMeter(int index)
{
    m_defs.remove(index);
    m_values.remove(index);

    if (index == m_sLevelIdx)  m_sLevelIdx = -1;
    if (index == m_fwdPwrIdx)  m_fwdPwrIdx = -1;
    if (index == m_swrIdx)     m_swrIdx = -1;
}

float MeterModel::convertRaw(const MeterDef& def, qint16 raw) const
{
    // Conversion factors from FlexLib Meter.cs UpdateValue()
    // Firmware v1.4.0.0: volt_denom = 1024 (version < 1.11.0.0)
    if (def.unit == "dBm" || def.unit == "dB" || def.unit == "dBFS" || def.unit == "SWR")
        return static_cast<float>(raw) / 128.0f;
    if (def.unit == "Volts" || def.unit == "Amps")
        return static_cast<float>(raw) / 1024.0f;  // v1.4.0.0
    if (def.unit == "degF" || def.unit == "degC")
        return static_cast<float>(raw) / 64.0f;
    return static_cast<float>(raw);
}

void MeterModel::updateValues(const QVector<quint16>& ids, const QVector<qint16>& vals)
{
    const int n = qMin(ids.size(), vals.size());
    bool sChanged = false;
    bool txChanged = false;

    for (int i = 0; i < n; ++i) {
        const int idx = static_cast<int>(ids[i]);
        auto it = m_defs.constFind(idx);
        if (it == m_defs.constEnd()) continue;

        const float v = convertRaw(*it, vals[i]);
        m_values[idx] = v;

        if (idx == m_sLevelIdx) {
            m_sLevel = v;
            sChanged = true;
        } else if (idx == m_fwdPwrIdx) {
            m_fwdPower = v;
            txChanged = true;
        } else if (idx == m_swrIdx) {
            m_swr = v;
            txChanged = true;
        }

        emit meterUpdated(idx, v);
    }

    if (sChanged)
        emit sLevelChanged(m_sLevel);
    if (txChanged)
        emit txMetersChanged(m_fwdPower, m_swr);
}

const MeterDef* MeterModel::meterDef(int index) const
{
    auto it = m_defs.constFind(index);
    return (it != m_defs.constEnd()) ? &(*it) : nullptr;
}

int MeterModel::findMeter(const QString& source, const QString& name, int sourceIndex) const
{
    for (auto it = m_defs.constBegin(); it != m_defs.constEnd(); ++it) {
        if (it->source == source && it->name == name) {
            if (sourceIndex < 0 || it->sourceIndex == sourceIndex)
                return it->index;
        }
    }
    return -1;
}

float MeterModel::value(int index) const
{
    return m_values.value(index, 0.0f);
}

} // namespace AetherSDR
