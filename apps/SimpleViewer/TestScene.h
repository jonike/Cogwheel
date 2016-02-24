// SimpleViewer test scene
// ---------------------------------------------------------------------------
// Copyright (C) 2015-2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#ifndef _SIMPLEVIEWER_TEST_SCENE_H_
#define _SIMPLEVIEWER_TEST_SCENE_H_

#include <Cogwheel/Assets/Mesh.h>
#include <Cogwheel/Assets/MeshCreation.h>
#include <Cogwheel/Assets/MeshModel.h>
#include <Cogwheel/Core/Engine.h>
#include <Cogwheel/Input/Keyboard.h>
#include <Cogwheel/Math/Transform.h>
#include <Cogwheel/Scene/Camera.h>
#include <Cogwheel/Scene/SceneNode.h>

using namespace Cogwheel;

class LocalRotator final {
public:
    LocalRotator(Scene::SceneNodes::UID node_ID)
        : m_node_ID(node_ID) {
    }

    void rotate(Core::Engine& engine) {
        Math::Transform transform = Scene::SceneNodes::get_local_transform(m_node_ID);
        transform.rotation = Math::Quaternionf::from_angle_axis(float(engine.get_time().get_total_time()) * 0.1f, Math::Vector3f::up());
        Scene::SceneNodes::set_local_transform(m_node_ID, transform);
    }

    static inline void rotate_callback(Core::Engine& engine, void* state) {
        static_cast<LocalRotator*>(state)->rotate(engine);
    }

private:
    Scene::SceneNodes::UID m_node_ID;
};

class BoxGun final {
public:
    BoxGun(Scene::SceneNodes::UID shooter_node_ID)
        : m_shooter_node_ID(shooter_node_ID)
        , m_cube_mesh_ID(Assets::MeshCreation::cube(1))
        , m_model_ID(Assets::MeshModels::UID::invalid_UID())
        , m_existed_time(0.0f) {
    }

    void update(Core::Engine& engine) {
        using namespace Cogwheel::Assets;
        using namespace Cogwheel::Input;
        using namespace Cogwheel::Scene;

        const Keyboard* keyboard = engine.get_keyboard();

        if (keyboard->is_pressed(Keyboard::Key::Space) && m_model_ID == MeshModels::UID::invalid_UID()) {
            Math::Transform transform = SceneNodes::get_global_transform(m_shooter_node_ID);
            transform.scale = 0.1f;
            transform.translation -= transform.rotation.up() * transform.scale;
            SceneNodes::UID cube_node_ID = SceneNodes::create("Cube", transform);
            m_model_ID = MeshModels::create(cube_node_ID, m_cube_mesh_ID);
            m_existed_time = 0.0f;
        }

        if (m_model_ID != MeshModels::UID::invalid_UID()) {
            float delta_time = engine.get_time().get_smooth_delta_time();
            m_existed_time += delta_time;
            SceneNode cube_node = MeshModels::get_model(m_model_ID).scene_node_ID;
            Math::Transform transform = cube_node.get_global_transform();
            transform.translation += transform.rotation.forward() * 3.0f * delta_time;
            cube_node.set_global_transform(transform);

            // Cube bullets should only exist for 2 seconds.
            if (m_existed_time > 2.0f) {
                SceneNodes::destroy(cube_node.get_ID());
                MeshModels::destroy(m_model_ID);
                m_model_ID = MeshModels::UID::invalid_UID();
            }
        }
    }

    static inline void update_callback(Core::Engine& engine, void* state) {
        static_cast<BoxGun*>(state)->update(engine);
    }

private:
    Scene::SceneNodes::UID m_shooter_node_ID;
    Assets::Meshes::UID m_cube_mesh_ID;
    Assets::MeshModels::UID m_model_ID;

    float m_existed_time;
};

Scene::SceneNodes::UID create_test_scene(Core::Engine& engine) {
    using namespace Cogwheel::Assets;
    using namespace Cogwheel::Math;
    using namespace Cogwheel::Scene;

    SceneNode root_node = SceneNodes::create("Root");

    { // Add camera
        Cameras::allocate(1u);
        SceneNodes::UID cam_node_ID = SceneNodes::create("Cam");

        Transform cam_transform = SceneNodes::get_global_transform(cam_node_ID);
        cam_transform.translation = Vector3f(0, 1, -6);
        SceneNodes::set_global_transform(cam_node_ID, cam_transform);

        Matrix4x4f perspective_matrix, inverse_perspective_matrix;
        CameraUtils::compute_perspective_projection(0.1f, 100.0f, PI<float>() / 4.0f, 8.0f / 6.0f,
            perspective_matrix, inverse_perspective_matrix);

        Cameras::UID cam_ID = Cameras::create(cam_node_ID, perspective_matrix, inverse_perspective_matrix);
    }

    { // Create floor.
        SceneNode plane_node = SceneNodes::create("Floor");
        Meshes::UID plane_mesh_ID = MeshCreation::plane(10);
        MeshModels::UID plane_model_ID = MeshModels::create(plane_node.get_ID(), plane_mesh_ID);
        plane_node.set_parent(root_node);
    }

    { // Create rotating box. TODO Replace by those three cool spinning rings later.
        Transform transform = Transform(Vector3f(0.0f, 0.5f, 0.0f));
        SceneNode cube_node = SceneNodes::create("Rotating cube", transform);
        Meshes::UID cube_mesh_ID = MeshCreation::cube(3);
        MeshModels::UID cube_model_ID = MeshModels::create(cube_node.get_ID(), cube_mesh_ID);
        cube_node.set_parent(root_node);

        LocalRotator* simple_rotator = new LocalRotator(cube_node.get_ID());
        engine.add_mutating_callback(LocalRotator::rotate_callback, simple_rotator);
    }

    { // Destroyable cylinder. Test by destroying the mesh, model and scene node.
        Transform transform = Transform(Vector3f(-1.5f, 0.5f, 0.0f));
        SceneNode cylinder_node = SceneNodes::create("Destroyed Cylinder", transform);
        Meshes::UID cylinder_mesh_ID = MeshCreation::cylinder(4, 16);
        MeshModels::UID cylinder_model_ID = MeshModels::create(cylinder_node.get_ID(), cylinder_mesh_ID);
        cylinder_node.set_parent(root_node);
    }

    { // Sphere for the hell of it. 
        Transform transform = Transform(Vector3f(1.5f, 0.5f, 0.0f));
        SceneNode sphere_node = SceneNodes::create("Sphere", transform);
        Meshes::UID sphere_mesh_ID = MeshCreation::revolved_sphere(32, 16);
        MeshModels::UID sphere_model_ID = MeshModels::create(sphere_node.get_ID(), sphere_mesh_ID);
        sphere_node.set_parent(root_node);
    }

    { // GUN!
        Cameras::UID cam_ID = *Cameras::begin();
        SceneNodes::UID cam_node_ID = Cameras::get_node_ID(cam_ID);
        BoxGun* boxgun = new BoxGun(cam_node_ID);
        engine.add_mutating_callback(BoxGun::update_callback, boxgun);
    }

    return root_node.get_ID();
}

#endif _SIMPLEVIEWER_TEST_SCENE_H_