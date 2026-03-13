#pragma once

#include "models/RadioModel.h"
#include "core/RadioDiscovery.h"
#include "core/AudioEngine.h"

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QProgressBar>
#include <QStatusBar>

namespace AetherSDR {

class FrequencyDial;
class ConnectionPanel;
class SpectrumWidget;
class AppletPanel;
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Radio/connection events
    void onConnectionStateChanged(bool connected);
    void onConnectionError(const QString& msg);
    void onSliceAdded(SliceModel* slice);
    void onSliceRemoved(int id);

    // GUI controls
    void onFrequencyChanged(double mhz);
    void onModeChanged(int index);
    void onVolumeChanged(int value);
    void onMuteToggled(bool muted);
    void onTxToggled(bool tx);

    // Audio
    void onAudioLevel(float rms);

private:
    void buildUI();
    void buildMenuBar();
    void applyDarkTheme();
    void updateSliceControls(SliceModel* s);
    SliceModel* activeSlice() const;

    // Core objects
    RadioDiscovery m_discovery;
    RadioModel     m_radioModel;
    AudioEngine    m_audio;

    // GUI — left sidebar
    ConnectionPanel* m_connPanel{nullptr};

    // GUI — main area
    SpectrumWidget*  m_spectrum{nullptr};
    FrequencyDial*   m_freqDial{nullptr};

    // GUI — right applet panel
    AppletPanel*     m_appletPanel{nullptr};

    // Controls strip
    QComboBox*   m_modeCombo{nullptr};
    QPushButton* m_txButton{nullptr};
    QPushButton* m_muteButton{nullptr};
    QSlider*     m_volumeSlider{nullptr};
    QProgressBar* m_audioLevel{nullptr};
    QLabel*      m_freqLabel{nullptr};

    // Status bar labels
    QLabel* m_connStatusLabel{nullptr};
    QLabel* m_radioInfoLabel{nullptr};

    // Guard: set true while updating controls from the model, so that
    // onFrequencyChanged / onModeChanged don't echo the change back to the radio.
    bool m_updatingFromModel{false};
};

} // namespace AetherSDR
