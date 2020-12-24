#include "controllers/dlgprefcontrollers.h"

#include <QDesktopServices>

#include "controllers/controllermanager.h"
#include "controllers/defs_controllers.h"
#include "controllers/dlgprefcontroller.h"
#include "defs_urls.h"
#include "moc_dlgprefcontrollers.cpp"
#include "preferences/dialog/dlgpreferences.h"

DlgPrefControllers::DlgPrefControllers(DlgPreferences* pPreferences,
        UserSettingsPointer pConfig,
        ControllerManager* pControllerManager,
        QTreeWidgetItem* pControllersRootItem)
        : DlgPreferencePage(pPreferences),
          m_pDlgPreferences(pPreferences),
          m_pConfig(pConfig),
          m_pControllerManager(pControllerManager),
          m_pControllersRootItem(pControllersRootItem) {
    setupUi(this);
    setupControllerWidgets();

    const QString presetsPath = userPresetsPath(m_pConfig);
    connect(btnOpenUserPresets,
            &QPushButton::clicked,
            this,
            [this, presetsPath] { slotOpenLocalFile(presetsPath); });

    // Connections
    connect(m_pControllerManager,
            &ControllerManager::devicesChanged,
            this,
            &DlgPrefControllers::rescanControllers);
}

DlgPrefControllers::~DlgPrefControllers() {
    destroyControllerWidgets();
}

void DlgPrefControllers::slotOpenLocalFile(const QString& file) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(file));
}

void DlgPrefControllers::slotUpdate() {
    for (DlgPrefController* controllerDlg : qAsConst(m_controllerPages)) {
        controllerDlg->slotUpdate();
    }
}

void DlgPrefControllers::slotCancel() {
    for (DlgPrefController* controllerDlg : qAsConst(m_controllerPages)) {
        controllerDlg->slotCancel();
    }
}

void DlgPrefControllers::slotApply() {
    for (DlgPrefController* controllerDlg : qAsConst(m_controllerPages)) {
        controllerDlg->slotApply();
    }
}

void DlgPrefControllers::slotResetToDefaults() {
    for (DlgPrefController* controllerDlg : qAsConst(m_controllerPages)) {
        controllerDlg->slotResetToDefaults();
    }
}

QUrl DlgPrefControllers::helpUrl() const {
    return QUrl(MIXXX_MANUAL_CONTROLLERS_URL);
}

bool DlgPrefControllers::handleTreeItemClick(QTreeWidgetItem* clickedItem) {
    int controllerIndex = m_controllerTreeItems.indexOf(clickedItem);
    if (controllerIndex >= 0) {
        DlgPrefController* controllerDlg = m_controllerPages.value(controllerIndex);
        if (controllerDlg) {
            m_pDlgPreferences->switchToPage(controllerDlg);
        }
        return true;
    } else if (clickedItem == m_pControllersRootItem) {
        // Switch to the root page and expand the controllers tree item.
        m_pDlgPreferences->expandTreeItem(clickedItem);
        m_pDlgPreferences->switchToPage(this);
        return true;
    }
    return false;
}

void DlgPrefControllers::rescanControllers() {
    destroyControllerWidgets();
    setupControllerWidgets();
}

void DlgPrefControllers::destroyControllerWidgets() {
    while (!m_controllerPages.isEmpty()) {
        DlgPrefController* controllerDlg = m_controllerPages.takeLast();
        m_pDlgPreferences->removePageWidget(controllerDlg);
        delete controllerDlg;
    }

    m_controllerTreeItems.clear();
    while (m_pControllersRootItem->childCount() > 0) {
        QTreeWidgetItem* controllerTreeItem = m_pControllersRootItem->takeChild(0);
        delete controllerTreeItem;
    }
}

void DlgPrefControllers::setupControllerWidgets() {
    // For each controller, create a dialog and put a little link to it in the
    // treepane on the left.
    QList<Controller*> controllerList =
            m_pControllerManager->getControllerList(false, true);

    std::sort(controllerList.begin(), controllerList.end(), controllerCompare);

    foreach (Controller* pController, controllerList) {
        DlgPrefController* controllerDlg = new DlgPrefController(
                this, pController, m_pControllerManager, m_pConfig);
        connect(controllerDlg,
                &DlgPrefController::mappingStarted,
                m_pDlgPreferences,
                &DlgPreferences::hide);
        connect(controllerDlg,
                &DlgPrefController::mappingEnded,
                m_pDlgPreferences,
                &DlgPreferences::show);

        m_controllerPages.append(controllerDlg);

        connect(pController,
                &Controller::openChanged,
                [this, controllerDlg](bool bOpen) {
                    slotHighlightDevice(controllerDlg, bOpen);
                });

        QTreeWidgetItem* controllerTreeItem = new QTreeWidgetItem(
                QTreeWidgetItem::Type);
        m_pDlgPreferences->addPageWidget(controllerDlg,
                controllerTreeItem,
                pController->getName(),
                "controllers.svg");

        m_pControllersRootItem->addChild(controllerTreeItem);
        m_controllerTreeItems.append(controllerTreeItem);

        // If controller is open make controller label bold
        QFont temp = controllerTreeItem->font(0);
        temp.setBold(pController->isOpen());
        controllerTreeItem->setFont(0, temp);
    }

    // If no controllers are available, show the "No controllers available" message.
    txtNoControllersAvailable->setVisible(controllerList.empty());
}

void DlgPrefControllers::slotHighlightDevice(DlgPrefController* controllerDlg, bool enabled) {
    int controllerPageIndex = m_controllerPages.indexOf(controllerDlg);
    if (controllerPageIndex < 0) {
        return;
    }

    QTreeWidgetItem* controllerTreeItem =
            m_controllerTreeItems.at(controllerPageIndex);
    if (!controllerTreeItem) {
        return;
    }

    QFont temp = controllerTreeItem->font(0);
    temp.setBold(enabled);
    controllerTreeItem->setFont(0, temp);
}
