#ifndef COQUILLO_WEBTAGS_FILTERPROXYMODEL_H
#define COQUILLO_WEBTAGS_FILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace Coquillo {
    namespace WebTags {
        class FilterProxyModel : public QSortFilterProxyModel {
            Q_OBJECT

            public:
                enum FilterMode { HideFiltered, ShowFiltered };
                FilterProxyModel(QObject * parent = 0);
                void addFilter(const QString & value);
                void addFilters(const QStringList & filters);
                void removeFilter(const QString & value);
                void setFilterMode(FilterMode mode);
                inline FilterMode filterMode() const { return _mode; }

//                 QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

            protected:
                bool filterAcceptsRow(int src_row, const QModelIndex & src_parent = QModelIndex()) const;

            private:
                FilterMode _mode;
                QStringList _filtered;
        };
    }
}

#endif
