#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "searchdialog.h"
#include <QDebug>
#include <QLabel>
#include <QFileDialog>
#include <QTextEdit>
#include <QMessageBox>

/*
- Close/New/Open
    - untitled (empty) => do nothing
    - existing (saved) => do nothing

    - untitled (non-empty) => askToSave -> Save -> [save as]
    - existing (non-saved) => askToSave -> Save
------------------------------------------
- Save
    - untitled (empty/non-empty) => Save As
    - existing (non-saved) => save dialog
------------------------------------------
- Save As (no need to check any state)
    - untitled (empty/non-empty)
    - existing (saved/non-saved)
        =>> save as dialog in all cases
*/


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // setting up window
    // -------------------
    setWindowTitle("Vimmy - untitled[*]");
    // QMainWindow::setWindowIcon(QIcon("./resources/vimmy-logo.png"));
    // QMainWindow::setIconSize(QSize(64, 64));

    // setting up actions & signals/slots
    // -------------------
    connect(ui->newDoc, &QAction::triggered, this, &MainWindow::newDocument);
    connect(ui->open, &QAction::triggered, this, &MainWindow::openDocument);
    connect(ui->save, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(ui->saveAs, &QAction::triggered, this, &MainWindow::saveAsDocument);
    connect(ui->quit, &QAction::triggered, this, &QMainWindow::close);

    connect(ui->search, &QAction::triggered, this, &MainWindow::search);

    connect(ui->editor, &QTextEdit::textChanged, this,
            [this] {
                if (isDocumentUntitled())
                    setFilename("");
                setSavedStatus(false);
    });


    connect(ui->editor, &VimTextEdit::modeChanged,
            ui->mode, &QLabel::setText);

    connect(ui->editor, &VimTextEdit::countChanged,
            ui->count, &QLabel::setText);

    connect(ui->editor, &VimTextEdit::commandChanged,
            ui->command, &QLabel::setText);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openDocument()
{
// - Close/New/Open
//     - untitled (empty) => do nothing
//     - existing (saved) => do nothing

//     - untitled (non-empty) => Save -> [save as]
//     - existing (non-saved) => Save

    if ((isDocumentUntitled() && !isDocumentEmpty()) ||
    (!isDocumentUntitled() && !isDocumentSaved()))
    {
        auto response = askToSave();
        if (response == QMessageBox::Save)
            saveDocument();
        else if (response == QMessageBox::Cancel)
            return;
    }

    

    QString filename = QFileDialog::getOpenFileName(
                                    this,
                                    "Open File",
                                    "", 
                                    tr("Text files (*.txt)"));

    if (filename.isEmpty())
        return;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }

    setFilename(filename);

    QTextStream in(&file);
    ui->editor->setText(in.readAll());

    file.close();
}

bool MainWindow::isDocumentEmpty() const
{
    return ui->editor->isEmpty();
}

void MainWindow::newDocument()
{
// - Close/New/Open
//     - untitled (empty) => do nothing
//     - existing (saved) => do nothing

//     - untitled (non-empty) => Save -> [save as]
//     - existing (non-saved) => Save

    if ((isDocumentUntitled() && !isDocumentEmpty()) ||
    (!isDocumentUntitled() && !isDocumentSaved()))
    {
        auto response = askToSave();
        if (response == QMessageBox::Save)
            saveDocument();
        else if (response == QMessageBox::Cancel)
            return;
    }

    ui->editor->setText(QString());
    setSavedStatus(true);
}

void MainWindow::saveAsDocument()
{
// - Save As (no need to check any state)
//     - untitled (empty/non-empty)
//     - existing (saved/non-saved)
//         =>> save as dialog in all cases
    qDebug() << "in save as";
    QString tempFilename = QFileDialog::getSaveFileName(this,
            "Save As",
            isDocumentUntitled() ? QString() : m_filename,
            "Text files (*.txt)");

    if (tempFilename.isEmpty())
        return;
    setFilename(tempFilename);
    saveDocument();

    setSavedStatus(true);
}

void MainWindow::saveDocument()
{
    // Added Comment
    // kdjf

// - Save
//     - untitled (empty/non-empty) => Save As
//     - existing (non-saved) => save dialog
//     - existing (saved) => do nothing

    if (isDocumentUntitled())
    {
        qDebug() << "in save going to save as";
        saveAsDocument();
        return;
    }
    else if (isDocumentSaved())
        return;

    QFile file(m_filename);
    if (!file.open(QIODevice::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
        return;
    }

    QString text = ui->editor->toPlainText();

    QTextStream out(&file);
    out << text;

    file.close();

    setSavedStatus(true);
}

void MainWindow::search()
{
    SearchDialog dialog;
    dialog.exec();
}

int MainWindow::askToSave()
{
    if (isDocumentSaved())
        return QMessageBox::Discard;

    QMessageBox box(this);
    box.setText("The document has been modified.");
    box.setInformativeText("Do you want to save your changes?");
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    return box.exec();

}
