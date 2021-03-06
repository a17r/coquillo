#include <QSortFilterProxyModel>
#include <QCloseEvent>
#include <QProgressBar>
#include <QSettings>
#include <QSignalMapper>
#include <QWidgetAction>

#include "filebrowser/filebrowser.hpp"
#include "processor/renamewidget.hpp"
#include "processor/parserwidget.hpp"
#include "settings/settingsdialog.hpp"
#include "tags/tagsmodel.hpp"
#include "tags/tagdataroles.hpp"
#include "webtags/tagsearchdialog.hpp"

#include "headerdatamodel.hpp"
#include "itemcountlabel.hpp"
#include "mainwindow.hpp"
#include "progresslistener.hpp"
#include "sortpicker.hpp"
#include "stringstoremodel.hpp"
#include "togglewidgetbyaction.hpp"
#include "ui_mainwindow.h"

#include "player.hpp"
#include <QDockWidget>

#include <QDebug>

#include <QTimer>
#include <QDir>

namespace Coquillo {
    MainWindow::MainWindow(Qt::WindowFlags flags)
    : QMainWindow(0, flags) {
        _ui = new Ui::MainWindow;
        _ui->setupUi(this);

        _store = new Tags::Store(this);
        _progress = new ProgressListener(this);
        _model = new Tags::TagsModel(_store, _progress, this);
        _model->setRecursive(QSettings().value("RecursiveScan").toBool());

        setupMainView();
        setupToolBar();
        setupStatusBar();
        setupTagEditor();
        setupFileBrowser();
        setupRenameWidget();
        setupParserWidget();
        setupPlayer();

        // _ui->dockFiles->raise();
        setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::West);

        restoreSettings();

        connect(_store, &Tags::Store::aboutToCommit, this, &MainWindow::preStoreCommit);

        connect(_store, &Tags::Store::committed, [this] {
            _files->blockSignals(true);
            _files->setSelectedDirectories(_model->directories());
            _files->blockSignals(false);
        });
    }

    MainWindow::~MainWindow() {
        delete _ui;
    }

    QMenu * MainWindow::createPopupMenu() {
        auto * menu = QMainWindow::createPopupMenu();
        menu->addSeparator();

        auto * action = menu->addAction(tr("Menubar"));
        action->setCheckable(true);
        action->setChecked(menuBar()->isVisible());

        connect(action, SIGNAL(triggered(bool)), menuBar(), SLOT(setVisible(bool)));

        action = menu->addAction(tr("Statusbar"));
        action->setCheckable(true);
        action->setChecked(statusBar()->isVisible());

        connect(action, SIGNAL(triggered(bool)), statusBar(), SLOT(setVisible(bool)));

        menu->addSeparator();

        action = menu->addAction(tr("Lock toolbars"));
        action->setCheckable(true);
        action->setChecked(!findChild<QToolBar*>()->isMovable());

        connect(action, SIGNAL(triggered(bool)), this, SLOT(setInterfaceLocked(bool)));

        return menu;
    }

    void MainWindow::addPaths(const QStringList & paths) {
        _model->addPaths(paths);
    }

    void MainWindow::sort(int column, Qt::SortOrder order) {
        auto * proxy = qobject_cast<QSortFilterProxyModel*>(_ui->itemView->model());
        proxy->sort(column, order);
    }

    void MainWindow::setupFileBrowser() {
        StringStoreModel * bookmarks = new StringStoreModel("history/bookmarks", 2, this);

        StringStoreModel * path_history = new StringStoreModel("history/directories", this);
        path_history->setLimit(100);

        // _files = new FileBrowser;
        _files = _ui->fileBrowser;
        _files->setBookmarkModel(bookmarks);
        _files->setHistoryModel(path_history);
        _files->setDirectory(QSettings().value("DefaultLocation").toString());
        _files->setRecursive(QSettings().value("RecursiveScan").toBool());
        // _ui->toolBox->addTab(_files, tr("Browse"));

        connect(_files, SIGNAL(recursionEnabled(bool)),
            _model, SLOT(setRecursive(bool)));

        connect(_files, SIGNAL(pathSelected(QString, bool)),
            _model, SLOT(addPath(QString)));

        connect(_files, SIGNAL(pathUnselected(QString, bool)),
            _model, SLOT(removeDirectory(QString)));

        // _ui->dockFiles->setTitleBarWidget(new QWidget);
    }

    void MainWindow::setupMainView() {
        addAction(_ui->actionQuit);
        addAction(_ui->actionMenubar);
        addAction(_ui->actionSelectPrevious);
        addAction(_ui->actionSelectNext);

        auto proxy = new QSortFilterProxyModel(this);
        proxy->setSortRole(Qt::EditRole);
        proxy->setSourceModel(_model);

        _ui->itemView->setModel(proxy);

        _ui->itemView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        _ui->itemView->horizontalHeader()->setSectionsMovable(true);
        _ui->itemView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        _ui->itemView->horizontalHeader()->resizeSection(0, 20);

        connect(_ui->itemView->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(showHeaderContextMenu(QPoint)));

        new ToggleWidgetByAction(menuBar(), _ui->actionMenubar);

        // auto * lock = & _model->operationLock();

        connect(_progress, &ProgressListener::started, [this]{
            _ui->actionAbort->setVisible(true);
            _ui->actionReload->setVisible(false);

            _ui->actionSave->setEnabled(false);
            _ui->actionDiscard->setEnabled(false);
        });

        connect(_progress, &ProgressListener::finished, [this]{
            _ui->actionAbort->setVisible(false);
            _ui->actionReload->setVisible(true);

            _ui->actionSave->setEnabled(true);
            _ui->actionDiscard->setEnabled(true);
        });
    }

    void MainWindow::setupParserWidget() {
        StringStoreModel * history = new StringStoreModel("history/parser", this);

        // _tag_parser = new Processor::ParserWidget(this);
        _tag_parser = _ui->parserWidget;
        _tag_parser->setHistoryModel(history);
        _tag_parser->setModel(_ui->itemView->model());
        _tag_parser->setSelectionModel(_ui->itemView->selectionModel());
        // _ui->toolBox->addTab(_tag_parser, tr("Parse tags"));

        // _ui->dockParseTags->setWidget(_tag_parser);
        // _ui->dockParseTags->setTitleBarWidget(new QWidget);

        // tabifyDockWidget(_ui->dockFiles, _ui->dockParseTags);
    }

    void MainWindow::setupPlayer() {
        connect(_ui->itemView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex & idx) {
            const QString path(idx.data(Tags::FilePathRole).toString());
            _ui->player->playFile(path);
            _ui->player->show();
        });
    }

    void MainWindow::setupRenameWidget() {
        StringStoreModel * history = new StringStoreModel("history/rename", this);

        // _file_rename = new Processor::RenameWidget(this);
        _file_rename = _ui->renameWidget;
        _file_rename->setHistoryModel(history);
        _file_rename->setModel(_ui->itemView->model());
        _file_rename->setSelectionModel(_ui->itemView->selectionModel());
        // _ui->toolBox->addTab(_file_rename, tr("Rename files"));

        // _ui->dockRenameFiles->setWidget(_file_rename);
        // _ui->dockRenameFiles->setTitleBarWidget(new QWidget);

        // tabifyDockWidget(_ui->dockFiles, _ui->dockRenameFiles);
    }

    void MainWindow::setupStatusBar() {
        auto * bar = new QProgressBar(this);
        bar->setFixedWidth(140);
        bar->hide();

        auto * label = new ItemCountLabel(_ui->itemView->model());

        connect(_progress, SIGNAL(started()), label, SLOT(hide()));
        connect(_progress, SIGNAL(finished()), label, SLOT(show()));

        connect(_progress, SIGNAL(started()), bar, SLOT(show()));
        connect(_progress, SIGNAL(finished()), bar, SLOT(hide()));
        connect(_progress, SIGNAL(aborted()), bar, SLOT(hide()));
        connect(_progress, SIGNAL(progress(int)), bar, SLOT(setValue(int)));
        connect(_progress, SIGNAL(rangeChanged(int, int)), bar, SLOT(setRange(int, int)));

        statusBar()->addPermanentWidget(label);
        statusBar()->addPermanentWidget(bar);

        new ToggleWidgetByAction(statusBar(), _ui->actionStatusbar);
    }

    void MainWindow::setupTagEditor() {
        _ui->tagEditor->removeTab(3);
        _ui->tagEditor->setModel(_ui->itemView->model());
        _ui->tagEditor->setSelectionModel(_ui->itemView->selectionModel());
        // _ui->dockEditor->setTitleBarWidget(new QWidget);
    }

    void MainWindow::setupToolBar() {
        connect(_ui->actionDiscard, SIGNAL(triggered()), _model, SLOT(discardChanges()));
        connect(_ui->actionReload, SIGNAL(triggered()), _model, SLOT(reload()));
        connect(_ui->actionDiscard, SIGNAL(triggered()), _model, SLOT(revert()));

        connect(_ui->actionSave, &QAction::triggered, _store, &Tags::Store::commit);

        _sort_picker = new SortPicker(this);

        auto * sort_action = new QWidgetAction(this);
        sort_action->setDefaultWidget(_sort_picker);

        auto * model = new HeaderDataModel(Qt::Horizontal, _sort_picker);
        model->setSourceModel(_model);

        _sort_picker->setModel(model);
        _ui->toolBar->addSeparator();
        _ui->toolBar->addAction(sort_action);

        connect(_sort_picker, SIGNAL(currentIndexChanged(int)), SLOT(sort(int)));

        new ToggleWidgetByAction(_ui->toolBar, _ui->actionToolbar);
    }

    void MainWindow::closeEvent(QCloseEvent * event) {
        abort();
        saveSettings();
        event->accept();
    }

    void MainWindow::abort() {
        _model->abort();
    }

    void MainWindow::openSettingsDialog() {
        Settings::SettingsDialog dialog(this);
        dialog.exec();
    }

    void MainWindow::openTagSearchDialog() {
        if (_ui->actionOpenTagSearch->isChecked()) {
            auto dialog = new WebTags::TagSearchDialog(_ui->itemView->selectionModel(), this);
            // dialog->setModel(_ui->itemView->model());
            // dialog->setSelectedRows(_ui->itemView->selectionModel()->selectedRows(Tags::PathField));

            dialog->setModal(true);
            dialog->show();

            connect(dialog, &QDialog::finished, dialog, [=]{
                dialog->deleteLater();
                _ui->actionOpenTagSearch->setChecked(false);
            });
        }
    }

    void MainWindow::selectAll() {
        QAbstractItemModel * model = _ui->itemView->model();
        const QModelIndex start = model->index(0, 0);
        const QModelIndex end = model->index(model->rowCount() - 1, model->columnCount() - 1);
        _ui->itemView->selectionModel()->select(QItemSelection(start, end), QItemSelectionModel::Select);
    }

    void MainWindow::setInterfaceLocked(bool state) {
        foreach (QToolBar * bar, findChildren<QToolBar*>()) {
            bar->setMovable(!state);
        }
    }

    void MainWindow::showHeaderContextMenu(const QPoint & point) const {
        QHeaderView * header = _ui->itemView->horizontalHeader();
        QSignalMapper mapper;
        QMap<QString, QAction *> labels;

        /*
         * Menu is created as a pointer to allow safe binding of the signal handler onto it.
         * That way we make sure that the listener is destroyed after it is not needed anymore.
         */
        QMenu * menu = new QMenu(header);

        QAction * action = menu->addAction(tr("Show modification indicator"));
        action->setCheckable(true);
        action->setChecked(!header->isSectionHidden(0));
        menu->addSeparator();

        mapper.setMapping(action, 0);
        connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));

        for (int i = 1; i < header->count(); i++) {
            const QString label = header->model()->headerData(i, Qt::Horizontal).toString();
            QAction * action = new QAction(label, menu);
            action->setCheckable(true);
            action->setChecked(!header->isSectionHidden(i));
            mapper.setMapping(action, i);
            labels[label] = action;

            connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
        }

        foreach (QAction * action, labels.values()) {
            menu->addAction(action);
        }

        void(QSignalMapper::* mapped_signal)(int) = &QSignalMapper::mapped;

        menu->connect(&mapper, mapped_signal, [=](int section) {
            header->setSectionHidden(section, !header->isSectionHidden(section));
        });

        menu->exec(header->mapToGlobal(point));
        menu->deleteLater();
    }

    void MainWindow::restoreSettings() {
        const QSettings settings;

        resize(settings.value("UI/Size").toSize());
        restoreState(settings.value("UI/State").toByteArray());
        _ui->splitter->restoreState(settings.value("UI/Splitter").toByteArray());
        // _ui->itemView->horizontalHeader()->restoreState(settings.value("UI/Header").toByteArray());
        // _ui->itemView->horizontalHeader()->setSectionsMovable(true);

        _sort_picker->setCurrentIndex(settings.value("UI/Sorting", Tags::PathField).toInt());

        menuBar()->setVisible(settings.value("UI/MenuBar", true).toBool());
        statusBar()->setVisible(settings.value("UI/StatusBar", true).toBool());

        setInterfaceLocked(settings.value("UI/LockToolBars").toBool());

        if (settings.value("UI/State").isNull()) {
            applyDefaultSettings();
        }

        const QByteArray state = settings.value("UI/Header").toByteArray();

        if (!state.isNull()) {
            /*
             * FIXME: Somewhere there is a bug that prevents restoring the header unless we postpone
             * that with a timer.
             */
            QTimer::singleShot(1, [state, this]{
                _ui->itemView->horizontalHeader()->restoreState(state);
                _ui->itemView->horizontalHeader()->setSectionsMovable(true);
            });
        }
    }

    void MainWindow::saveSettings() {
        QSettings settings;
        settings.setValue("UI/Sorting", _sort_picker->currentIndex());
        settings.setValue("UI/Size", size());
        settings.setValue("UI/State", saveState());
        settings.setValue("UI/Splitter", _ui->splitter->saveState());
        settings.setValue("UI/Header", _ui->itemView->horizontalHeader()->saveState());
        settings.setValue("UI/MenuBar", menuBar()->isVisible());
        settings.setValue("UI/StatusBar", statusBar()->isVisible());
        settings.setValue("UI/LockToolBars", !findChild<QToolBar*>()->isMovable());
        settings.sync();
    }

    void MainWindow::applyDefaultSettings() {
        auto * header = _ui->itemView->horizontalHeader();

        header->setSortIndicator(Tags::PathField, Qt::AscendingOrder);

        for (int i = 0; i < header->count(); i++) {
            if (i != 0 && i != Tags::PathField) {
                header->hideSection(i);
            }
        }
    }

    void MainWindow::on_actionSelectNext_triggered() {
        auto model = _ui->itemView->selectionModel();
        const auto current = model->currentIndex();
        const auto selection = model->selectedRows(current.column());

        if (selection.size() > 1) {
            int pos = (selection.indexOf(current) + 1) % selection.size();
            const auto next = selection[pos];
            model->setCurrentIndex(next, QItemSelectionModel::Rows);
        } else if (model->model()->rowCount() > 0) {
            int pos = (current.row() + 1) % model->model()->rowCount();
            const auto next = model->model()->index(pos, current.column());
            const auto flags = QItemSelectionModel::Rows | QItemSelectionModel::Clear | QItemSelectionModel::Select;
            model->setCurrentIndex(next, flags);
        }
    }

    void MainWindow::on_actionSelectPrevious_triggered() {
        auto model = _ui->itemView->selectionModel();
        const auto current = model->currentIndex();
        const auto selection = model->selectedRows(current.column());

        if (selection.size() > 1) {
            int pos = selection.indexOf(current) - 1;
            int max = selection.size() - 1;
            const auto next = selection[pos < 0 ? max : pos];
            model->setCurrentIndex(next, QItemSelectionModel::Rows);
        } else if (model->model()->rowCount() > 0) {
            int pos = current.row() - 1;
            int max = model->model()->rowCount() - 1;
            const auto next = model->model()->index(pos < 0 ? max : pos, current.column());
            const auto flags = QItemSelectionModel::Rows | QItemSelectionModel::Clear | QItemSelectionModel::Select;
            model->setCurrentIndex(next, flags);
        }
    }

    void MainWindow::preStoreCommit() {
        // if (QSettings().value("DeleteEmptyDirs").toBool()) {
        //     const auto changed = _store->changedItemsMap();
        //     QStringList dirs;
        //
        //     for (const auto & path : changed.keys()) {
        //         dirs << QFileInfo(path).absolutePath();
        //     }
        //
        //     auto * purge = new PurgeDirsAfterCommit(dirs, this);
        //
        //     connect(_store, &Tags::Store::committed, purge, &PurgeDirsAfterCommit::purge);
        //     connect(_store, &Tags::Store::committed, purge, &PurgeDirsAfterCommit::deleteLater);
        // }
    }
}
