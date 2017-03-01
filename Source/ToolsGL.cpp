//
//  ToolsGL.cpp
//  spatServerGRIS
//
//  Created by GRIS on 2017-03-01.
//
//

#include "ToolsGL.h"

float ToolsGL::Raycast(Ray ray, SpeakerObj speaker) {
    float t1 = (speaker.getMin().x - ray.getPosition().x) / ray.getNormal().x;
    float t2 = (speaker.getMax().x - ray.getPosition().x) / ray.getNormal().x;
    float t3 = (speaker.getMin().y - ray.getPosition().y) / ray.getNormal().y;
    float t4 = (speaker.getMax().y - ray.getPosition().y) / ray.getNormal().y;
    float t5 = (speaker.getMin().z - ray.getPosition().z) / ray.getNormal().z;
    float t6 = (speaker.getMax().z - ray.getPosition().z) / ray.getNormal().z;
    
    float tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
    float tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));
    
    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0) {
        return -1;
    }
    
    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return -1;
    }
    
    if (tmin < 0.0f) {
        return tmax;
    }
    return tmin;
}

void ToolsGL::printMatrix(glm::vec3 m){
    cout << "["<< m.x << " . "<<  m.y << " . "<<  m.z << "]" << endl;
}
