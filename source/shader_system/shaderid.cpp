#include "pch.h"

#include "shaderid.h"

#include "utils/debug/assertion.h"
#include "utils/data_structures/hash.h"


ShaderID::ShaderID(ds::StrID filepath)
    : m_filepath(filepath)
{
}


ShaderID::ShaderID(ds::StrID filepath, OptimizationLevel level)
    : m_filepath(filepath), m_optimizationLevel(level)
{
}


ShaderID::ShaderID(ds::StrID filepath, const std::bitset<MAX_SHADER_DEFINES_COUNT>& defineBits, OptimizationLevel level)
    : m_defineBits(defineBits), m_filepath(filepath), m_optimizationLevel(level)
{
}


void ShaderID::SetDefineBit(size_t index, bool value) noexcept
{
    AM_ASSERT_GRAPHICS_API(index < MAX_SHADER_DEFINES_COUNT, "Max shader ({}) defines ({}) count reached", m_filepath.CStr(), MAX_SHADER_DEFINES_COUNT);

    m_defineBits.set(index, value);
}


void ShaderID::ClearDefineBit(size_t index) noexcept
{
    SetDefineBit(index, false);
}


void ShaderID::ClearBits() noexcept
{
    m_defineBits.reset();
}


void ShaderID::SetOptimizationLevel(OptimizationLevel level) noexcept
{
    AM_ASSERT_GRAPHICS_API(level < OPTIMIZATION_LEVEL_COUNT, "Invalid optimization level ({})", static_cast<uint32_t>(level));
    m_optimizationLevel = level;
}


bool ShaderID::IsDefineBit(size_t index) const noexcept
{
    return m_defineBits.test(index);
}


uint64_t ShaderID::Hash() const noexcept
{
    ds::HashBuilder builder;
    builder.AddValue(m_filepath);
    builder.AddValue(m_defineBits);
    builder.AddValue(m_optimizationLevel); 

    return builder.Value();
}


ShaderIDProxy::ShaderIDProxy(const ShaderID &id)
    : m_hash(id.Hash())
{
}


ShaderIDProxy::ShaderIDProxy(uint64_t shaderHash)
    : m_hash(shaderHash)
{
}
