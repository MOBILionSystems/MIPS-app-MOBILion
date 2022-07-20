#ifndef MBIMETADATA_H
#define MBIMETADATA_H
#include <hdf5.h>
#include <string>
#include <map>

namespace MBI
{
    /*! @class MBI::IMetadata
    *   @brief Abstract interface for metadata from an MBI file.
    *   @author Greg Van Aken
    */
    class IMetadata
    {
    public:
        /// @brief Initialize an empty metadata object.
        IMetadata();

        /// @brief Initialize metadata from a group.
        //IMetadata(H5::Group group);

        IMetadata(hid_t& group);
        /// @brief Initialize metadata from another metadata object.
        void Copy(IMetadata& toCopy);

        /// @brief Read value at the requested key as a const char* (string).
        /// @param key a const char* key to lookup.
        const char* ReadString(const char* key);

        /// @brief Read a value at the requested key as a const char*.
        /// @param key a std::string key to lookup.
        const char* ReadString(std::string key);

        /// @brief Read value at the requested key as an integer.
        /// @param key a const char* key to lookup.
        const int ReadInt(const char* key);

        /// @brief Read value at the requested key as a double float.
        /// @param key a const char* key to lookup.
        const double ReadDouble(const char* key);

        /// @brief Read all metadata key, value pairs into cache.
        void LoadAll();

        /// @brief Read all metadata from the fime
        const std::map<std::string, std::string>& ReadAll();

        /// @brief Retrieve all cached metadata.
        const std::map<std::string, std::string>& GetCache();

        /// @brief Overwrite all cached metadata.
        void SetCache(const std::map<std::string, std::string>& toCopy);

        /// @brief Add key,val pair to metadata.
        /// @param key the key to add.
        /// @param val the value to add to that key.
        void AddMetadata(const char* key, const char* val);
        void AddMetadata(const char* key, std::string val);
        /// @brief Write any cached key,value pairs as attributes
        void Write();

        /// @brief Close file metadata
        void Close();

        ~IMetadata();

    private:
        hid_t group;
        hid_t group_global;
        //H5::Group group;
        std::map<std::string, std::string> cache;
    };

    /*! @class MBI::GlobalMetadata
    *   @brief Metadata global to the MBI file.
    *   @author Greg Van Aken
    */
    class GlobalMetadata : public IMetadata
    {
    public:
        GlobalMetadata() { };
        GlobalMetadata(hid_t& group) : IMetadata(group) {
        }
    };

    /*! @class MBI::FrameMetadata
    *   @brief Metadata specific to a single frame.
    *   @author Greg Van Aken
    */
    class FrameMetadata : public IMetadata
    {
    public:
        FrameMetadata() { };
        FrameMetadata(hid_t& group) : IMetadata(group) {
        }
    };

    /*! @class MBI::FragmentationMetadata
    *   @brief Metadata pertinent to fragmentation
    *   @author Greg Van Aken
    */
    class FragmentationMetadata
    {
    public:
        /*! @enum MBI::FragmentationMetadata::eType
        *   @brief The types of fragmentation data for a single frame
        *   @author Greg Van Aken
        */
        enum class eType
        {
            NONE,		///< No fragmentation data present
            HILO		///< Alternating high CE, low CE frames
        };

        /// @brief Initialize a fragmentation metadata object.
        FragmentationMetadata();

        eType type; ///< Type of fragmentation used to collect the data.
        double fragEnergy; ///< Fragmentation energy of the frame.
    };
}

#endif // MBIMETADATA_H
