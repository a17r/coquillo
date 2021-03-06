#ifndef COQUILLO_MAINWINDOW_H
#define COQUILLO_MAINWINDOW_H

#include <QMainWindow>
#include "tags/tagstore.hpp"

namespace Ui {
    class MainWindow;
}

namespace Coquillo {
    namespace Processor {
        class ParserWidget;
        class RenameWidget;
    }

    namespace Tags {
        class TagsModel;
    }

    class FileBrowser;
    class ProgressListener;
    class SortPicker;

    class MainWindow : public QMainWindow {
        Q_OBJECT

        public:
            MainWindow(Qt::WindowFlags flags = Qt::WindowFlags());
            ~MainWindow();
            QMenu * createPopupMenu();

        public slots:
            void addPaths(const QStringList & paths);
            void sort(int column, Qt::SortOrder = Qt::AscendingOrder);

        protected:
            void closeEvent(QCloseEvent * event);

        private slots:
            void abort();
            void openSettingsDialog();
            void openTagSearchDialog();
            void selectAll();
            void setInterfaceLocked(bool state);
            void showHeaderContextMenu(const QPoint & point) const;

            void on_actionSelectPrevious_triggered();
            void on_actionSelectNext_triggered();

            void preStoreCommit();

        private:
            void applyDefaultSettings();
            void setupFileBrowser();
            void setupMainView();
            void setupParserWidget();
            void setupPlayer();
            void setupRenameWidget();
            void setupStatusBar();
            void setupTagEditor();
            void setupToolBar();
            void restoreSettings();
            void saveSettings();

            FileBrowser * _files;
            Tags::TagsModel * _model;
            Processor::ParserWidget * _tag_parser;
            Processor::RenameWidget * _file_rename;
            Ui::MainWindow * _ui;
            ProgressListener * _progress;
            SortPicker * _sort_picker;
            Tags::Store * _store;
    };
}

#endif
