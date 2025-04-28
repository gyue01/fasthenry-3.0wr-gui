#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QProcess>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_CheckVersionButton_clicked()
{
    QProcess process;
    QString command = "docker";
    QStringList parameters = QStringList() << "-v";

    process.start(command, parameters);
    process.waitForFinished(-1); // Wait indefinitely for the process to finish
    QString output = "Output:\n"+process.readAllStandardOutput()
                     +"Error:\n"+process.readAllStandardError();

    ui->InputBrowser->setText(command+" "+parameters.join(" "));
    ui->OutputBrowser->setText(output);
}


void MainWindow::on_CheckVersionButton_2_clicked()
{
    QProcess process;
    QString command = "docker";
    QStringList parameters = QStringList()<<"desktop"<<"start";

    process.start(command, parameters);
    process.waitForFinished(-1); // Wait indefinitely for the process to finish
    QString output = "Output:\n"+process.readAllStandardOutput()
                     +"Error:\n"+process.readAllStandardError();

    ui->InputBrowser->setText(command+" "+parameters.join(" "));
    ui->OutputBrowser->setText(output);
}

