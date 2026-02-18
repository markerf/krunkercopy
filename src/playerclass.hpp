#include "playerInput.h"
#include <GL/glew.h>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <modelloader.h>

class Player {
public: 
    ModelLoader playermodel;
    unsigned int health;   
    glm::vec3 pos;
    
    // Add constructor to initialize everything!
    Player() 
        : health(100), 
          pos(glm::vec3(0.0f, 0.0f, 0.0f))
    {
    }
    
    void updatehealth(unsigned int dmg)
    {
        if (health > dmg)
            health -= dmg;
        else
            health = 0;
    }
    
    void updatepos(glm::vec3 newpos)
    {
        pos = newpos;
    } 
};
