#pragma once

#include <QWidget>
#include <QStringList>

class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace AetherSDR {

class SliceModel;
class RxApplet;
class SMeterWidget;

// AppletPanel — right-side panel with a row of toggle buttons at the top,
// an S-Meter gauge below them, and a scrollable stack of applets.
// Multiple applets can be visible simultaneously.
class AppletPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppletPanel(QWidget* parent = nullptr);

    void setSlice(SliceModel* slice);
    void setAntennaList(const QStringList& ants);

    RxApplet*     rxApplet()     { return m_rxApplet; }
    SMeterWidget* sMeterWidget() { return m_sMeter; }

private:
    QWidget*      m_sMeterSection{nullptr};
    SMeterWidget* m_sMeter{nullptr};
    RxApplet*    m_rxApplet{nullptr};
    QVBoxLayout* m_stack{nullptr};   // layout inside the scroll area
};

} // namespace AetherSDR
