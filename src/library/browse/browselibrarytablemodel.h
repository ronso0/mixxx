#pragma once

#include "library/librarytablemodel.h"

class TrackCollection;

class BrowseLibraryTableModel : public LibraryTableModel {
    Q_OBJECT
  public:
    BrowseLibraryTableModel(
            QObject* parent,
            TrackCollectionManager* pTrackCollectionManager,
            const char* settingsNamespace);
    ~BrowseLibraryTableModel() override = default;

    void setPath(const QString& path);
    void search(const QString& searchText, const QString& /* extraFilter */) override;

  private:
    QString m_directoryFilter;
    TrackCollection* m_pTrackCollection;
};
