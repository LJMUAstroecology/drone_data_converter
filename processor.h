#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <QObject>
#include <QMessageBox>
#include <QStringList>
#include <QDir>
#include <QTextStream>
#include <QApplication>
#include <QDebug>

#include <opencv2/opencv.hpp>


class processor : public QObject
{
    Q_OBJECT
public:
    explicit processor(QObject *parent = nullptr);

signals:
    void progress(int);
    void logWarning(QString);
    void logInfo(QString);
    void currentImage(QString);
    void position(QString);

public slots:
    void process(void);
    void abort(void);

    void setInputFolder(QString input_folder){
        this->input_folder = input_folder;
    }

    void setOutputFolder(QString output_folder){
        this->output_folder = output_folder;
    }

private:

    void processThermal(void);
    void processRGB(void);
    void processGPS(void);

    bool checkFolder(QDir folder);

    QStringList getFiles(QDir folder, QString glob);

    QByteArray getDataBuffer(QString path);
    QList<int> getOffsets(QString path);

    QList<double> latitude;
    QList<double> longitude;
    QList<double> altitude;

    QStringList thermal_files;
    QStringList rgb_files;
    QStringList thermal_offsets;
    QStringList rgb_offsets;

    int output_fps = 7;

    QString input_folder;
    QString output_folder;

    bool should_quit = false;
};

#endif // PROCESSOR_H
