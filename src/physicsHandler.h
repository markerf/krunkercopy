#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <modelloader.h>


struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
struct MoveResult {
    glm::vec3 position;
    glm::vec3 velocity;
    bool onGround;
};

class PhysicsHandler {
public:
    std::vector<AABB> worldAABBs;  // all static world geometry

    // called once at startup, feed it the meshes from ModelLoader
    void buildWorld(const std::vector<Mesh>& meshes) {
        for (auto& mesh : meshes) {
            AABB box;
            box.min = mesh.AABBmin;
            box.max = mesh.AABBmax;
            glm::vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z},
        {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z},
        {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z},
        {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z},
        {box.max.x, box.max.y, box.max.z},
        };

        glm::vec3 newMin(FLT_MAX);
        glm::vec3 newMax(-FLT_MAX);

        for (auto& corner : corners) {
            // apply the world transform to each corner
            glm::vec3 transformed = glm::vec3(mesh.worldtransform * glm::vec4(corner, 1.0f));
            newMin = glm::min(newMin, transformed);
            newMax = glm::max(newMax, transformed);
    }  

            box.min = newMin;
            box.max = newMax; 

            worldAABBs.push_back(box);
        }
    }

    bool collidesWithWorld(const AABB& player) {
        for (auto& box : worldAABBs) {
            if (AABBvsAABB(player, box))
                return true;
        }
        return false;
    }

    MoveResult resolveMovement(glm::vec3 currentPos, glm::vec3 velocity) {
        glm::vec3 newPos = currentPos + velocity;
        bool onGround = false;
        // test X
        if (collidesWithWorld(getAABB({ newPos.x, currentPos.y, currentPos.z }))){
            newPos.x = currentPos.x;
            velocity.x = 0.0f;
        }
        // test Y
        if (collidesWithWorld(getAABB({ newPos.x, newPos.y, currentPos.z }))){
            newPos.y = currentPos.y;
            velocity.y = 0.0f;
            onGround = true;
        }
        // test Z
        if (collidesWithWorld(getAABB({ newPos.x, newPos.y, newPos.z }))){
            newPos.z = currentPos.z;
            velocity.z = 0.0f;
        }


        return { newPos, velocity,onGround };
    }

private:
    // builds a player sized AABB around a position
    AABB getAABB(glm::vec3 pos) {
        return {
            pos + glm::vec3(-0.4f, -0.9f, -0.4f),
            pos + glm::vec3( 0.4f, 0.9f,  0.4f)
        };
    }

    bool AABBvsAABB(const AABB& a, const AABB& b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }
};
