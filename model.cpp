#include "model.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>


model_node::model_node(shape_t* s, model_node* p)
    : shape(s), 
      translation(glm::mat4(1.0f)),
      rotation(glm::mat4(1.0f)),
      scale(glm::mat4(1.0f)),
      color(1.0f,1.0f,1.0f,1.0f),
      parent(p)
{
    if (parent) parent->add_child(this);
}

model_node::~model_node() {
    for (auto c : children) delete c;
    children.clear();
}

void model_node::add_child(model_node* c) {
    children.push_back(c);
    c->parent = this;
}

void model_node::remove_child(model_node* c) {
    children.erase(std::remove(children.begin(), children.end(), c), children.end());
}

glm::mat4 model_node::local_matrix() const {
    return translation * rotation * scale;
}

glm::mat4 model_node::get_world_matrix() const {
    if (parent) {
        return parent->get_world_matrix() * local_matrix();
    }
    return local_matrix();
}

void model_node::collect(std::vector<model_node*>& out) {
    out.push_back(this);
    for (auto c : children) c->collect(out);
}

// ---------------- model_t ----------------

model_t::model_t() {
    root = new model_node(nullptr, nullptr);
    all_nodes.push_back(root);
}

model_t::~model_t() {
    delete root;
    for (auto s : owned_shapes) delete s;
    owned_shapes.clear();
    all_nodes.clear();
}

model_node* model_t::create_sphere(unsigned int level, model_node* parent) {
    if (!parent) parent = root;
    shape_t* s = new sphere_t(level);
    owned_shapes.push_back(s);
    auto node = new model_node(s, parent);
    all_nodes.push_back(node);
    return node;
}

model_node* model_t::create_cylinder(unsigned int level, model_node* parent) {
    if (!parent) parent = root;
    shape_t* s = new cylinder_t(level);
    owned_shapes.push_back(s);
    auto node = new model_node(s, parent);
    all_nodes.push_back(node);
    return node;
}

model_node* model_t::create_box(model_node* parent) {
    if (!parent) parent = root;
    shape_t* s = new box_t();
    owned_shapes.push_back(s);
    auto node = new model_node(s, parent);
    all_nodes.push_back(node);
    return node;
}

model_node* model_t::create_cone(unsigned int level, model_node* parent) {
    if (!parent) parent = root;
    shape_t* s = new cone_t(level);
    owned_shapes.push_back(s);
    auto node = new model_node(s, parent);
    all_nodes.push_back(node);
    return node;
}

void model_t::remove_node(model_node* node) {
    if (!node || node == root) return;

    if (node->parent) {
        node->parent->remove_child(node);
    }

    // Remove from all_nodes list
    all_nodes.erase(std::remove(all_nodes.begin(), all_nodes.end(), node), all_nodes.end());
    
    // Remove children from all_nodes as well
    std::vector<model_node*> to_remove;
    node->collect(to_remove);
    for (auto n : to_remove) {
        if (n != node) {
            all_nodes.erase(std::remove(all_nodes.begin(), all_nodes.end(), n), all_nodes.end());
        }
    }

    delete node;
}

bool model_t::save_to_file(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out) return false;

    // Format: type parent_index tx ty tz rx ry rz rw sx sy sz r g b
    // Using quaternion for rotation (simplified)
    std::vector<model_node*> nodes;
    root->collect(nodes);

    for (size_t i=0; i<nodes.size(); i++) {
        model_node* n = nodes[i];
        int parentIdx = -1;
        if (n->parent) {
            for (size_t j=0;j<nodes.size();j++) {
                if (nodes[j] == n->parent) { parentIdx = (int)j; break; }
            }
        }
        int type = -1;
        if (n->shape) type = n->shape->shapetype;

        // Extract transforms (simplified - just translation and scale diagonal)
        glm::vec3 t(n->translation[3]);
        glm::vec3 s(n->scale[0][0], n->scale[1][1], n->scale[2][2]);

        out << type << " " << parentIdx << " "
            << t.x << " " << t.y << " " << t.z << " "
            << "0 0 0 1 "  // simplified rotation as identity quaternion
            << s.x << " " << s.y << " " << s.z << " "
            << n->color.r << " " << n->color.g << " " << n->color.b << "\n";
    }

    return true;
}

bool model_t::load_from_file(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) return false;

    // cleanup old
    delete root;
    for (auto s : owned_shapes) delete s;
    owned_shapes.clear();
    all_nodes.clear();

    root = new model_node(nullptr,nullptr);
    all_nodes.push_back(root);

    std::vector<model_node*> nodes;
    nodes.push_back(root);

    std::string line;
    while (std::getline(in,line)) {
        std::istringstream iss(line);
        int type, parentIdx;
        float tx,ty,tz, rx,ry,rz,rw, sx,sy,sz, cr,cg,cb;
        if (!(iss >> type >> parentIdx >> tx >>ty >>tz >> rx>>ry>>rz>>rw >> sx >>sy >>sz >> cr >>cg >>cb))
            continue;

        shape_t* s = nullptr;
        switch(type) {
            case SPHERE_SHAPE: s = new sphere_t(1); break;
            case CYLINDER_SHAPE: s = new cylinder_t(1); break;
            case BOX_SHAPE: s = new box_t(); break;
            case CONE_SHAPE: s = new cone_t(1); break;
        }
        if (s) owned_shapes.push_back(s);

        model_node* parent = (parentIdx >=0 && parentIdx < (int)nodes.size())
                             ? nodes[parentIdx] : root;

        model_node* node = new model_node(s,parent);
        node->translation = glm::translate(glm::mat4(1.0f), glm::vec3(tx,ty,tz));
        node->scale = glm::scale(glm::mat4(1.0f), glm::vec3(sx,sy,sz));
        node->color = glm::vec4(cr,cg,cb,1.0f);

        nodes.push_back(node);
        all_nodes.push_back(node);
    }

    return true;
}

void model_t::debug_print() const {
    std::vector<model_node*> nodes;
    root->collect(nodes);
    for (size_t i=0;i<nodes.size();i++) {
        auto n = nodes[i];
        std::cout<<"Node "<<i<<" type="<<(n->shape? n->shape->shapetype:-1)
                 <<" children="<<n->children.size()<<"\n";
    }
}