#ifndef MBIFILE_H
#define MBIFILE_H

#include <QObject>
#include <hdf5.h>
#include <memory>
#include "mbimetadata.h"


namespace MBI
{
    class MBIFile : public QObject
    {
        Q_OBJECT
    public:
        explicit MBIFile(const QString path, const QString mode = "r", QObject *parent = nullptr);

        virtual ~MBIFile();

        static constexpr const char* DATA_DESCRIPTION = "data-description"; ///< Detailed (frame) metadata.
        static constexpr const char* GLOBAL_DESCRIPTION = "global-description"; ///< Global (file) metadata.
        static constexpr const char* DATA_CUBES = "data-cubes"; ///< All datasets.
        static constexpr const char* FRAME_DATA = "frame-%d-data"; ///< The datasets for a single frame p1.
        static constexpr const char* FRAME_METADATA = "frame-%d-metadata"; ///< The metadata for a single frame p1.
        static constexpr const char* FRAME_GROUP_DATA = "frame-group-%d-data"; ///< The datasets for a single frame p1.
        static constexpr const char* FRAME_GROUP_COUNT = "frame-group-count"; ///< The datasets for a single frame p1.
        static constexpr const char* FRAME_GROUP_METADATA = "frame-group-%d-metadata"; ///< The metadata for a single frame p1.
        static constexpr const char* INDEX_COUNTS = "index-counts"; ///< Offsets into the data-counts array for each scan.
        static constexpr const char* INDEX_POSITIONS = "index-positions"; ///< Offsets into the data-positions (gateS) array for each scan.
        static constexpr const char* DATA_COUNTS = "data-counts"; ///< All of the intensity values recorded in a frame.
        static constexpr const char* DATA_POSITIONS = "data-positions"; ///< All of the gate positions recorded in a frame.
        static constexpr const char* TRIGGER_TIMESTAMPS = "trigger-timestamps"; ///< The timestamps (in seconds) of each scan in a frame.
        static constexpr const char* RT_TIC = "rt-tic"; ///< The total ion count for each frame.
        static constexpr const char* AT_TIC = "at-tic"; ///< The total ion count for each scan.
        static constexpr const char* CAL_CSS_TRADITIONAL = "cal-css-traditional"; ///< Traditional (json) ccs coefficients.
        static constexpr const char* CAL_MS_TRADITIONAL = "cal-ms-traditional"; ///< Traditional (json) coefficients.

        /// @brief Close the file
        void Close();

        QString getCalMsCalibration();

    signals:

    private:
        QString path;
        QString mode;
        hid_t file_id = -1;
        hid_t dataCubes_id;  //H5::Group dataCubes; ///< The raw data arrays for each frame.
        hid_t dataDescription_id; //H5::Group dataDescription; ///< The global and frame-level metadata.
        std::shared_ptr<MBI::GlobalMetadata> globalMetadata; ///< File-wide metadata.
    };
}

#endif // MBIFILE_H

