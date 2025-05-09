#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QProcess>
#include <QString>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->CheckVersionButton,&QPushButton::clicked, this, &MainWindow::CheckVersionButton_clicked);
    connect(ui->StartDockerButton,&QPushButton::clicked, this, &MainWindow::StartDockerButton_clicked);
    connect(ui->BuildImageButton,&QPushButton::clicked, this, &MainWindow::BuildImageButton_clicked);
    connect(ui->RunFasthenry,&QPushButton::clicked, this, &MainWindow::RunFasthenry_clicked);
    connect(ui->LoadInpFile,&QPushButton::clicked, this, &MainWindow::LoadInpFile_clicked);
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
    ui->OutputBrowser->insertPlainText("In:"+command+" "+parameters.join(" ")+"\n");
    ui->OutputBrowser->insertPlainText("Out:");
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
    run_command("docker", QStringList()<<"build"<<"-t"<<"fasthenry_image"<<".");
}

void MainWindow::RunFasthenry_clicked()
{
    run_command("docker", QStringList()<<"run"<<"fasthenry_image"<<"/test/run.sh");
}

void MainWindow::LoadInpFile_clicked()
{
    QFileDialog dialog(this);
    QStringList fileNames;
    QFile qfile;

    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("INP File (*.inp)"));
    dialog.setViewMode(QFileDialog::Detail);

    if (dialog.exec())
        fileNames = dialog.selectedFiles();
    ui->OutputBrowser->insertPlainText("Loaded from" + fileNames.first());
    if(qfile.exists("tmp.inp")) qfile.remove("tmp.inp");
    qfile.copy(fileNames.first(),"tmp.inp");
}

void MainWindow::ShowModel_clicked()
{

}
