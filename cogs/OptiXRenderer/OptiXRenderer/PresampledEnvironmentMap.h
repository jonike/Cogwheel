// OptiXRenderer presampled environment map.
// ------------------------------------------------------------------------------------------------
// Copyright (C) 2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. 
// See LICENSE.txt for more detail.
// ------------------------------------------------------------------------------------------------

#ifndef _OPTIXRENDERER_PRESAMPLED_ENVIRONMENT_MAP_H_
#define _OPTIXRENDERER_PRESAMPLED_ENVIRONMENT_MAP_H_

#include <OptiXRenderer/Types.h>

#include <Cogwheel/Assets/Texture.h>

#include <optixu/optixpp_namespace.h>

//-------------------------------------------------------------------------------------------------
// Forward declarations.
//-------------------------------------------------------------------------------------------------
namespace Cogwheel {
namespace Assets {
    class InfiniteAreaLight;
}
}

namespace OptiXRenderer {

//-------------------------------------------------------------------------------------------------
// Presampled environment map samples.
// The samples stored in the buffer are of type LightSample.
//-------------------------------------------------------------------------------------------------
class PresampledEnvironmentMap final {
public:
    //---------------------------------------------------------------------------------------------
    // Constructors and destructor.
    //---------------------------------------------------------------------------------------------
    PresampledEnvironmentMap()
        : m_per_pixel_PDF(nullptr), m_samples(nullptr) {}
    PresampledEnvironmentMap(optix::Context& context, const Cogwheel::Assets::InfiniteAreaLight& light, 
                             optix::TextureSampler* texture_cache, int sample_count = 0);

    PresampledEnvironmentMap& operator=(PresampledEnvironmentMap&& rhs) {
        m_environment_map_ID = rhs.m_environment_map_ID;
        m_per_pixel_PDF = rhs.m_per_pixel_PDF; rhs.m_per_pixel_PDF = nullptr;
        m_samples = rhs.m_samples; rhs.m_samples = nullptr;
        m_light = rhs.m_light;
        return *this;
    }

    ~PresampledEnvironmentMap();

    //---------------------------------------------------------------------------------------------
    // Getters.
    //---------------------------------------------------------------------------------------------
    Light get_light() const {
        Light light;
        light.flags = Light::PresampledEnvironment;
        light.presampled_environment = m_light;
        return light;
    }

    // Next event estimation is disabled if sample count is 1. The only sample in the buffer is going to be an invalid one.
    inline bool next_event_estimation_possible() { return m_light.sample_count > 1; }

    Cogwheel::Assets::Textures::UID get_environment_map_ID() const { return m_environment_map_ID; }

private:
    PresampledEnvironmentMap(PresampledEnvironmentMap& other) = delete;
    PresampledEnvironmentMap(PresampledEnvironmentMap&& other) = delete;
    PresampledEnvironmentMap& operator=(PresampledEnvironmentMap& rhs) = delete;

    Cogwheel::Assets::Textures::UID m_environment_map_ID;

    optix::TextureSampler m_per_pixel_PDF;
    optix::Buffer m_samples;

    PresampledEnvironmentLight m_light;
};

} // NS OptiXRenderer

#endif // _OPTIXRENDERER_PRESAMPLED_ENVIRONMENT_MAP_H_