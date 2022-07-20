#include "mbifile.h"
#include <QDebug>


MBI::MBIFile::MBIFile(const QString path, const QString mode, QObject *parent)
    : QObject{parent}
{
    this->path = path;
    if(mode == "r"){
        file_id = H5Fopen(path.toLocal8Bit(), H5F_ACC_RDONLY, H5P_DEFAULT);
    }
    if (file_id == -1) {
        qWarning() << "Failed to open file";
        return;
    }
}

MBI::MBIFile::~MBIFile()
{
    qDebug() << "~MBIFile()";
    this->Close();

    herr_t err = H5Gclose(dataCubes_id);
    if (err < 0) {
        qWarning() << "Error";
    }
    err = H5Gclose(dataDescription_id);
    //err = H5Gclose(dataCubes_id);
    if (err < 0) {
        qWarning() << "Error";
    }

    err = H5Fflush(file_id, H5F_SCOPE_GLOBAL);
    if (err < 0) {
        qWarning() << "Error flush";
    }
    err = H5Fclose(file_id);
    if (err < 0) {
        qWarning() << "Error";
    }
}

void MBI::MBIFile::Close()
{

    if (globalMetadata) {
        globalMetadata->Close();
        globalMetadata.reset();
    }
    //dataCubes.close();
    //dataDescription.close();
    //file.close();
}

QString MBI::MBIFile::getCalMsCalibration()
{
    if (file_id == -1) {
        qWarning() << "Failed to open file";
        return "";
    }
    dataCubes_id = H5Gopen2(file_id, DATA_CUBES, H5P_DEFAULT);
    dataDescription_id = H5Gopen2(file_id, DATA_DESCRIPTION, H5P_DEFAULT);
    hid_t globalMetadataGroup = H5Gopen2(dataDescription_id, GLOBAL_DESCRIPTION, H5P_DEFAULT);
    globalMetadata = std::make_shared<MBI::GlobalMetadata>(globalMetadataGroup);
    const char* calMsTraditional = globalMetadata->ReadString(CAL_MS_TRADITIONAL);
    return QString(calMsTraditional);
}
