// Test Cogwheel meshes.
// ---------------------------------------------------------------------------
// Copyright (C) 2015-2016, Cogwheel. See AUTHORS.txt for authors
//
// This program is open source and distributed under the New BSD License. See
// LICENSE.txt for more detail.
// ---------------------------------------------------------------------------

#ifndef _COGWHEEL_ASSETS_MESH_TEST_H_
#define _COGWHEEL_ASSETS_MESH_TEST_H_

#include <Cogwheel/Assets/Mesh.h>

#include <gtest/gtest.h>

namespace Cogwheel {
namespace Assets {

class Assets_Mesh : public ::testing::Test {
protected:
    // Per-test set-up and tear-down logic.
    virtual void SetUp() {
        Meshes::allocate(8u);
    }
    virtual void TearDown() {
        Meshes::deallocate();
    }
};

TEST_F(Assets_Mesh, resizing) {
    // Test that capacity can be increased.
    unsigned int largerCapacity = Meshes::capacity() + 4u;
    Meshes::reserve(largerCapacity);
    EXPECT_GE(Meshes::capacity(), largerCapacity);

    // Test that capacity won't be decreased.
    Meshes::reserve(5u);
    EXPECT_GE(Meshes::capacity(), largerCapacity);

    Meshes::deallocate();
    EXPECT_LT(Meshes::capacity(), largerCapacity);
}

TEST_F(Assets_Mesh, sentinel_mesh) {
    Meshes::UID sentinel_ID = Meshes::UID::invalid_UID();

    EXPECT_FALSE(Meshes::has(sentinel_ID));
    EXPECT_EQ(0u, Meshes::get_index_count(sentinel_ID));
    EXPECT_EQ(nullptr, Meshes::get_indices(sentinel_ID));
    EXPECT_EQ(0u, Meshes::get_vertex_count(sentinel_ID));
    EXPECT_EQ(nullptr, Meshes::get_positions(sentinel_ID));
    EXPECT_EQ(nullptr, Meshes::get_normals(sentinel_ID));
    EXPECT_EQ(nullptr, Meshes::get_texcoords(sentinel_ID));
    Math::AABB bounds = Meshes::get_bounds(sentinel_ID);
    EXPECT_TRUE(isnan(bounds.minimum.x) && isnan(bounds.minimum.y) && isnan(bounds.minimum.z) &&
                isnan(bounds.maximum.x) && isnan(bounds.maximum.y) && isnan(bounds.maximum.z));
}

TEST_F(Assets_Mesh, create) {
    Meshes::UID mesh_ID = Meshes::create("TestMesh", 32u, 16u);

    EXPECT_TRUE(Meshes::has(mesh_ID));
    EXPECT_EQ(32u, Meshes::get_index_count(mesh_ID));
    EXPECT_NE(nullptr, Meshes::get_indices(mesh_ID));
    EXPECT_EQ(16u, Meshes::get_vertex_count(mesh_ID));
    EXPECT_NE(nullptr, Meshes::get_positions(mesh_ID));
    EXPECT_NE(nullptr, Meshes::get_normals(mesh_ID));
    EXPECT_NE(nullptr, Meshes::get_texcoords(mesh_ID));
    EXPECT_INVALID_AABB(Meshes::get_bounds(mesh_ID));

    // Test mesh created notification.
    Core::Iterable<Meshes::ChangedIterator> changed_meshes = Meshes::get_changed_meshes();
    EXPECT_EQ(1, changed_meshes.end() - changed_meshes.begin());
    EXPECT_EQ(mesh_ID, *changed_meshes.begin());
    EXPECT_EQ(Meshes::Changes::Created, Meshes::get_changes(mesh_ID));
}

TEST_F(Assets_Mesh, create_only_positions) {
    Meshes::UID mesh_ID = Meshes::create("TestMesh", 32u, 16u, MeshFlags::Position);

    EXPECT_TRUE(Meshes::has(mesh_ID));
    EXPECT_EQ(32u, Meshes::get_index_count(mesh_ID));
    EXPECT_NE(nullptr, Meshes::get_indices(mesh_ID));
    EXPECT_EQ(16u, Meshes::get_vertex_count(mesh_ID));
    EXPECT_NE(nullptr, Meshes::get_positions(mesh_ID));
    EXPECT_EQ(nullptr, Meshes::get_normals(mesh_ID));
    EXPECT_EQ(nullptr, Meshes::get_texcoords(mesh_ID));

    // Test mesh created notification.
    Core::Iterable<Meshes::ChangedIterator> changed_meshes = Meshes::get_changed_meshes();
    EXPECT_EQ(1, changed_meshes.end() - changed_meshes.begin());
    EXPECT_EQ(mesh_ID, *changed_meshes.begin());
    EXPECT_EQ(Meshes::Changes::Created, Meshes::get_changes(mesh_ID));
}

TEST_F(Assets_Mesh, destroy) {
    Meshes::UID mesh_ID = Meshes::create("TestMesh", 32u, 16u);
    EXPECT_TRUE(Meshes::has(mesh_ID));

    Meshes::reset_change_notifications();

    Meshes::destroy(mesh_ID);
    EXPECT_FALSE(Meshes::has(mesh_ID));

    // Test mesh destroyed notification.
    Core::Iterable<Meshes::ChangedIterator> changed_meshes = Meshes::get_changed_meshes();
    EXPECT_EQ(1, changed_meshes.end() - changed_meshes.begin());
    EXPECT_EQ(mesh_ID, *changed_meshes.begin());
    EXPECT_EQ(Meshes::Changes::Destroyed, Meshes::get_changes(mesh_ID));
}

TEST_F(Assets_Mesh, create_and_destroy_notifications) {
    Meshes::UID mesh_ID0 = Meshes::create("TestMesh0", 32u, 16u);
    Meshes::UID mesh_ID1 = Meshes::create("TestMesh1", 32u, 16u);
    EXPECT_TRUE(Meshes::has(mesh_ID0));
    EXPECT_TRUE(Meshes::has(mesh_ID1));

    { // Test mesh create notifications.
        Core::Iterable<Meshes::ChangedIterator> changed_meshes = Meshes::get_changed_meshes();
        EXPECT_EQ(2u, changed_meshes.end() - changed_meshes.begin());

        bool mesh0_created = false;
        bool mesh1_created = false;
        bool other_changes = false;
        for (const Meshes::UID mesh_ID : changed_meshes) {
            bool mesh_created = Meshes::get_changes(mesh_ID) == Meshes::Changes::Created;
            if (mesh_ID == mesh_ID0 && mesh_created)
                mesh0_created = true;
            else if (mesh_ID == mesh_ID1 && mesh_created)
                mesh1_created = true;
            else
                other_changes = true;
        }

        EXPECT_TRUE(mesh0_created);
        EXPECT_TRUE(mesh1_created);
        EXPECT_FALSE(other_changes);
    }

    Meshes::reset_change_notifications();

    { // Test destroy.
        Meshes::destroy(mesh_ID0);
        EXPECT_FALSE(Meshes::has(mesh_ID0));

        Core::Iterable<Meshes::ChangedIterator> changed_meshes = Meshes::get_changed_meshes();
        EXPECT_EQ(1, changed_meshes.end() - changed_meshes.begin());

        bool mesh0_destroyed = false;
        bool other_changes = false;
        for (const Meshes::UID mesh_ID : changed_meshes) {
            if (mesh_ID == mesh_ID0 && Meshes::get_changes(mesh_ID) == Meshes::Changes::Destroyed)
                mesh0_destroyed = true;
            else
                other_changes = true;
        }

        EXPECT_TRUE(mesh0_destroyed);
        EXPECT_FALSE(other_changes);
    }

    Meshes::reset_change_notifications();

    { // Test that destroyed mesh cannot be destroyed again.
        EXPECT_FALSE(Meshes::has(mesh_ID0));
        
        Meshes::destroy(mesh_ID0);
        EXPECT_FALSE(Meshes::has(mesh_ID0));
        EXPECT_TRUE(Meshes::get_changed_meshes().is_empty());
    }
}

} // NS Assets
} // NS Cogwheel

#endif // _COGWHEEL_ASSETS_MESH_TEST_H_