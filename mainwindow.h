#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QDebug>
#include <QFileDialog>

#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QCloseEvent>
#include <processor.h>



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void setInputFolder(QString path="");
    void setOutputFolder(QString path="");
    void logWarning(QString message);
    void logInfo(QString message);
    void displayImage(QString filename);
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QSettings *settings;
    QString input_folder;
    QString output_folder;

    processor* processor_;
};

#endif // MAINWINDOW_H
