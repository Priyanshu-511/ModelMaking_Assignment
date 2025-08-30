#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

enum ShapeType {
    SPHERE_SHAPE,
    CYLINDER_SHAPE,
    BOX_SHAPE,
    CONE_SHAPE
};

class shape_t {
public:
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> colors;
    ShapeType shapetype;
    unsigned int level;
    
    // OpenGL buffers
    GLuint VAO, VBO;
    bool buffers_initialized;

    shape_t(unsigned int tessLevel);
    virtual ~shape_t();

    virtual void draw() const = 0;
    
protected:
    void setup_buffers();  // Setup VAO/VBO for modern OpenGL
};

// Derived shapes
class sphere_t : public shape_t {
public:
    sphere_t(unsigned int tessLevel = 0);
    void draw() const override;
private:
    void makeSphere();
};

class cylinder_t : public shape_t {
public:
    cylinder_t(unsigned int tessLevel = 0);
    void draw() const override;
private:
    void makeCylinder();
};

class box_t : public shape_t {
public:
    box_t(unsigned int tessLevel = 0);
    void draw() const override;
private:
    void makeBox();
};

class cone_t : public shape_t {
public:
    cone_t(unsigned int tessLevel = 0);
    void draw() const override;
private:
    void makeCone();
};

#endif