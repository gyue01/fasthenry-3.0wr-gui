#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QProcess>
#include <QString>
#include <QFileDialog>
#include <QPixmap>
#include <QDebug>

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
    connect(ui->ShowResult,&QPushButton::clicked, this, &MainWindow::ShowResult_clicked);
    connect(ui->ShowModel,&QPushButton::clicked, this, &MainWindow::ShowModel_clicked);
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
    commandline->waitForFinished(-1);
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
    QProcess container;
    QFile file;

    if(file.exists("Zc.mat")) file.remove("Zc.mat");
    if(file.exists("zbuffile.pdf")) file.remove("zbuffile.pdf");
    container.start("docker", QStringList()<<"run"<<"--name"<<"fasthenry_container"<<"fasthenry_image"<<"/test/run.sh");
    container.waitForFinished(3000);
    ui->OutputBrowser->insertPlainText(container.readAllStandardOutput());
    run_command("docker", QStringList()<<"cp"<<"fasthenry_container:/zbuffile.pdf"<<".");
    run_command("docker", QStringList()<<"cp"<<"fasthenry_container:/Zc.mat"<<".");
    run_command("docker", QStringList()<<"cp"<<"fasthenry_container:/zbuffile-1.png"<<".");
    run_command("docker", QStringList()<<"kill"<<"fasthenry_container");
    run_command("docker", QStringList()<<"rm"<<"fasthenry_container");
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

void MainWindow::ShowResult_clicked()
{
    QFile file("Zc.mat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        ui->Output_Zc->insertPlainText(line+"\n");
    }
}

void MainWindow::ShowModel_clicked()
{
    QPixmap mypix("zbuffile-1.png");
    if (! ui->graphicsView->scene()) {
        qDebug() << "No Scene!";

        QGraphicsScene *scene = new QGraphicsScene(this);
        ui->graphicsView->setScene(scene);
    }
    ui->graphicsView->scene()->addPixmap(mypix);
    ui->graphicsView->fitInView(ui->graphicsView->sceneRect());
}
