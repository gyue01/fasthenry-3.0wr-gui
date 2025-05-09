#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QProcess *commandline;
    void run_command(QString command, QStringList parameters);


private slots:
    void StartDockerButton_clicked();
    void CheckVersionButton_clicked();
    void BuildImageButton_clicked();
    void CommandOutputReady();
    void RunFasthenry_clicked();
    void LoadInpFile_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
