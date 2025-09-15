#include "library/browse/browselibrarytablemodel.h"

#include "library/dao/trackschema.h"
#include "library/queryutil.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "moc_browselibrarytablemodel.cpp"

BrowseLibraryTableModel::BrowseLibraryTableModel(
        QObject* pParent,
        TrackCollectionManager* pTrackCollectionManager,
        const char* settingsNamespace)
        : LibraryTableModel(pParent,
                  pTrackCollectionManager,
                  settingsNamespace),
          m_pTrackCollection(pTrackCollectionManager->internalCollection()) {
}

void BrowseLibraryTableModel::setPath(const QString& path) {
    FieldEscaper escaper(m_pTrackCollection->database());
    // Note: don't use leading or trailing '%' for the LIKE pattern. We only want
    // tracks from exactly that path, not from it's children or any other path
    // that coincidentally contains 'path'.
    const QString escapedDirPath = escaper.escapeString(path);
    m_directoryFilter = QStringLiteral(
            "%1 IN (SELECT %2.%1 "
            "FROM %2 JOIN %3 "
            "ON %2.%4=%3.id "
            "WHERE %3.%5 LIKE %6)")
                                .arg(LIBRARYTABLE_ID,
                                        LIBRARY_TABLE,
                                        TRACKLOCATIONS_TABLE,
                                        LIBRARYTABLE_LOCATION,
                                        TRACKLOCATIONSTABLE_DIRECTORY,
                                        escapedDirPath);
}

void BrowseLibraryTableModel::search(const QString& searchText, const QString& /* extraFilter */) {
    LibraryTableModel::search(searchText, m_directoryFilter);
}
