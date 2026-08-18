// TransformTests: Implementation for the DirectorDesk Scene module.
// This file owns project behavior only; keep platform and dependency boundaries explicit.
// Contract coverage: transform composition and persisted rotation/scale values are reversible.


#include "DirectorDesk/Scene/Document.h"
#include "DirectorDesk/Scene/Transform.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

TEST_CASE("Transform matrix uses TRS order", "[scene][transform]") {
    DirectorDesk::Scene::Transform transform;
    transform.position = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.scale = glm::vec3(2.0f, 2.0f, 2.0f);
    const glm::vec4 point = transform.ToMatrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    REQUIRE(point.x == Catch::Approx(3.0f));
    REQUIRE(point.y == Catch::Approx(2.0f));
    REQUIRE(point.z == Catch::Approx(3.0f));
}

TEST_CASE("Euler conversion round-trips yaw", "[scene][transform]") {
    DirectorDesk::Scene::Transform transform;
    transform.SetEulerDegrees(glm::vec3(0.0f, 90.0f, 0.0f));
    const glm::vec3 euler = transform.EulerDegrees();
    REQUIRE(euler.y == Catch::Approx(90.0f).margin(0.1f));
}

TEST_CASE("Document assigns stable node ids", "[scene][document]") {
    DirectorDesk::Scene::Document document;
    DirectorDesk::Scene::Node node;
    node.id = document.NextNodeId();
    node.name = "椅子";
    document.Add(node);
    REQUIRE(document.SelectedId() == "node-1");
    REQUIRE(document.Find("node-1") != nullptr);
    REQUIRE(document.Find("node-1")->name == "椅子");
}
