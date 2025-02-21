#include "searchdialog.h"
#include "ui_searchdialog.h"

SearchDialog::SearchDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SearchDialog)
{
    ui->setupUi(this);

    // setting up actions
    // ------------------
    QObject::connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);
}

SearchDialog::~SearchDialog()
{
    delete ui;

}
