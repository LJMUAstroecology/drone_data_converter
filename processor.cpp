#include "processor.h"

processor::processor(QObject *parent) : QObject(parent)
{
   connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(abort()));
}

void processor::process(void){

    bool res = checkFolder(QDir(input_folder));
    should_quit = false;

    if(!res){
        emit logWarning("Folder check failed.");
        return;
    };

    emit position(QString("Coords: (0, 0) Altitude: 0 m)"));

    processGPS();
    emit progress(0);
    processRGB();
    emit progress(0);
    processThermal();
}

void processor::processRGB(void){

    if(rgb_files.size() == 0) return;

    int frame_count = 0;
    QString video_filename = QString("%1/rgb.avi").arg(output_folder);
    auto fourcc = cv::VideoWriter::fourcc('M','J','P','G');
    cv::VideoWriter* writer = nullptr;

    QDir().mkpath(QString("%1/%2").arg(output_folder).arg("rgb"));
    QFile output_gps(QString("%1/gps_rgb.csv").arg(output_folder));

    output_gps.remove();

    if(!output_gps.open(QIODevice::Append)){
        emit logWarning("Couldn't write RGB GPS file.");
    }

    for(int i=0; i < rgb_files.size(); i++){
        auto data_file = rgb_files[i];
        auto offset_file = rgb_offsets[i];

        auto data_buffer = getDataBuffer(data_file);
        auto offsets = getOffsets(offset_file);

        for(int j=0; j < offsets.size()-1; j++){

            QCoreApplication::processEvents();
            if(should_quit) return;

            auto offset = offsets[j];
            auto output_size = offsets[j+1]-offsets[j];

            if(output_size == 0) continue;

            QString filename = QString("%1/%2/frame_%3.jpg").arg(output_folder).arg("rgb").arg(frame_count, 5, 10, QChar('0'));
            emit logInfo(filename);

            auto split_buffer = data_buffer.mid(offset, output_size);

            QFile output(filename);
            if(output.open(QIODevice::WriteOnly)){
                output.write(split_buffer);
            }
            output.close();

            emit currentImage(filename);

            cv::Mat raw_image;

            try{
                std::vector<char> data(split_buffer.data(), split_buffer.data()+split_buffer.size());
                raw_image = cv::imdecode(data, cv::IMREAD_UNCHANGED);
            }catch(...){
                emit logWarning("Failed to export image");
                continue;
            }

            if(output_gps.isOpen() && latitude.size() >= frame_count && latitude.size() > 0){
                    QTextStream stream( &output_gps );
                    stream << QString("%1, %2, %3, %4\n")
                              .arg(QString("frame_%1.jpg").arg(frame_count, 5, 10, QChar('0')))
                              .arg(latitude[frame_count])
                              .arg(longitude[frame_count])
                              .arg(altitude[frame_count]);

                    emit position(QString("Coords: (%1, %2) Altitude: %3 m").arg(latitude[frame_count])
                                  .arg(longitude[frame_count])
                                  .arg(altitude[frame_count]));
            }

            if(writer == nullptr){
                writer = new cv::VideoWriter(video_filename.toStdString(), fourcc, output_fps, raw_image.size());
                if(!writer->isOpened()){
                    emit logWarning("Couldn't write RGB video.");
                    return;
                }
            }

            if(writer->isOpened()){
                writer->write(raw_image);
            }
            frame_count++;

            double fraction = 100.0 /rgb_files.size();
            double subfraction = fraction*static_cast<double>(j) / offsets.size();

            emit progress(ceil(i*fraction+subfraction));
        }
    }

    if(output_gps.isOpen())
        output_gps.close();

    if(writer != nullptr){
        writer->release();
    }

    emit logInfo("Processed RGB images");

}

QList<int> processor::getOffsets(QString filename){
    QList<int> offsets;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logWarning(QString("Couldn't open offset file: %1").arg(filename));
        return offsets;
    }

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        offsets.append(line.simplified().toInt());
    }

    offsets.append(file.size());
    file.close();

    return offsets;
}

QByteArray processor::getDataBuffer(QString filename){

    QByteArray buffer;

    QFile data(filename);
    if (!data.open(QIODevice::ReadOnly)) {
        emit logWarning(QString("Couldn't open data buffer: %1").arg(filename));
        return buffer;
    }

    buffer = data.readAll();

    data.close();
    return buffer;
}

void processor::processGPS(void){

    QString filename = QString("%1/gps.csv").arg(input_folder);
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logWarning(QString("Couldn't open GPS file: %1").arg(filename));
        return;
    }

    latitude.clear();
    longitude.clear();
    altitude.clear();

    while (!file.atEnd()) {
        QByteArray line = file.readLine().simplified();
        auto coords = line.split(',');

        if(coords.size() >= 3){
            latitude.append(coords[0].simplified().toDouble());
            longitude.append(coords[1].simplified().toDouble());
            altitude.append(coords[2].simplified().toDouble());
        }else{
            latitude.append(0);
            longitude.append(0);
            altitude.append(0);
            emit logWarning(QString("Couldn't parse GPS line: %1").arg(QString(line.simplified())));
            return;
        }
    }

    file.close();

    emit logInfo("Processed GPS file");
}

void processor::processThermal(void){

    if(thermal_files.size() == 0) return;

    int frame_count = 0;
    QString video_filename = QString("%1/thermal.avi").arg(output_folder);
    auto fourcc = cv::VideoWriter::fourcc('M','J','P','G');
    cv::VideoWriter* writer = nullptr;

    QDir().mkpath(QString("%1/%2").arg(output_folder).arg("raw"));
    QDir().mkpath(QString("%1/%2").arg(output_folder).arg("preview"));

    QFile output_gps(QString("%1/gps_thermal.csv").arg(output_folder));
    output_gps.remove();

    if(!output_gps.open(QIODevice::Append)){
        emit logWarning(tr("Couldn't write Thermal GPS file."));
    }

    for(int i=0; i < thermal_files.size(); i++){

        auto data_file = thermal_files[i];
        auto offset_file = thermal_offsets[i];

        auto data_buffer = getDataBuffer(data_file);
        auto offsets = getOffsets(offset_file);

        for(int j=0; j < offsets.size()-1; j++){

            QCoreApplication::processEvents();
            if(should_quit) return;

            auto offset = offsets[j];
            auto output_size = offsets[j+1]-offsets[j];

            if(output_size == 0) continue;

            QString filename = QString("%1/%2/frame_%3.tiff").arg(output_folder).arg("raw").arg(frame_count, 5, 10, QChar('0'));
            auto split_buffer = data_buffer.mid(offset, output_size);
            emit logInfo(filename);

            QFile output(filename);
            if(output.open(QIODevice::WriteOnly)){
                output.write(split_buffer);
            }

            output.close();
            cv::Mat raw_image;

            try {
                std::vector<char> data(split_buffer.data(), split_buffer.data()+split_buffer.size());
                raw_image = cv::imdecode(data, cv::IMREAD_UNCHANGED);
            } catch (...) {
                emit logWarning("Couldn't save thermal image");
                continue;
            }

            if(output_gps.isOpen() && latitude.size() >= frame_count && latitude.size() > 0){
                QTextStream stream( &output_gps );
                stream << QString("%1, %2, %3, %4\n")
                              .arg(QString("frame_%1.tiff").arg(frame_count, 5, 10, QChar('0')))
                              .arg(latitude[frame_count])
                              .arg(longitude[frame_count])
                              .arg(altitude[frame_count]);

                emit position(QString("Coords: (%1, %2) Altitude: %3 m").arg(latitude[frame_count])
                              .arg(longitude[frame_count])
                              .arg(altitude[frame_count]));
            }



            double min=0, max=0;

            cv::minMaxIdx(raw_image, &min, &max);
            double range = max-min;

            cv::Mat preview_frame;
            raw_image.convertTo(preview_frame, CV_32FC1);
            preview_frame -= min;
            preview_frame *= 255.0/range;

            preview_frame.convertTo(preview_frame, CV_8UC1);
            cv::cvtColor(preview_frame, preview_frame, cv::COLOR_GRAY2BGR);

            if(writer == nullptr){
                writer = new cv::VideoWriter(video_filename.toStdString(), fourcc, output_fps, raw_image.size());
                if(!writer->isOpened()){
                    emit logWarning("Failed to open thermal video file.");
                }
            }

            QString preview_filename = QString("%1/%2/frame_%3.tiff").arg(output_folder).arg("preview").arg(frame_count, 5, 10, QChar('0'));

            if(!preview_frame.empty()){
                cv::imwrite(preview_filename.toStdString(), preview_frame);
                emit currentImage(preview_filename);

                if(writer->isOpened()){
                    writer->write(preview_frame);
                }

                frame_count++;
            }

            double fraction = 100.0 /thermal_files.size();
            double subfraction = fraction*static_cast<double>(j) / offsets.size();

            emit progress(ceil(i*fraction+subfraction));
        }
    }

    if(output_gps.isOpen())
        output_gps.close();

    if(writer != nullptr){
        writer->release();
    }

    emit logInfo("Processed thermal images");
}

 QStringList processor::getFiles(QDir folder, QString glob){
    QStringList filters;
    filters << glob;
    folder.setNameFilters(filters);
    auto entries = folder.entryInfoList();

    QStringList paths;
    for(auto &entry : entries){
        paths.append(entry.absoluteFilePath());
    }

    return paths;
}

bool processor::checkFolder(QDir folder){
    // Check that gps exists

    if(!folder.exists("gps.csv")){
        logWarning(tr("No gps.csv found"));
    }

    thermal_files.clear();
    rgb_files.clear();
    thermal_offsets.clear();
    rgb_offsets.clear();

    // Check that
    thermal_files = getFiles(input_folder, "thermal*.dat");
    rgb_files = getFiles(input_folder, "rgb*.dat");
    thermal_offsets = getFiles(input_folder, "thermal_offset*.txt");
    rgb_offsets = getFiles(input_folder, "rgb_offset*.txt");

    if(thermal_files.size() == 0 && rgb_files.size() == 0){
        logWarning(tr("No data files found."));
        return false;
    }else{
        if(thermal_files.size() != thermal_offsets.size()){
            logWarning(tr("Different number of thermal data and offset files."));
            return false;
        }

        if(rgb_files.size() != rgb_offsets.size()){
            logWarning(tr("Different number of RGB data and offset files."));
            return false;
        }

    }

    return true;
}

void processor::abort(){
    should_quit = true;
    logWarning(tr("Canceled"));
    emit progress(0);
    emit position(QString("Coords: (0, 0) Altitude: 0 m)"));
}
