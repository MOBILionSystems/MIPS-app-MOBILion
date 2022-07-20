#include "mbimetadata.h"

#include "pch.h"
#include "MBIException.h"

#include <assert.h>
#include <iostream>
#include <vector>
#include <hdf5.h>
namespace MBI
{
    herr_t attr_op(hid_t group, const char* name, const H5L_info2_t* info, void* op_data)
    {
        std::vector<std::string>* data = static_cast<std::vector<std::string>*>(op_data);
        data->push_back(std::string(name));
        return 0;
    }

    IMetadata::IMetadata() {}

    IMetadata::IMetadata(hid_t& group) : group(0)
    {
        this->group = group;
    }

    void IMetadata::Copy(IMetadata& toCopy)
    {
        std::map<std::string, std::string> copiedCache = toCopy.GetCache();
        SetCache(copiedCache);
    }

    void IMetadata::Close()
    {
        if (group != 0)
        {
            herr_t err = H5Gclose(group);
            /*if (err < 0) {
                std::cout << "Error close metagroup" << std::endl;
            }*/
            group = 0;
        }
        //group.close();
    }

    IMetadata::~IMetadata()
    {
        Close();
        //group.close();
    }

    const char* IMetadata::ReadString(const char* key)
    {
        //char* val;// = (char*)malloc(sizeof(char) * 1024);
        char* val = nullptr;
        if (std::string(key) == "") {
            return "";
        }
        //H5Eset_auto(NULL, NULL, NULL);
        //H5Eset_auto(NULL, NULL, NULL);
        if (group == 0)
            return "";
        if (cache.find(std::string(key)) == cache.end())
        {
            if (std::string(key).find("smp-concentration") != std::string::npos) {
                cache[std::string(key)] = std::string("50");
                return cache[std::string(key)].c_str();
            }
            try
            {
                herr_t ret_value = H5Aexists(group, key);
                if (ret_value > 0)
                {
                    hid_t attr_id = H5Aopen(group, key, H5P_DEFAULT);
                    if (attr_id > 0) {
                                hid_t atype = H5Aget_type(attr_id);
                                int size = H5Tget_size(atype);
                                //char* attr_buf = (char*)malloc(size + 1);
                                //memset(attr_buf, 0, size + 1);
                                //auto type = H5Tcopy(H5T_C_S1);
                                auto type_class = H5Tget_class(atype);
                                if (type_class == H5T_STRING) {
                                    hid_t memtype = H5Tget_native_type(atype, H5T_DIR_ASCEND);
                                    //herr_t ret_value = H5Aread(attr_id, memtype, attr_buf);
                                    herr_t ret_value = H5Aread(attr_id, atype, &val);// atype, & val);
                                    if (ret_value >= 0) {
                                        cache[std::string(key)] = std::string(val);
                                        H5free_memory(val);
                                    }
                                    else {
                                        cache[std::string(key)] = std::string("");
                                    }
                                }
                                else if (type_class == H5T_INTEGER)
                                {
                                    herr_t ret_value = H5Aread(attr_id, atype, &val);// atype, & val);
                                    if (ret_value >= 0) {
                                        cache[std::string(key)] = std::string(val);
                                        free(val);
                                    }
                                    else {
                                        cache[std::string(key)] = std::string("");
                                    }
                                }
                                H5Tclose(atype);
                                H5Aclose(attr_id);
                    }

                    return cache[std::string(key)].c_str();
                }
            }
            catch (std::exception& exc)
            {
                cache[std::string(key)] = "";
            }
            //catch (const H5::Exception& ex)
            //{
            //	cache[std::string(key)] = "";
            //}
            catch (...)
            {
                cache[std::string(key)] = "";
            }
            return "";
        }
//		free(val);
        return cache[std::string(key)].c_str();
    }

    const char* IMetadata::ReadString(std::string key)
    {
        return ReadString(key.c_str());
    }
    // adc-min-nanoseconds -> check!!!
    const int IMetadata::ReadInt(const char* key)
    {
        return std::atoi(ReadString(key));
    }

    const double IMetadata::ReadDouble(const char* key)
    {
        return std::atof(ReadString(key));
    }

    void IMetadata::LoadAll()
    {
        std::vector<std::string>* data = new std::vector<std::string>();
        if (group == 0)
            return;
        //H5Eset_auto(NULL, NULL, NULL);
        if (H5Literate2(group, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_op, static_cast<void*>(data)) < 0) {
            throw MBIException("function:IMetadata::LoadAll", "Error in H5Literate2");
        }
        for (auto name : *data)
        {
            ReadString(name);
        }
        /*group.iterateAttrs((H5::attr_operator_t) attr_op, NULL, static_cast<void*>(data));
        */
        delete data;
    }
    // Read
    const std::map<std::string, std::string>& IMetadata::ReadAll()
    {
        LoadAll();
        return GetCache();
    }

    const std::map<std::string, std::string>& IMetadata::GetCache()
    {
        return cache;
    }

    void IMetadata::SetCache(const std::map<std::string, std::string>& toCopy)
    {
#if (__cplusplus == 201703L)
        std::copy(toCopy.begin(), toCopy.end(), std::inserter(cache, cache.end()) );
#else
        for (auto m : toCopy) {
            cache[m.first] = m.second;
        }
#endif
    }

    void IMetadata::AddMetadata(const char* key, const char* val)
    {
        cache[std::string(key)] = std::string(val);
    }
    void IMetadata::AddMetadata(const char* key, std::string val)
    {
        cache[std::string(key)] = val;
    }
    void IMetadata::Write()
    {
        if (group == 0)
            return;
        for (auto attrs : cache)
        {
            if (attrs.first == "")
                continue;
            if (attrs.second == "")
                continue;
            const char* key = attrs.first.c_str();
            const char* val[1] = { attrs.second.c_str() };
            const int str_size = attrs.second.length();
            hid_t space = H5Screate(H5S_SCALAR);
            if (space < 0) {
                continue;
            }
            hid_t attrdatatypeid = H5Tcopy(H5T_C_S1);

            if (attrdatatypeid < 0) {
                H5Sclose(space);
                continue;
            }

            auto status = H5Tset_size(attrdatatypeid, H5T_VARIABLE);
            if (status < 0) {
                H5Sclose(space);
                H5Tclose(attrdatatypeid);
                continue;
            }

            herr_t ret_value = H5Aexists(group, key);
            if (ret_value > 0) {
                herr_t ret_value = H5Adelete(group, key);
                if (ret_value < 0) {
                    H5Sclose(space);
                    H5Tclose(attrdatatypeid);
                    continue;
                }
            }

            hid_t attrb = H5Acreate(group, key, attrdatatypeid, space, H5P_DEFAULT, H5P_DEFAULT);

            if (attrb < 0) {
                H5Sclose(space);
                H5Tclose(attrdatatypeid);
                continue;
            }
            status = H5Awrite(attrb, attrdatatypeid, val);
            if (status < 0) {
                H5Sclose(space);
                H5Tclose(attrdatatypeid);
                H5Aclose(attrb);
            }
            else {
                H5Sclose(space);
                H5Tclose(attrdatatypeid);
                H5Aclose(attrb);
            }
        }

    }

    FragmentationMetadata::FragmentationMetadata()
    {
        type = eType::NONE;
        fragEnergy = 0.0;
    }
}


