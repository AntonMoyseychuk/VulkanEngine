#include "strid.h"

namespace ds
{
    StrIDDataStorage::StrIDDataStorage()
    {
        m_storage.reserve(PREALLOCATED_IDS_COUNT);
    }
    
    
    uint64_t StrIDDataStorage::Store(const std::string_view &str) noexcept
    {
        if (!str.data()) {
            return INVALID_ID_HASH;
        }

        const uint64_t id = s_hasher(str);

        if (!IsExist(id)) {
            m_storage[id] = str;
        }

        return id;
    }
    
    
    const std::string* StrIDDataStorage::Load(uint64_t id) const noexcept
    {
        const auto result = m_storage.find(id);

        return result == m_storage.cend() ? nullptr : &result->second;
    }
    
    
    StrID::StrID(const char *str) 
        : m_id(s_storage.Store(str))
    {
    }
    
    
    StrID::StrID(const std::string &str)
        : m_id(s_storage.Store(str))
    {
    }
    
    
    StrID::StrID(const std::string_view &str)
        : m_id(s_storage.Store(str))
    {
    }
    
    
    StrID &StrID::operator=(const char *str) noexcept
    {
        m_id = s_storage.Store(str);
        return *this;
    }
    
    
    const char* StrID::CStr() const noexcept
    {
        const std::string* pResult = s_storage.Load(m_id);
            
        return pResult ? pResult->c_str() : nullptr;
    }
}