#include "Core/Context.hpp"
#include "Core3D/Model.hpp"
#include "Navigation/NavMesh.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Util/Logger.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

glm::mat4 BuildMapTransform() {
    constexpr float SCALE = 0.02f;
    glm::mat4 transform(1.0f);
    transform = glm::scale(transform, glm::vec3(SCALE));
    transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    return transform;
}

bool ParseVec3(int startIndex, int argc, char** argv, glm::vec3& out) {
    if (startIndex + 2 >= argc) {
        return false;
    }

    out.x = std::strtof(argv[startIndex + 0], nullptr);
    out.y = std::strtof(argv[startIndex + 1], nullptr);
    out.z = std::strtof(argv[startIndex + 2], nullptr);
    return true;
}

void PrintUsage() {
    std::cout << "Usage: CSNavProbe <startX> <startY> <startZ> <goalX> <goalY> <goalZ>\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 7) {
        PrintUsage();
        return 1;
    }

    glm::vec3 start(0.0f);
    glm::vec3 goal(0.0f);
    if (!ParseVec3(1, argc, argv, start) || !ParseVec3(4, argc, argv, goal)) {
        PrintUsage();
        return 1;
    }

    auto context = Core::Context::GetInstance();
    (void)context;
    Util::Logger::SetLevel(Util::Logger::Level::DEBUG);

    const std::string mapPath = std::string(ASSETS_DIR) + "/de_dust2-map/source/de_dust2.obj";
    Core3D::Model mapModel(mapPath, false);

    Physics::CollisionMesh collisionMesh;
    collisionMesh.Build(mapModel, BuildMapTransform());

    Navigation::NavMesh navMesh;
    navMesh.Build(collisionMesh);

    auto path = navMesh.FindPath(start, goal, collisionMesh);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "nav_nodes=" << navMesh.GetNodeCount() << "\n";
    std::cout << "start=(" << start.x << ", " << start.y << ", " << start.z << ")\n";
    std::cout << "goal=(" << goal.x << ", " << goal.y << ", " << goal.z << ")\n";
    std::cout << "path_nodes=" << path.size() << "\n";

    for (size_t i = 0; i < path.size(); ++i) {
        const auto& waypoint = path[i];
        std::cout << "  [" << i << "] ("
                  << waypoint.x << ", " << waypoint.y << ", " << waypoint.z << ")\n";
    }

    return path.empty() ? 2 : 0;
}