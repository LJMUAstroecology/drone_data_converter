#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    settings = new QSettings("AstroEcology", "ThermalConverter");
    auto prev_folder = settings->value("last_folder").toString();

    connect(ui->inputFolderLineEdit, SIGNAL(textEdited(QString)), SLOT(setInputFolder(QString)));
    connect(ui->selectInputFolderButton, SIGNAL(clicked()), this, SLOT(setInputFolder()));
    connect(ui->outputFolderLineEdit, SIGNAL(textEdited(QString)), SLOT(setOutputFolder(QString)));
    connect(ui->selectOutputFolderButton, SIGNAL(clicked()), this, SLOT(setOutputFolder()));

    processor_ = new processor(this);

    connect(ui->convertButton, SIGNAL(clicked()), processor_, SLOT(process()));
    connect(processor_, SIGNAL(logWarning(QString)), this, SLOT(logWarning(QString)));
    connect(processor_, SIGNAL(logInfo(QString)), this, SLOT(logInfo(QString)));
    connect(processor_, SIGNAL(progress(int)), ui->progressBar, SLOT(setValue(int)));
    connect(ui->cancelButton, SIGNAL(clicked()), processor_, SLOT(abort()));
    connect(processor_, SIGNAL(currentImage(QString)), this, SLOT(displayImage(QString)));
    connect(processor_, SIGNAL(position(QString)), ui->positionLabel, SLOT(setText(QString)));

    if(prev_folder != ""){
         setInputFolder(prev_folder);
    }

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    processor_->abort();
    event->accept();
}

void MainWindow::displayImage(QString filename){
    QImage image(filename);
    ui->imageDisplay->setPixmap(QPixmap::fromImage(image.scaledToWidth(ui->imageDisplay->width())));
}

void MainWindow::setInputFolder(QString dirName){

    if(dirName == ""){
        dirName = QFileDialog::getExistingDirectory(this, "Output folder", input_folder);
    }

    if(dirName != ""){
        input_folder = QDir::cleanPath(dirName);
        settings->setValue("last_folder", input_folder);
    }

    ui->inputFolderLineEdit->setText(input_folder);
    setOutputFolder(input_folder);

    if(processor_ != nullptr)
        processor_->setInputFolder(input_folder);

    return;

}

void MainWindow::setOutputFolder(QString dirName){
    if(dirName == ""){
        dirName = QFileDialog::getExistingDirectory(this, "Output folder", input_folder);
    }

    if(dirName != ""){
        output_folder = QDir::cleanPath(dirName);
    }

    ui->outputFolderLineEdit->setText(output_folder);

    if(processor_ != nullptr)
        processor_->setOutputFolder(output_folder);

    return;
}

void MainWindow::logWarning(QString message){
   ui->logger->setTextColor(Qt::red);
   ui->logger->append(message);
}

void MainWindow::logInfo(QString message){
   ui->logger->setTextColor(Qt::green);
   ui->logger->append(message);
}

MainWindow::~MainWindow()
{
    delete ui;
}
