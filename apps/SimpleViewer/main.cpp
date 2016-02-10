#include <GLFWDriver.h>

#include <Cogwheel/Assets/Mesh.h>
#include <Cogwheel/Assets/MeshModel.h>
#include <Cogwheel/Core/Engine.h>
#include <Cogwheel/Input/Keyboard.h>
#include <Cogwheel/Input/Mouse.h>
#include <Cogwheel/Scene/Camera.h>

#include <ObjLoader/ObjLoader.h>

#include <OptiXRenderer/Renderer.h>

#include <cstdio>
#include <iostream>

using namespace Cogwheel::Assets;
using namespace Cogwheel::Core;
using namespace Cogwheel::Input;
using namespace Cogwheel::Math;
using namespace Cogwheel::Scene;

static std::string g_filepath;

class SimpleRotator final {
public:
    SimpleRotator(SceneNodes::UID node_ID)
        : m_node_ID(node_ID) { }

    void rotate(Engine& engine) {
        Transform transform = SceneNodes::get_global_transform(m_node_ID);
        transform.rotation = Quaternionf::from_angle_axis(engine.get_time().get_total_time() * 0.1f, Vector3f::up());
        SceneNodes::set_global_transform(m_node_ID, transform);
    }

    static inline void rotate_callback(Cogwheel::Core::Engine& engine, void* state) {
        static_cast<SimpleRotator*>(state)->rotate(engine);
    }

private:
    SceneNodes::UID m_node_ID;
};

class Navigation final {
public:

    Navigation(SceneNodes::UID node_ID, float velocity) 
        : m_node_ID(node_ID)
        , m_vertical_rotation(0.0f) 
        , m_horizontal_rotation(0.0f)
        , m_velocity(velocity)
    { }

    void navigate(Engine& engine) {
        const Keyboard* keyboard = engine.get_keyboard();
        const Mouse* mouse = engine.get_mouse();

        SceneNode node = m_node_ID;
        Transform transform = node.get_global_transform();

        { // Translation
            float strafing = 0.0f;
            if (keyboard->is_pressed(Keyboard::Key::D))
                strafing = 1.0f;
            if (keyboard->is_pressed(Keyboard::Key::A))
                strafing -= 1.0f;

            float forward = 0.0f;
            if (keyboard->is_pressed(Keyboard::Key::W))
                forward = 1.0f;
            if (keyboard->is_pressed(Keyboard::Key::S))
                forward -= 1.0f;

            if (strafing != 0.0f || forward != 0.0f) {
                Vector3f translation_offset = transform.rotation * Vector3f(strafing, 0.0f, forward);
                transform.translation += normalize(translation_offset) * m_velocity * engine.get_time().get_smooth_delta_time();
            }
        }

        { // Rotation
            if (mouse->get_left_button().is_pressed) {
                m_vertical_rotation += degrees_to_radians(float(mouse->get_delta().x));

                // Clamp horizontal rotation to -89 and 89 degrees to avoid turning the camera on it's head and the singularities of cross products at the poles.
                m_horizontal_rotation += degrees_to_radians(float(mouse->get_delta().y));
                m_horizontal_rotation = clamp(m_horizontal_rotation, -PI<float>() * 0.49f, PI<float>() * 0.49f);

                transform.rotation = Quaternionf::from_angle_axis(m_vertical_rotation, Vector3f::up()) * Quaternionf::from_angle_axis(m_horizontal_rotation, Vector3f::right());
            }
        }

        if (transform != node.get_global_transform())
            node.set_global_transform(transform);
    }

    static inline void navigate_callback(Cogwheel::Core::Engine& engine, void* state) {
        static_cast<Navigation*>(state)->navigate(engine);
    }

private:
    SceneNodes::UID m_node_ID;
    float m_vertical_rotation;
    float m_horizontal_rotation;
    float m_velocity;
};

static inline void scenenode_cleanup_callback(void* dummy) {
    SceneNodes::clear_change_notifications();
}

void initializer(Cogwheel::Core::Engine& engine) {
    engine.get_window().set_name("SimpleViewer");

    SceneNodes::allocate(8u);
    engine.add_tick_cleanup_callback(scenenode_cleanup_callback, nullptr);
    Meshes::allocate(8u);
    MeshModels::allocate(8u);

    ObjLoader::load(g_filepath);

    AABB scene_bounds = AABB::invalid();
    for (Meshes::ConstUIDIterator uid_itr = Meshes::begin(); uid_itr != Meshes::end(); ++uid_itr) {
        AABB mesh_aabb = Meshes::get_bounds(*uid_itr);
        scene_bounds.grow_to_contain(mesh_aabb);
    }
    
    if (SceneNodes::begin() != SceneNodes::end()) {
        // Rotate first node.
        SimpleRotator* simple_rotator = new SimpleRotator(*SceneNodes::begin());
        engine.add_mutating_callback(SimpleRotator::rotate_callback, simple_rotator);
    }

    { // Add camera
        Cameras::allocate(1u);
        SceneNodes::UID cam_node_ID = SceneNodes::create("Cam");

        Matrix4x4f perspective_matrix, inverse_perspective_matrix;
        CameraUtils::compute_perspective_projection(0.1f, 100.0f, PI<float>() / 4.0f, 8.0f / 6.0f,
            perspective_matrix, inverse_perspective_matrix);

        Cameras::UID cam_ID = Cameras::create(cam_node_ID, perspective_matrix, inverse_perspective_matrix);

        float camera_velocity = magnitude(scene_bounds.size()) * 0.1f;
        Navigation* camera_navigation = new Navigation(cam_node_ID, camera_velocity);
        engine.add_mutating_callback(Navigation::navigate_callback, camera_navigation);
    }
}

void initialize_window(Cogwheel::Core::Window& window) {
    OptiXRenderer::Renderer* renderer = new OptiXRenderer::Renderer();
    Engine::get_instance()->add_non_mutating_callback(OptiXRenderer::render_callback, renderer);
}

void main(int argc, char** argv) {
    g_filepath = argc >= 2 ? std::string(argv[1]) : "";

    if (g_filepath.empty()) {
        printf("SimpleViewer requires path to model as first argument.\n");
        exit(1);
    }

    GLFWDriver::run(initializer, initialize_window);
}
