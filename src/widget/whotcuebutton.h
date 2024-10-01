#pragma once

#include <QString>

#include "track/trackid.h"
#include "util/parented_ptr.h"
#include "widget/wcuemenupopup.h"
#include "widget/wpushbutton.h"

class WHotcueButton : public WPushButton {
    Q_OBJECT

    struct HotcueDragData {
        TrackId trackId = TrackId();
        int hotcue = Cue::kNoHotCue;

        const QByteArray toByteArray() {
            QByteArray bytes;
            QDataStream dataStream(&bytes, QIODevice::WriteOnly);
            qWarning() << "     .";
            qWarning() << "     toByteArray";
            qWarning() << "     --> id: " << trackId.toVariant().toInt();
            qWarning() << "     --> hot:" << hotcue;
            qWarning() << "     .";
            dataStream << trackId.toVariant().toInt() << hotcue;
            return bytes;
        };

        void fromByteArray(QByteArray& bytes) {
            QDataStream stream(&bytes, QIODevice::ReadOnly);
            int id = -1;
            int hot = -1;
            stream >> id >> hot;
            qWarning() << "     .";
            qWarning() << "     fromByteArray";
            qWarning() << "     --> id: " << id;
            qWarning() << "     --> hot:" << hot;
            trackId = TrackId(QVariant::fromValue(id));
            hotcue = hot;
            qWarning() << "     --> data id: " << trackId;
            qWarning() << "     --> data hot:" << hotcue;
            qWarning() << "     .";
        }
    };

  public:
    WHotcueButton(const QString& group, QWidget* pParent);

    void setup(const QDomNode& node, const SkinContext& context) override;

    ConfigKey getLeftClickConfigKey() {
        return createConfigKey(QStringLiteral("activate"));
    }
    ConfigKey getClearConfigKey() {
        return createConfigKey(QStringLiteral("clear"));
    }

    Q_PROPERTY(bool light MEMBER m_bCueColorIsLight);
    Q_PROPERTY(bool dark MEMBER m_bCueColorIsDark);
    Q_PROPERTY(QString type MEMBER m_type);

  protected:
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void mouseMoveEvent(QMouseEvent* pEvent) override;
    void dragEnterEvent(QDragEnterEvent* pEvent) override;
    void dropEvent(QDropEvent* pEvent) override;

    void restyleAndRepaint() override;

  private slots:
    void slotColorChanged(double color);
    void slotTypeChanged(double type);

  private:
    ConfigKey createConfigKey(const QString& name);
    void updateStyleSheet();

    const QString m_group;
    int m_hotcue;
    bool m_hoverCueColor;
    parented_ptr<ControlProxy> m_pCoColor;
    parented_ptr<ControlProxy> m_pCoType;
    parented_ptr<WCueMenuPopup> m_pCueMenuPopup;
    int m_cueColorDimThreshold;
    bool m_bCueColorDimmed;
    bool m_bCueColorIsLight;
    bool m_bCueColorIsDark;
    QString m_type;
    QMargins m_dndRectMargins;
};
