#ifndef MODEL_HPP
#define MODEL_HPP

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "shape.hpp"

// A hierarchical model node
struct model_node {
    shape_t* shape;             // may be null
    glm::mat4 translation;
    glm::mat4 rotation;
    glm::mat4 scale;
    glm::vec4 color;
    model_node* parent;
    std::vector<model_node*> children;

    model_node(shape_t* s = nullptr, model_node* p = nullptr);
    ~model_node();

    void add_child(model_node* c);
    void remove_child(model_node* c);

    glm::mat4 local_matrix() const;
    glm::mat4 get_world_matrix() const;  // Added: computes world transform
    void collect(std::vector<model_node*>& out);
};

class model_t {
public:
    model_node* root;
    std::vector<model_node*> all_nodes; // flat list for bookkeeping
    std::vector<shape_t*> owned_shapes;

    model_t();
    ~model_t();

    model_node* create_sphere(unsigned int level = 1, model_node* parent = nullptr);
    model_node* create_cylinder(unsigned int level = 1, model_node* parent = nullptr);
    model_node* create_box(model_node* parent = nullptr);
    model_node* create_cone(unsigned int level = 1, model_node* parent = nullptr);

    void remove_node(model_node* node);

    bool save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);

    void debug_print() const;
};

#endif // MODEL_HPP