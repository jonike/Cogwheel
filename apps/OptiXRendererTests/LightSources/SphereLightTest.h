// Test sphere lights in OptiXRenderer.
// ---------------------------------------------------------------------------
// Copyright (C) 2015-2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#ifndef _OPTIXRENDERER_LIGHT_SOURCES_SPHERE_LIGHT_TEST_H_
#define _OPTIXRENDERER_LIGHT_SOURCES_SPHERE_LIGHT_TEST_H_

#include <Utils.h>

#include <OptiXRenderer/Shading/LightSources/SphereLightImpl.h>

#include <gtest/gtest.h>

namespace OptiXRenderer {

GTEST_TEST(SphereLight, power_preservation_when_radius_changes) {
    using namespace optix;

    const unsigned int MAX_SAMPLES = 1024u;

    SphereLight light_with_radius_0;
    light_with_radius_0.position = make_float3(0.0f, 10.0f, 0.0f);
    light_with_radius_0.radius = 0.0f;
    light_with_radius_0.power = make_float3(10.0f); // We only care about the red channel in the tests

    SphereLight light_with_radius_1 = light_with_radius_0;
    light_with_radius_1.radius = 1.0f;

    SphereLight light_with_radius_2 = light_with_radius_0;
    light_with_radius_2.radius = 2.0f;

    SphereLight light_with_radius_5 = light_with_radius_0;
    light_with_radius_5.radius = 5.0f;

    SphereLight light_with_radius_9 = light_with_radius_0;
    light_with_radius_9.radius = 9.0f;

    float3 shading_position = make_float3(0.0f);
    float3 shading_normal = make_float3(0.0f, 1.0f, 0.0f);

    // Sample 1024 QMC samples (sample02) and compare the power of a light source with 3 different radii.
    float luminances_at_radius_0[MAX_SAMPLES];
    float luminances_at_radius_1[MAX_SAMPLES];
    float luminances_at_radius_2[MAX_SAMPLES];
    float luminances_at_radius_5[MAX_SAMPLES];
    float luminances_at_radius_9[MAX_SAMPLES];
    for (unsigned int i = 0u; i < MAX_SAMPLES; ++i) {
        float2 random_sample = RNG::sample02(i);

        LightSample sample0 = LightSources::sample_radiance(light_with_radius_0, shading_position, random_sample);
        luminances_at_radius_0[i] = sample0.radiance.x * (dot(shading_normal, sample0.direction) / sample0.PDF);

        LightSample sample1 = LightSources::sample_radiance(light_with_radius_1, shading_position, random_sample);
        luminances_at_radius_1[i] = sample1.radiance.x * (dot(shading_normal, sample1.direction) / sample1.PDF);

        LightSample sample2 = LightSources::sample_radiance(light_with_radius_2, shading_position, random_sample);
        luminances_at_radius_2[i] = sample2.radiance.x * (dot(shading_normal, sample2.direction) / sample2.PDF);

        LightSample sample5 = LightSources::sample_radiance(light_with_radius_5, shading_position, random_sample);
        luminances_at_radius_5[i] = sample5.radiance.x * (dot(shading_normal, sample5.direction) / sample5.PDF);

        LightSample sample9 = LightSources::sample_radiance(light_with_radius_9, shading_position, random_sample);
        luminances_at_radius_9[i] = sample9.radiance.x * (dot(shading_normal, sample9.direction) / sample9.PDF);
    }

    float luminance_at_radius_0 = sort_and_pairwise_summation(luminances_at_radius_0, luminances_at_radius_0 + MAX_SAMPLES) / float(MAX_SAMPLES);
    float luminance_at_radius_1 = sort_and_pairwise_summation(luminances_at_radius_1, luminances_at_radius_1 + MAX_SAMPLES) / float(MAX_SAMPLES);
    float luminance_at_radius_2 = sort_and_pairwise_summation(luminances_at_radius_2, luminances_at_radius_2 + MAX_SAMPLES) / float(MAX_SAMPLES);
    float luminance_at_radius_5 = sort_and_pairwise_summation(luminances_at_radius_5, luminances_at_radius_5 + MAX_SAMPLES) / float(MAX_SAMPLES);
    float luminance_at_radius_9 = sort_and_pairwise_summation(luminances_at_radius_9, luminances_at_radius_9 + MAX_SAMPLES) / float(MAX_SAMPLES);

    // Map radiance arriving at intersection point to light source power by applying an inverse quadratric fall off and assume that all lights are point lights.
    float distance_to_light = length(shading_position - light_with_radius_0.position);
    float light_0_power = luminance_at_radius_0 * (4.0f * PIf * distance_to_light * distance_to_light);
    float light_1_power = luminance_at_radius_1 * (4.0f * PIf * distance_to_light * distance_to_light);
    float light_2_power = luminance_at_radius_2 * (4.0f * PIf * distance_to_light * distance_to_light);
    float light_5_power = luminance_at_radius_5 * (4.0f * PIf * distance_to_light * distance_to_light);
    float light_9_power = luminance_at_radius_9 * (4.0f * PIf * distance_to_light * distance_to_light);

    // Test that the estimated power equals
    EXPECT_TRUE(light_with_radius_0.power.x == light_0_power);
    EXPECT_TRUE(almost_equal_eps(light_with_radius_1.power.x, light_1_power, 0.0001f));
    EXPECT_TRUE(almost_equal_eps(light_with_radius_2.power.x, light_2_power, 0.001f));
    EXPECT_TRUE(almost_equal_eps(light_with_radius_5.power.x, light_5_power, 0.001f));
    EXPECT_TRUE(almost_equal_eps(light_with_radius_9.power.x, light_9_power, 0.004f));
}

GTEST_TEST(SphereLight, consistent_PDF) {
    using namespace optix;

    const unsigned int MAX_SAMPLES = 128u;
    const float3 position = make_float3(10.0f, 0.0f, 0.0f); // TODO vary.

    SphereLight light;
    light.position = make_float3(0.0f, 10.0f, 0.0f);
    light.radius = 1.0f;
    light.power = make_float3(10.0f);

    std::vector<SphereLight> lights = { light, light, light, light, light };
    lights[1].radius = 2.0f;
    lights[1].radius = 4.0f;
    lights[1].radius = 7.0f;
    lights[1].radius = 13.0f;

    for (SphereLight& light : lights) {
        for (unsigned int i = 0u; i < MAX_SAMPLES; ++i) {
            LightSample sample = LightSources::sample_radiance(light, position, RNG::sample02(i));
            if (is_PDF_valid(sample.PDF)) {
                float PDF = LightSources::PDF(light, position, sample.direction);
                EXPECT_TRUE(almost_equal_eps(sample.PDF, PDF, 0.0001f));
            }
        }
    }
}

GTEST_TEST(SphereLight, pdf_rejects_rays_that_miss) {
    using namespace optix;

    SphereLight light;
    light.position = make_float3(0.0f, 10.0f, 0.0f);
    light.radius = 2.0f;
    light.power = make_float3(10.0f);

    const float3 lit_position = make_float3(0.0f, 0.0f, 0.0f);
    const float3 hit_light_direction = normalize(make_float3(1.0f, 10.0f, 0.0f));
    const float3 miss_light_direction = normalize(make_float3(3.0f, 10.0f, 0.0f));

    float hit_light_PDF = LightSources::PDF(light, lit_position, hit_light_direction);
    EXPECT_GT(hit_light_PDF, 0.0f);

    float miss_light_PDF = LightSources::PDF(light, lit_position, miss_light_direction);
    EXPECT_FLOAT_EQ(miss_light_PDF, 0.0f);
}

} // NS OptiXRenderer

#endif // _OPTIXRENDERER_LIGHT_SOURCES_SPHERE_LIGHT_TEST_H_