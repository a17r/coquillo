
#include <QCloseEvent>
#include <QDebug>
#include <QItemSelectionModel>
#include <QPointer>
#include <QSettings>
#include <QSignalMapper>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "stringstoremodel.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "filebrowser/filebrowser.h"
#include "metadata/metadatamodel.h"
#include "processor/renamewidget.h"
#include "processor/parserwidget.h"
#include "tageditor/basictags.h"

namespace Coquillo {
    MainWindow::MainWindow(QWidget * parent)
    : QMainWindow(parent) {
        _ui = new Ui::MainWindow;
        _ui->setupUi(this);

        _metaData = new MetaDataModel(this);

        QSortFilterProxyModel * sort_proxy = new QSortFilterProxyModel(this);
        sort_proxy->setSourceModel(_metaData);
        _ui->metaData->setModel(sort_proxy);

        QSettings * storage = new QSettings("history");
        StringStoreModel * bookmarks = new StringStoreModel("bookmarks", 2, this);
        bookmarks->setStorage(storage);

        _fileBrowser = new FileBrowser(this);
        _fileBrowser->setBookmarkModel(bookmarks);
        _ui->tools->addTab(_fileBrowser, tr("Directories"));

        StringStoreModel * rename_history = new StringStoreModel("rename", this);
        rename_history->setStorage(storage);

        StringStoreModel * parser_history = new StringStoreModel("parser", this);
        parser_history->setStorage(storage);

        _fileRenamer = new Processor::RenameWidget(this);
        _fileRenamer->setHistoryModel(rename_history);
        _fileRenamer->setModel(_ui->metaData->model());
        _fileRenamer->setSelectionModel(_ui->metaData->selectionModel());
        _ui->tools->addTab(_fileRenamer, tr("Rename files"));

        _tagParser = new Processor::ParserWidget(this);
        _tagParser->setHistoryModel(parser_history);
        _tagParser->setModel(_ui->metaData->model());
        _tagParser->setSelectionModel(_ui->metaData->selectionModel());
        _ui->tools->addTab(_tagParser, tr("Parse tags"));

        _basicTags = new TagEditor::BasicTags(this);
        _basicTags->setModel(_ui->metaData->model());
        _ui->tagEditor->addTab(_basicTags, tr("Tags"));

        connect(_fileBrowser, SIGNAL(recursionEnabled(bool)),
            _metaData, SLOT(setRecursive(bool)));

        connect(_fileBrowser, SIGNAL(pathSelected(QString, bool)),
            _metaData, SLOT(addDirectory(QString)));

        connect(_fileBrowser, SIGNAL(pathUnselected(QString, bool)),
            _metaData, SLOT(removeDirectory(QString)));

        connect(_ui->metaData->selectionModel(), SIGNAL(currentRowChanged(QModelIndex, QModelIndex)),
            _basicTags, SLOT(setCurrentIndex(QModelIndex)));

        connect(_basicTags, SIGNAL(cloneValue(QVariant, int)), SLOT(applyValue(QVariant, int)));
        connect(_ui->actionReset, SIGNAL(triggered()), _metaData, SLOT(revert()));
        connect(_ui->actionReload, SIGNAL(triggered()), _metaData, SLOT(reload()));

        _ui->metaData->header()->setSectionResizeMode(0, QHeaderView::Fixed);
        _ui->metaData->header()->resizeSection(0, 20);
        _ui->metaData->header()->setSortIndicator(1, Qt::AscendingOrder);
        _ui->metaData->header()->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(_ui->metaData->header(), SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(showHeaderContextMenu(QPoint)));

        QTimer::singleShot(1, this, SLOT(restoreSettings()));
    }

    MainWindow::~MainWindow() {
        delete _ui;
    }

    QMenu * MainWindow::createPopupMenu() {
        QMenu * menu = QMainWindow::createPopupMenu();
        menu->addSeparator();
        QAction * action;

        action = menu->addAction(tr("Menu Bar"));
        action->setCheckable(true);
        action->setChecked(menuBar()->isVisible());
        connect(action, SIGNAL(triggered(bool)), menuBar(), SLOT(setVisible(bool)));

        action = menu->addAction(tr("Status Bar"));
        action->setCheckable(true);
        action->setChecked(statusBar()->isVisible());
        connect(action, SIGNAL(triggered(bool)), statusBar(), SLOT(setVisible(bool)));

        return menu;
    }

    void MainWindow::closeEvent(QCloseEvent * event) {
        saveSettings();
        event->accept();
    }

    void MainWindow::applyValue(const QVariant & value, int column) {
        foreach (QModelIndex idx, _ui->metaData->selectionModel()->selectedRows(column)) {
            const_cast<QAbstractItemModel*>(idx.model())->setData(idx, value);
        }
    }

    void MainWindow::invertSelection() {
        QAbstractItemModel * model = _ui->metaData->model();
        QItemSelectionModel * selection_model = _ui->metaData->selectionModel();
        const QModelIndex tl = model->index(0, 0);
        const QModelIndex br = model->index(model->rowCount() - 1, model->columnCount() - 1);
        selection_model->select(QItemSelection(tl, br), QItemSelectionModel::Toggle);

        // Make sure the active row stays within the selection so that the
        // tag editor widget won't fall out of sync.
        selection_model->setCurrentIndex(selection_model->selection().indexes().first(), QItemSelectionModel::NoUpdate);
    }

    void MainWindow::selectAll() {
        QAbstractItemModel * model = _ui->metaData->model();
        const QModelIndex tl = model->index(0, 0);
        const QModelIndex br = model->index(model->rowCount() - 1, model->columnCount() - 1);
        _ui->metaData->selectionModel()->select(QItemSelection(tl, br), QItemSelectionModel::Select);
    }

    void MainWindow::showHeaderContextMenu(const QPoint & point) const {
        QHeaderView * header = _ui->metaData->header();
        QMenu menu(header);
        QSignalMapper mapper;
        QMap<QString, QAction *> labels;

        QAction * action = menu.addAction(tr("Show modification indicator"));
        action->setCheckable(true);
        action->setChecked(!header->isSectionHidden(0));
        menu.addSeparator();

        mapper.setMapping(action, 0);
        connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));

        for (int i = 1; i < header->count(); i++) {
            const QString label = header->model()->headerData(i, Qt::Horizontal).toString();
            QAction * action = new QAction(label, &menu);
            action->setCheckable(true);
            action->setChecked(!header->isSectionHidden(i));
            mapper.setMapping(action, i);
            labels[label] = action;

            connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
        }

        foreach (QAction * action, labels.values()) {
            menu.addAction(action);
        }

        void(QSignalMapper::* mapped_signal)(int) = &QSignalMapper::mapped;

        connect(&mapper, mapped_signal, [=](int section) {
            header->setSectionHidden(section, !header->isSectionHidden(section));
        });

        menu.exec(header->mapToGlobal(point));
    }

    void MainWindow::restoreSettings() {
        QSettings settings;
        restoreState(settings.value("UI/MainWindow/State").toByteArray());
        resize(settings.value("UI/MainWindow/Size").toSize());
        menuBar()->setVisible(settings.value("UI/MainWindow/MenuBar", true).toBool());
        statusBar()->setVisible(settings.value("UI/MainWindow/StatusBar", true).toBool());
        _ui->splitter->restoreState(settings.value("UI/MainSplitter/State").toByteArray());
        _ui->metaData->header()->restoreState(settings.value("UI/MainHeader/State").toByteArray());
        _fileBrowser->setRecursive(settings.value("RecursiveScan").toBool());
    }

    void MainWindow::saveSettings() {
        QSettings settings;
        settings.setValue("RecursiveScan", _fileBrowser->isRecursive());
        settings.setValue("UI/MainWindow/State", saveState());
        settings.setValue("UI/MainWindow/Size", size());
        settings.setValue("UI/MainWindow/MenuBar", menuBar()->isVisible());
        settings.setValue("UI/MainWindow/StatusBar", statusBar()->isVisible());
        settings.setValue("UI/MainSplitter/State", _ui->splitter->saveState());
        settings.setValue("UI/MainHeader/State", _ui->metaData->header()->saveState());
    }
}
