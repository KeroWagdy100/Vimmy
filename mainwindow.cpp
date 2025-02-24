#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "searchdialog.h"
#include <QDebug>
#include <QLabel>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // setting up window
    // -------------------
    setWindowTitle("Editor[*]");

    // setting up actions & signals/slots
    // -------------------
    connect(ui->quit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->save, &QAction::triggered, this, &MainWindow::save);
    connect(ui->search, &QAction::triggered, this, &MainWindow::search);

    connect(ui->editor, &QTextEdit::textChanged, this,
            [this] {
                if (!isWindowModified())
                    setWindowModified(true);
    });

    connect(ui->editor, &VimTextEdit::modeChanged,
            ui->mode, &QLabel::setText);

    connect(ui->editor, &VimTextEdit::countChanged,
            ui->count, &QLabel::setText);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::save()
{
    qDebug() << "Saving " << ui->editor->toPlainText();
}

void MainWindow::search()
{
    SearchDialog dialog;
    dialog.exec();
}
