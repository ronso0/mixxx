#include "preferences/dialog/dlgpreferencepage.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLayout>
#include <QSlider>
#include <QSpinBox>

#include "moc_dlgpreferencepage.cpp"

DlgPreferencePage::DlgPreferencePage(QWidget* pParent)
        : QWidget(pParent) {
}

DlgPreferencePage::~DlgPreferencePage() {
}

QUrl DlgPreferencePage::helpUrl() const {
    return QUrl();
}

void DlgPreferencePage::setScrollSafeGuardForAllInputWidgets(QObject* obj) {
    // Set the focus policy to for scrollable input widgets and connect them
    // to the custom event filter
    for (auto* ch : obj->children()) {
        // children() does not descend into QGroupBox,
        // so we need to do it manually
        QGroupBox* gBox = qobject_cast<QGroupBox*>(ch);
        if (gBox) {
            setScrollSafeGuardForAllInputWidgets(gBox);
            continue;
        }

        QComboBox* combo = qobject_cast<QComboBox*>(ch);
        if (combo) {
            setScrollSafeGuard(combo);
            continue;
        }
        QSpinBox* spin = qobject_cast<QSpinBox*>(ch);
        if (spin) {
            setScrollSafeGuard(spin);
            continue;
        }
        QDoubleSpinBox* spinDouble = qobject_cast<QDoubleSpinBox*>(ch);
        if (spinDouble) {
            setScrollSafeGuard(spinDouble);
            continue;
        }
        QSlider* slider = qobject_cast<QSlider*>(ch);
        if (slider) {
            setScrollSafeGuard(slider);
            continue;
        }
        // workaround for Qt bug present in 6.2.3 (fixed in 6.5)
        // Required to avoid scroll events getting swallowed/ignored when cursor
        // is above an inactive (disabled) checkbox.
        QCheckBox* cbox = qobject_cast<QCheckBox*>(ch);
        if (cbox) {
            setScrollSafeGuard(cbox);
            continue;
        }
    }
}

void DlgPreferencePage::setScrollSafeGuard(QWidget* pWidget) {
    pWidget->setFocusPolicy(Qt::StrongFocus);
    pWidget->installEventFilter(this);
}

bool DlgPreferencePage::eventFilter(QObject* obj, QEvent* e) {
    if (e->type() == QEvent::Wheel) {
        // Reject scrolling only if widget is unfocused.
        // Object to widget cast is needed to check the focus state.
        QWidget* wid = qobject_cast<QWidget*>(obj);
        if (wid && !wid->hasFocus()) {
            QApplication::sendEvent(qobject_cast<QObject*>(layout()), e);
            return true;
        }
    }
    return QObject::eventFilter(obj, e);
}
