namespace ds 
{
    template <typename ElemT>
    inline StrIDDataStorage<ElemT>::StrIDDataStorage()
    {
        m_storage.reserve(PREALLOCATED_IDS_COUNT);
    }


    template <typename ElemT>
    inline uint64_t StrIDDataStorage<ElemT>::Store(const StrIDDataStorage<ElemT>::StringViewType &str) noexcept
    {
        if (!str.data()) {
            return INVALID_ID_HASH;
        }

        const uint64_t id = amHash(str);

        if (!IsExist(id)) {
            m_storage[id] = str;
        }

        return id;
    }


    template <typename ElemT>
    inline const typename StrIDDataStorage<ElemT>::StringType* StrIDDataStorage<ElemT>::Load(uint64_t id) const noexcept
    {
        const auto result = m_storage.find(id);

        return result == m_storage.cend() ? nullptr : &result->second;
    }


    template <typename ElemT>
    inline StrIDImpl<ElemT>::StrIDImpl(const ElementType *str)
        : m_id(s_storage.Store(str))
    {
    }
    
    
    template <typename ElemT>
    inline StrIDImpl<ElemT>::StrIDImpl(const StringType &str)
        : m_id(s_storage.Store(str))
    {
    }
    
    
    template <typename ElemT>
    inline StrIDImpl<ElemT>::StrIDImpl(const StringViewType &str)
        : m_id(s_storage.Store(str))
    {
    }


    template <typename ElemT>
    inline StrIDImpl<ElemT>& StrIDImpl<ElemT>::operator=(const typename StrIDImpl<ElemT>::ElementType *str) noexcept
    {
        m_id = s_storage.Store(str);
        return *this;
    }
    
    
    template <typename ElemT>
    inline const char *StrIDImpl<ElemT>::CStr() const noexcept
    {
        const StringType* pResult = s_storage.Load(m_id);
            
        return pResult ? pResult->c_str() : nullptr;
    }
}