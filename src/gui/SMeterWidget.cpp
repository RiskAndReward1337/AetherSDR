#include "SMeterWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QFontMetrics>

namespace AetherSDR {

// ─── Construction ────────────────────────────────────────────────────────────

SMeterWidget::SMeterWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(minimumSizeHint());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Peak hold decay: drops 0.5 dB every 50 ms after a new peak
    m_peakDecay.setInterval(50);
    connect(&m_peakDecay, &QTimer::timeout, this, [this]() {
        m_peakDbm -= 0.5f;
        if (m_peakDbm < m_levelDbm) {
            m_peakDbm = m_levelDbm;
            m_peakDecay.stop();
        }
        update();
    });

    // Hard reset peak hold every 10 seconds
    m_peakReset.setInterval(10000);
    m_peakReset.start();
    connect(&m_peakReset, &QTimer::timeout, this, [this]() {
        m_peakDbm = m_levelDbm;
        update();
    });
}

// ─── Public interface ────────────────────────────────────────────────────────

void SMeterWidget::setLevel(float dbm)
{
    // Exponential smoothing for needle movement
    m_levelDbm = m_levelDbm + SMOOTH_ALPHA * (dbm - m_levelDbm);

    // Peak hold
    if (dbm > m_peakDbm) {
        m_peakDbm = dbm;
        m_peakDecay.start();
    }

    update();
}

QString SMeterWidget::sUnitsText() const
{
    if (m_levelDbm <= S0_DBM) return "S0";
    if (m_levelDbm <= S9_DBM) {
        const int s = qRound((m_levelDbm - S0_DBM) / DB_PER_S);
        return QString("S%1").arg(qBound(0, s, 9));
    }
    const int over = qRound(m_levelDbm - S9_DBM);
    return QString("S9+%1").arg(over);
}

// ─── Mapping ─────────────────────────────────────────────────────────────────

float SMeterWidget::dbmToFraction(float dbm) const
{
    // S0 to S9 occupies the left 60% of the arc
    // S9 to S9+60 occupies the right 40%
    const float clamped = qBound(S0_DBM, dbm, MAX_DBM);

    if (clamped <= S9_DBM) {
        // Linear within S0..S9 → 0.0..0.6
        return 0.6f * (clamped - S0_DBM) / (S9_DBM - S0_DBM);
    }
    // Linear within S9..S9+60 → 0.6..1.0
    return 0.6f + 0.4f * (clamped - S9_DBM) / (MAX_DBM - S9_DBM);
}

// ─── Paint ───────────────────────────────────────────────────────────────────

void SMeterWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor(0x0f, 0x0f, 0x1a));

    // The gauge arc: center at bottom-center, radius fills most of the widget
    const float cx = w * 0.5f;
    const float cy = h * 0.88f;
    const float radius = qMin(w * 0.42f, h * 0.72f);

    // Arc spans from 180° (left) to 0° (right) — i.e. the top semicircle
    // fraction 0.0 → angle 180°, fraction 1.0 → angle 0°
    auto fractionToAngle = [](float frac) -> float {
        return M_PI * (1.0f - frac);  // radians, 0=right, PI=left
    };

    // ── Draw arc background ──────────────────────────────────────────────
    QPen arcPen(QColor(0x20, 0x30, 0x40), 2);
    p.setPen(arcPen);
    // Draw thin arc line
    const QRectF arcRect(cx - radius, cy - radius, radius * 2, radius * 2);
    p.drawArc(arcRect, 0, 180 * 16);  // Qt uses 1/16 degree units

    // ── Draw scale ticks and labels ──────────────────────────────────────
    QFont tickFont = font();
    tickFont.setPixelSize(qMax(9, h / 14));
    tickFont.setBold(true);
    p.setFont(tickFont);

    // S-unit ticks: S1 through S9
    for (int s = 1; s <= 9; ++s) {
        const float dbm = S0_DBM + s * DB_PER_S;
        const float frac = dbmToFraction(dbm);
        const float angle = fractionToAngle(frac);

        const float tickInner = radius - 10;
        const float tickOuter = radius - 2;
        const float labelR    = radius - 20;

        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);

        // Tick line
        p.setPen(QPen(s <= 9 ? QColor(0xc8, 0xd8, 0xe8) : QColor(0xff, 0x44, 0x44), 1.5));
        p.drawLine(QPointF(cx + tickInner * cosA, cy - tickInner * sinA),
                   QPointF(cx + tickOuter * cosA, cy - tickOuter * sinA));

        // Label
        const QString label = (s <= 9) ? QString::number(s) : QString("+%1").arg((s - 9) * 10);
        const QFontMetrics fm(tickFont);
        const int tw = fm.horizontalAdvance(label);
        p.setPen(s <= 9 ? QColor(0xc8, 0xd8, 0xe8) : QColor(0xff, 0x44, 0x44));
        p.drawText(QPointF(cx + labelR * cosA - tw / 2.0,
                           cy - labelR * sinA + fm.ascent() / 2.0), label);
    }

    // Over-S9 ticks: +10, +20, +40, +60
    const int overs[] = {10, 20, 40, 60};
    for (int over : overs) {
        const float dbm = S9_DBM + over;
        const float frac = dbmToFraction(dbm);
        const float angle = fractionToAngle(frac);

        const float tickInner = radius - 10;
        const float tickOuter = radius - 2;
        const float labelR    = radius - 20;

        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);

        p.setPen(QPen(QColor(0xff, 0x44, 0x44), 1.5));
        p.drawLine(QPointF(cx + tickInner * cosA, cy - tickInner * sinA),
                   QPointF(cx + tickOuter * cosA, cy - tickOuter * sinA));

        const QString label = QString("+%1").arg(over);
        const QFontMetrics fm(tickFont);
        const int tw = fm.horizontalAdvance(label);
        p.setPen(QColor(0xff, 0x44, 0x44));
        p.drawText(QPointF(cx + labelR * cosA - tw / 2.0,
                           cy - labelR * sinA + fm.ascent() / 2.0), label);
    }

    // ── Draw colored arc segments ────────────────────────────────────────
    // White arc from S0 to S9, red from S9 to S9+60
    {
        // White segment: 0.0..0.6 → angles 180°..72°
        QPen whitePen(QColor(0xc8, 0xd8, 0xe8), 3);
        p.setPen(whitePen);
        const float innerR = radius - 12;
        const QRectF innerArc(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
        // Qt drawArc: start angle in 1/16°, CCW positive
        // fraction 0.0 = 180°, fraction 0.6 = 72°
        p.drawArc(innerArc,
                  static_cast<int>(72 * 16),        // start at 72°
                  static_cast<int>((180 - 72) * 16)); // span 108°

        // Red segment: 0.6..1.0 → angles 72°..0°
        QPen redPen(QColor(0xff, 0x44, 0x44), 3);
        p.setPen(redPen);
        p.drawArc(innerArc,
                  0,                              // start at 0°
                  static_cast<int>(72 * 16));     // span 72°
    }

    // ── Draw needle ──────────────────────────────────────────────────────
    {
        const float frac = dbmToFraction(m_levelDbm);
        const float angle = fractionToAngle(frac);
        const float needleLen = radius - 24;

        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);

        // Needle shadow
        p.setPen(QPen(QColor(0, 0, 0, 80), 3));
        p.drawLine(QPointF(cx + 1, cy + 1),
                   QPointF(cx + 1 + needleLen * cosA, cy + 1 - needleLen * sinA));

        // Needle
        p.setPen(QPen(QColor(0xff, 0xff, 0xff), 2));
        p.drawLine(QPointF(cx, cy),
                   QPointF(cx + needleLen * cosA, cy - needleLen * sinA));

        // Center dot
        p.setBrush(QColor(0x00, 0xb4, 0xd8));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(cx, cy), 4, 4);
    }

    // ── Draw peak marker (small triangle) ────────────────────────────────
    if (m_peakDbm > m_levelDbm + 1.0f) {
        const float frac = dbmToFraction(m_peakDbm);
        const float angle = fractionToAngle(frac);
        const float markerR = radius - 2;

        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);

        const QPointF tip(cx + markerR * cosA, cy - markerR * sinA);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xff, 0xaa, 0x00));
        // Small triangle pointing inward
        const float perpCos = -sinA;
        const float perpSin = cosA;
        const float sz = 3.0f;
        QPainterPath tri;
        tri.moveTo(tip);
        tri.lineTo(tip.x() - 6 * cosA + sz * perpCos,
                   tip.y() + 6 * sinA + sz * perpSin);
        tri.lineTo(tip.x() - 6 * cosA - sz * perpCos,
                   tip.y() + 6 * sinA - sz * perpSin);
        tri.closeSubpath();
        p.drawPath(tri);
    }

    // ── Text readout — all top-aligned on the same baseline ─────────────
    // Source label centered at top
    QFont srcFont = font();
    srcFont.setPixelSize(qMax(9, h / 14));
    const QFontMetrics sfm(srcFont);
    const int topY = sfm.height() + 2;

    p.setFont(srcFont);
    p.setPen(QColor(0x80, 0x90, 0xa0));
    p.drawText((w - sfm.horizontalAdvance(m_source)) / 2, topY, m_source);

    // S-units — top left, larger font
    QFont sFont = font();
    sFont.setPixelSize(qMax(13, h / 8));
    sFont.setBold(true);
    p.setFont(sFont);
    p.setPen(QColor(0x00, 0xb4, 0xd8));
    p.drawText(6, topY, sUnitsText());

    // dBm value — top right, same size as S-units
    QFont dbmFont = font();
    dbmFont.setPixelSize(qMax(13, h / 8));
    dbmFont.setBold(true);
    p.setFont(dbmFont);
    const QFontMetrics dfm(dbmFont);
    const QString dbmText = QString("%1 dBm").arg(m_levelDbm, 0, 'f', 0);
    p.setPen(QColor(0xc8, 0xd8, 0xe8));
    p.drawText(w - dfm.horizontalAdvance(dbmText) - 6, topY, dbmText);
}

} // namespace AetherSDR
