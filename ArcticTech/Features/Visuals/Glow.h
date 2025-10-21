#pragma once
#include <vector>

namespace glow {
    void draw();
    void restore();

    extern std::vector<GlowObjectDefinition_t> m_glObjectDefinitions;
}