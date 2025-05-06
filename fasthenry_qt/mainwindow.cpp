#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QProcess>
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->CheckVersionButton,&QPushButton::clicked, this, &MainWindow::CheckVersionButton_clicked);
    connect(ui->StartDockerButton,&QPushButton::clicked, this, &MainWindow::StartDockerButton_clicked);
    connect(ui->BuildImageButton,&QPushButton::clicked, this, &MainWindow::BuildImageButton_clicked);
    commandline = new QProcess(this);
    commandline->setProcessChannelMode(QProcess::MergedChannels);
    connect(commandline, &QProcess::readyReadStandardOutput,this, &MainWindow::CommandOutputReady);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete commandline;
}

void MainWindow::CommandOutputReady()
{
    ui->OutputBrowser->insertPlainText(commandline->readAllStandardOutput());
}

void MainWindow::run_command(QString command, QStringList parameters)
{
    ui->InputBrowser->setText(command+" "+parameters.join(" "));
    commandline->start(command, parameters);
}

void MainWindow::CheckVersionButton_clicked()
{
    run_command("docker", QStringList() << "-v");
}


void MainWindow::StartDockerButton_clicked()
{
    run_command("docker", QStringList()<<"desktop"<<"start");
}


void MainWindow::BuildImageButton_clicked()
{
    run_command("cmd", QStringList()<<"/c"<<"cd");
}

