#include "shape.hpp"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------- shape_t ----------------
shape_t::shape_t(unsigned int tessLevel)
    : level(tessLevel), VAO(0), VBO(0), buffers_initialized(false) {
    if (level > 4) level = 4; // clamp
}

shape_t::~shape_t() {
    if (buffers_initialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
}

void shape_t::setup_buffers() {
    if (buffers_initialized) return;
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // Store vertex positions only (3 floats per vertex)
    std::vector<float> buffer_data;
    for (const auto& vertex : vertices) {
        buffer_data.push_back(vertex.x);
        buffer_data.push_back(vertex.y);
        buffer_data.push_back(vertex.z);
    }
    
    glBufferData(GL_ARRAY_BUFFER, buffer_data.size() * sizeof(float), buffer_data.data(), GL_STATIC_DRAW);
    
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    buffers_initialized = true;
}

// ---------------- sphere_t ----------------
sphere_t::sphere_t(unsigned int tessLevel)
    : shape_t(tessLevel) {
    shapetype = SPHERE_SHAPE;
    makeSphere();
    setup_buffers();
}

void sphere_t::makeSphere() {
    vertices.clear();
    colors.clear();

    unsigned int stacks = 6 * (1u << level);
    unsigned int slices = 6 * (1u << level);
    if (stacks < 6) stacks = 6;
    if (slices < 6) slices = 6;

    for (unsigned int i = 0; i < stacks; i++) {
        float phi1 = M_PI * i / stacks;
        float phi2 = M_PI * (i+1) / stacks;

        for (unsigned int j = 0; j < slices; j++) {
            float theta1 = 2*M_PI * j / slices;
            float theta2 = 2*M_PI * (j+1) / slices;

            glm::vec4 p1( sin(phi1)*cos(theta1), cos(phi1), sin(phi1)*sin(theta1), 1.0f );
            glm::vec4 p2( sin(phi2)*cos(theta1), cos(phi2), sin(phi2)*sin(theta1), 1.0f );
            glm::vec4 p3( sin(phi2)*cos(theta2), cos(phi2), sin(phi2)*sin(theta2), 1.0f );
            glm::vec4 p4( sin(phi1)*cos(theta2), cos(phi1), sin(phi1)*sin(theta2), 1.0f );

            // two triangles per quad
            vertices.push_back(p1); colors.push_back(glm::vec4(1,0,0,1));
            vertices.push_back(p2); colors.push_back(glm::vec4(0,1,0,1));
            vertices.push_back(p3); colors.push_back(glm::vec4(0,0,1,1));

            vertices.push_back(p1); colors.push_back(glm::vec4(1,0,0,1));
            vertices.push_back(p3); colors.push_back(glm::vec4(0,0,1,1));
            vertices.push_back(p4); colors.push_back(glm::vec4(1,1,0,1));
        }
    }
}

void sphere_t::draw() const {
    if (!buffers_initialized) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

// ---------------- cylinder_t ----------------
cylinder_t::cylinder_t(unsigned int tessLevel)
    : shape_t(tessLevel) {
    shapetype = CYLINDER_SHAPE;
    makeCylinder();
    setup_buffers();
}

void cylinder_t::makeCylinder() {
    vertices.clear();
    colors.clear();

    unsigned int slices = 8 * (1u << level);
    if (slices < 8) slices = 8;
    float height = 1.0f;
    float radius = 0.5f;

    // Side faces
    for (unsigned int i=0; i<slices; i++) {
        float theta1 = 2*M_PI*i/slices;
        float theta2 = 2*M_PI*(i+1)/slices;
        glm::vec4 p1(radius*cos(theta1), -height/2, radius*sin(theta1), 1.0f);
        glm::vec4 p2(radius*cos(theta2), -height/2, radius*sin(theta2), 1.0f);
        glm::vec4 p3(radius*cos(theta2),  height/2, radius*sin(theta2), 1.0f);
        glm::vec4 p4(radius*cos(theta1),  height/2, radius*sin(theta1), 1.0f);

        // side quad as two triangles
        vertices.push_back(p1); colors.push_back(glm::vec4(1,0,0,1));
        vertices.push_back(p2); colors.push_back(glm::vec4(0,1,0,1));
        vertices.push_back(p3); colors.push_back(glm::vec4(0,0,1,1));

        vertices.push_back(p1); colors.push_back(glm::vec4(1,0,0,1));
        vertices.push_back(p3); colors.push_back(glm::vec4(0,0,1,1));
        vertices.push_back(p4); colors.push_back(glm::vec4(1,1,0,1));
    }
    
    // caps
    glm::vec4 centerBottom(0,-height/2,0,1), centerTop(0,height/2,0,1);
    for (unsigned int i=0; i<slices; i++) {
        float t1=2*M_PI*i/slices, t2=2*M_PI*(i+1)/slices;
        glm::vec4 b1(radius*cos(t1), -height/2, radius*sin(t1),1);
        glm::vec4 b2(radius*cos(t2), -height/2, radius*sin(t2),1);
        vertices.push_back(centerBottom); colors.push_back(glm::vec4(1,0,1,1));
        vertices.push_back(b1); colors.push_back(glm::vec4(0,1,1,1));
        vertices.push_back(b2); colors.push_back(glm::vec4(1,1,1,1));
        
        glm::vec4 t1v(radius*cos(t1), height/2, radius*sin(t1),1);
        glm::vec4 t2v(radius*cos(t2), height/2, radius*sin(t2),1);
        vertices.push_back(centerTop); colors.push_back(glm::vec4(1,0,1,1));
        vertices.push_back(t2v); colors.push_back(glm::vec4(0,1,1,1));
        vertices.push_back(t1v); colors.push_back(glm::vec4(1,1,1,1));
    }
}

void cylinder_t::draw() const {
    if (!buffers_initialized) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

// ---------------- box_t ----------------
box_t::box_t(unsigned int tessLevel)
    : shape_t(tessLevel) {
    shapetype = BOX_SHAPE;
    makeBox();
    setup_buffers();
}

void box_t::makeBox() {
    vertices.clear();
    colors.clear();

    float h=0.5f;
    glm::vec4 pts[8] = {
        {-h,-h,-h,1}, {h,-h,-h,1}, {h,h,-h,1}, {-h,h,-h,1},
        {-h,-h,h,1},  {h,-h,h,1},  {h,h,h,1},  {-h,h,h,1}
    };
    int faces[6][4] = {
        {0,1,2,3}, {4,7,6,5}, {0,4,5,1},
        {2,6,7,3}, {0,3,7,4}, {1,5,6,2}
    };
    glm::vec4 cols[6] = {
        {1,0,0,1},{0,1,0,1},{0,0,1,1},
        {1,1,0,1},{0,1,1,1},{1,0,1,1}
    };
    
    for(int f=0; f<6; f++) {
        int* q = faces[f];
        vertices.push_back(pts[q[0]]); colors.push_back(cols[f]);
        vertices.push_back(pts[q[1]]); colors.push_back(cols[f]);
        vertices.push_back(pts[q[2]]); colors.push_back(cols[f]);

        vertices.push_back(pts[q[0]]); colors.push_back(cols[f]);
        vertices.push_back(pts[q[2]]); colors.push_back(cols[f]);
        vertices.push_back(pts[q[3]]); colors.push_back(cols[f]);
    }
}

void box_t::draw() const {
    if (!buffers_initialized) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

// ---------------- cone_t ----------------
cone_t::cone_t(unsigned int tessLevel)
    : shape_t(tessLevel) {
    shapetype = CONE_SHAPE;
    makeCone();
    setup_buffers();
}

void cone_t::makeCone() {
    vertices.clear();
    colors.clear();

    unsigned int slices = 8 * (1u << level);
    if (slices < 8) slices = 8;
    float radius = 0.5f;
    float height = 1.0f;
    glm::vec4 apex(0,height/2,0,1);
    glm::vec4 center(0,-height/2,0,1);

    for (unsigned int i=0; i<slices; i++) {
        float t1=2*M_PI*i/slices, t2=2*M_PI*(i+1)/slices;
        glm::vec4 b1(radius*cos(t1), -height/2, radius*sin(t1),1);
        glm::vec4 b2(radius*cos(t2), -height/2, radius*sin(t2),1);
        
        // side triangles
        vertices.push_back(apex); colors.push_back(glm::vec4(1,0,0,1));
        vertices.push_back(b1);   colors.push_back(glm::vec4(0,1,0,1));
        vertices.push_back(b2);   colors.push_back(glm::vec4(0,0,1,1));
        
        // base triangles
        vertices.push_back(center); colors.push_back(glm::vec4(1,1,0,1));
        vertices.push_back(b2);     colors.push_back(glm::vec4(0,1,1,1));
        vertices.push_back(b1);     colors.push_back(glm::vec4(1,0,1,1));
    }
}

void cone_t::draw() const {
    if (!buffers_initialized) return;
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}