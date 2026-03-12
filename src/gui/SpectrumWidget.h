#pragma once

#include <QWidget>
#include <QVector>
#include <QImage>

namespace AetherSDR {

// Panadapter / spectrum display widget.
//
// The widget is split vertically:
//   Top ~40%  — spectrum line plot (current FFT frame, smoothed)
//   Bottom ~60% — waterfall (scrolling heat-map history)
//
// Feed live data via updateSpectrum() at whatever rate the radio delivers it.
class SpectrumWidget : public QWidget {
    Q_OBJECT

public:
    explicit SpectrumWidget(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {800, 300}; }

    // Set the frequency range covered by this panadapter
    void setFrequencyRange(double centerMhz, double bandwidthMhz);

    // Feed a new FFT frame. bins are scaled dBm values.
    void updateSpectrum(const QVector<float>& binsDbm);

    // Overlay: show a vertical line for the active slice at freqMhz
    void setSliceFrequency(double freqMhz);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void drawGrid(QPainter& p, const QRect& r);
    void drawSpectrum(QPainter& p, const QRect& r);
    void drawSliceMarker(QPainter& p, int totalHeight);
    void drawWaterfall(QPainter& p, const QRect& r);

    void pushWaterfallRow(const QVector<float>& bins, int destWidth);
    QRgb dbmToRgb(float dbm) const;

    QVector<float> m_bins;       // raw FFT frame (dBm)
    QVector<float> m_smoothed;   // exponential-smoothed for visual stability

    double m_centerMhz{14.225};
    double m_bandwidthMhz{0.200};
    double m_sliceFreqMhz{14.225};

    float m_refLevel{0.0f};         // top of display (dBm)
    float m_dynamicRange{120.0f};   // dB range shown in spectrum

    // Waterfall colour range (dBm)
    float m_wfMinDbm{-130.0f};
    float m_wfMaxDbm{-20.0f};

    // Scrolling waterfall image (Format_RGB32)
    QImage m_waterfall;

    static constexpr float SMOOTH_ALPHA = 0.35f;
    // Fraction of widget height reserved for the spectrum line
    static constexpr float SPECTRUM_FRAC = 0.40f;
};

} // namespace AetherSDR
