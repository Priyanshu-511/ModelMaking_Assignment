#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <fstream>
#include <sstream>

#include "model.hpp"
#include "shape.hpp"

using namespace std;

int win_w = 800, win_h = 600;

// App modes
enum AppMode { MODE_NONE=0, MODE_MODELLING, MODE_INSPECTION };
AppMode app_mode = MODE_NONE;

// transform modes
enum TransformMode { TM_NONE=0, TM_ROT, TM_TRANS, TM_SCALE };
TransformMode trans_mode = TM_NONE;
int axis_mode = 0; // 1=X,2=Y,3=Z

// current model & node
model_t* current_model = nullptr;
model_node* current_node = nullptr;

// camera
glm::mat4 view_matrix;
glm::mat4 proj_matrix;
glm::vec3 camera_pos(0.0f, 0.0f, 6.0f);
glm::vec3 camera_target(0.0f, 0.0f, 0.0f);

// shader program
GLuint shader_program = 0;
GLint uniform_mvp = -1;
GLint uniform_color = -1;

// Vertex and fragment shader source
const char* vertex_shader_source = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char* fragment_shader_source = R"(
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
)";

// Compile shader
GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        cout << "Shader compilation failed: " << info_log << endl;
        return 0;
    }
    return shader;
}

// Create shader program
bool create_shader_program() {
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    
    if (!vertex_shader || !fragment_shader) {
        return false;
    }
    
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    
    GLint success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        cout << "Shader program linking failed: " << info_log << endl;
        return false;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    uniform_mvp = glGetUniformLocation(shader_program, "uMVP");
    uniform_color = glGetUniformLocation(shader_program, "uColor");
    
    return true;
}

// helper: ensure model exists
static void ensure_model() {
    if (!current_model) {
        current_model = new model_t();
        current_node = nullptr;
        cout << "Created new model\n";
    }
}

// print current app mode to terminal
static void print_mode() {
    if (app_mode == MODE_MODELLING) cout << "Mode: MODELLING\n";
    else if (app_mode == MODE_INSPECTION) cout << "Mode: INSPECTION\n";
    else cout << "Mode: NONE\n";
}

// compute centroid of a node's shape
static glm::vec3 compute_shape_centroid(model_node* node) {
    glm::vec3 centroid(0.0f);
    if (!node || !node->shape) return centroid;
    const auto &verts = node->shape->vertices;
    if (verts.empty()) return centroid;
    for (auto &v : verts) centroid += glm::vec3(v.x, v.y, v.z);
    centroid /= (float)verts.size();
    return centroid;
}

// compute centroid of whole model
static glm::vec3 compute_model_centroid(model_t* model) {
    glm::vec3 centroid(0.0f);
    if (!model || !model->root) return centroid;
    std::vector<model_node*> nodes;
    model->root->collect(nodes);
    int count = 0;
    for (auto n : nodes) {
        if (n->shape && !n->shape->vertices.empty()) {
            glm::vec3 c = compute_shape_centroid(n);
            glm::vec4 tc = n->get_world_matrix() * glm::vec4(c, 1.0f);
            centroid += glm::vec3(tc);
            ++count;
        }
    }
    if (count > 0) centroid /= (float)count;
    return centroid;
}

// keyboard handler
static void handle_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods; // suppress warnings
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    // toggle modes
    if (key == GLFW_KEY_M) {
        app_mode = MODE_MODELLING;
        print_mode();
        return;
    }
    if (key == GLFW_KEY_I) {
        app_mode = MODE_INSPECTION;
        print_mode();
        return;
    }

    // MODE: MODELLING
    if (app_mode == MODE_MODELLING) {
        // 1-4 add shapes
        if (key == GLFW_KEY_1) {
            ensure_model();
            current_node = current_model->create_sphere(1, current_model->root);
            cout << "Added sphere (current shape updated)\n";
            current_model->debug_print();
            return;
        }
        if (key == GLFW_KEY_2) {
            ensure_model();
            current_node = current_model->create_cylinder(1, current_model->root);
            cout << "Added cylinder (current shape updated)\n";
            current_model->debug_print();
            return;
        }
        if (key == GLFW_KEY_3) {
            ensure_model();
            current_node = current_model->create_box(current_model->root);
            cout << "Added box (current shape updated)\n";
            current_model->debug_print();
            return;
        }
        if (key == GLFW_KEY_4) {
            ensure_model();
            current_node = current_model->create_cone(1, current_model->root);
            cout << "Added cone (current shape updated)\n";
            current_model->debug_print();
            return;
        }

        // 5 remove last added shape
        if (key == GLFW_KEY_5) {
            if (!current_node || !current_model) {
                cout << "No current node to remove\n";
                return;
            }
            if (current_node == current_model->root) {
                cout << "Cannot remove root node\n";
                return;
            }
            model_node* newcur = nullptr;
            auto &alist = current_model->all_nodes;
            int idx = -1;
            for (int i = (int)alist.size()-1; i>=0; --i) {
                if (alist[i] == current_node) { idx = i; break; }
            }
            if (idx > 1) newcur = alist[idx-1];
            current_model->remove_node(current_node);
            current_node = newcur;
            cout << "Removed selected node\n";
            current_model->debug_print();
            return;
        }

        // transform mode selection
        if (key == GLFW_KEY_R) { trans_mode = TM_ROT; cout << "Rotation mode activated\n"; return; }
        if (key == GLFW_KEY_T) { trans_mode = TM_TRANS; cout << "Translation mode activated\n"; return; }
        if (key == GLFW_KEY_G) { trans_mode = TM_SCALE; cout << "Scaling mode activated\n"; return; }

        // axis selection
        if (key == GLFW_KEY_X) { axis_mode = 1; cout << "Axis X selected\n"; return; }
        if (key == GLFW_KEY_Y) { axis_mode = 2; cout << "Axis Y selected\n"; return; }
        if (key == GLFW_KEY_Z) { axis_mode = 3; cout << "Axis Z selected\n"; return; }

        // apply transforms
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
            if (!current_node) { cout << "No current node selected\n"; return; }
            if (trans_mode == TM_ROT) {
                float ang = glm::radians(10.0f);
                glm::vec3 cen = compute_shape_centroid(current_node);
                glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -cen);
                glm::mat4 T2 = glm::translate(glm::mat4(1.0f), cen);
                if (axis_mode == 1) current_node->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(1,0,0)) * T1 * current_node->rotation;
                else if (axis_mode == 2) current_node->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,1,0)) * T1 * current_node->rotation;
                else if (axis_mode == 3) current_node->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,0,1)) * T1 * current_node->rotation;
                cout << "Rotated current shape +10 degrees\n";
            } else if (trans_mode == TM_TRANS) {
                float d = 0.1f;
                if (axis_mode == 1) current_node->translation = glm::translate(current_node->translation, glm::vec3(d,0,0));
                else if (axis_mode == 2) current_node->translation = glm::translate(current_node->translation, glm::vec3(0,d,0));
                else if (axis_mode == 3) current_node->translation = glm::translate(current_node->translation, glm::vec3(0,0,d));
                cout << "Translated current shape +0.1\n";
            } else if (trans_mode == TM_SCALE) {
                float s = 1.1f;
                glm::vec3 sv(1.0f);
                if (axis_mode == 1) sv.x = s;
                else if (axis_mode == 2) sv.y = s;
                else if (axis_mode == 3) sv.z = s;
                glm::vec3 cen = compute_shape_centroid(current_node);
                glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -cen);
                glm::mat4 T2 = glm::translate(glm::mat4(1.0f), cen);
                current_node->scale = T2 * glm::scale(glm::mat4(1.0f), sv) * T1 * current_node->scale;
                cout << "Scaled current shape by 1.1\n";
            }
            return;
        }
        if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) {
            if (!current_node) { cout << "No current node selected\n"; return; }
            if (trans_mode == TM_ROT) {
                float ang = glm::radians(-10.0f);
                glm::vec3 cen = compute_shape_centroid(current_node);
                glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -cen);
                glm::mat4 T2 = glm::translate(glm::mat4(1.0f), cen);
                if (axis_mode == 1) current_node->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(1,0,0)) * T1 * current_node->rotation;
                else if (axis_mode == 2) current_node->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,1,0)) * T1 * current_node->rotation;
                else if (axis_mode == 3) current_node->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,0,1)) * T1 * current_node->rotation;
                cout << "Rotated current shape -10 degrees\n";
            } else if (trans_mode == TM_TRANS) {
                float d = -0.1f;
                if (axis_mode == 1) current_node->translation = glm::translate(current_node->translation, glm::vec3(d,0,0));
                else if (axis_mode == 2) current_node->translation = glm::translate(current_node->translation, glm::vec3(0,d,0));
                else if (axis_mode == 3) current_node->translation = glm::translate(current_node->translation, glm::vec3(0,0,d));
                cout << "Translated current shape -0.1\n";
            } else if (trans_mode == TM_SCALE) {
                float s = 0.9f;
                glm::vec3 sv(1.0f);
                if (axis_mode == 1) sv.x = s;
                else if (axis_mode == 2) sv.y = s;
                else if (axis_mode == 3) sv.z = s;
                glm::vec3 cen = compute_shape_centroid(current_node);
                glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -cen);
                glm::mat4 T2 = glm::translate(glm::mat4(1.0f), cen);
                current_node->scale = T2 * glm::scale(glm::mat4(1.0f), sv) * T1 * current_node->scale;
                cout << "Scaled current shape by 0.9\n";
            }
            return;
        }

        // color change
        if (key == GLFW_KEY_C) {
            if (!current_node) { cout << "No current node selected\n"; return; }
            cout << "Enter R G B (0..1) separated by spaces: ";
            float r,g,b;
            if (cin >> r >> g >> b) {
                current_node->color = glm::vec4(r,g,b,1.0f);
                cout << "Updated color of current shape\n";
            } else {
                cin.clear();
                string dummy; getline(cin,dummy);
                cout << "Invalid color input\n";
            }
            return;
        }

        // save model
        if (key == GLFW_KEY_S) {
            if (!current_model) { cout << "No model to save\n"; return; }
            cout << "Enter filename to save (add .mod if needed): ";
            string fname; cin >> fname;
            if (fname.find(".mod") == string::npos) fname += ".mod";
            if (current_model->save_to_file(fname)) cout << "Saved model to " << fname << "\n";
            else cout << "Save failed\n";
            return;
        }
    }

    // MODE: INSPECTION
    if (app_mode == MODE_INSPECTION) {
        if (key == GLFW_KEY_L) {
            cout << "Enter filename to load (include .mod): ";
            string fname; cin >> fname;
            if (fname.find(".mod") == string::npos) fname += ".mod";
            if (current_model) { delete current_model; current_model = nullptr; current_node = nullptr; }
            current_model = new model_t();
            if (!current_model->load_from_file(fname)) {
                cout << "Load failed for " << fname << "\n";
                delete current_model; current_model = nullptr;
            } else {
                if (!current_model->all_nodes.empty()) current_node = current_model->all_nodes.back();
                cout << "Loaded model: " << fname << " (nodes: " << current_model->all_nodes.size() << ")\n";
                
                // Position camera to look at model centroid
                glm::vec3 mc = compute_model_centroid(current_model);
                cout << "Model centroid: " << mc.x << " " << mc.y << " " << mc.z << "\n";
                camera_target = mc;
                camera_pos = mc + glm::vec3(0, 0, 6.0f);
                view_matrix = glm::lookAt(camera_pos, camera_target, glm::vec3(0,1,0));
            }
            return;
        }

        // rotate entire model
        if (key == GLFW_KEY_R) { trans_mode = TM_ROT; cout << "Model rotation mode ON\n"; return; }
        if (key == GLFW_KEY_X) { axis_mode = 1; cout << "Axis X selected\n"; return; }
        if (key == GLFW_KEY_Y) { axis_mode = 2; cout << "Axis Y selected\n"; return; }
        if (key == GLFW_KEY_Z) { axis_mode = 3; cout << "Axis Z selected\n"; return; }

        if ((key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) && trans_mode == TM_ROT) {
            if (!current_model || !current_model->root) return;
            float ang = glm::radians(10.0f);
            glm::vec3 mc = compute_model_centroid(current_model);
            glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -mc);
            glm::mat4 T2 = glm::translate(glm::mat4(1.0f), mc);
            if (axis_mode == 1) current_model->root->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(1,0,0)) * T1 * current_model->root->rotation;
            else if (axis_mode == 2) current_model->root->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,1,0)) * T1 * current_model->root->rotation;
            else if (axis_mode == 3) current_model->root->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,0,1)) * T1 * current_model->root->rotation;
            cout << "Rotated entire model +10 deg\n";
            return;
        }
        if ((key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) && trans_mode == TM_ROT) {
            if (!current_model || !current_model->root) return;
            float ang = glm::radians(-10.0f);
            glm::vec3 mc = compute_model_centroid(current_model);
            glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -mc);
            glm::mat4 T2 = glm::translate(glm::mat4(1.0f), mc);
            if (axis_mode == 1) current_model->root->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(1,0,0)) * T1 * current_model->root->rotation;
            else if (axis_mode == 2) current_model->root->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,1,0)) * T1 * current_model->root->rotation;
            else if (axis_mode == 3) current_model->root->rotation = T2 * glm::rotate(glm::mat4(1.0f), ang, glm::vec3(0,0,1)) * T1 * current_model->root->rotation;
            cout << "Rotated entire model -10 deg\n";
            return;
        }
    }
}

// Render function
static void draw_scene() {
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!current_model || !shader_program) return;

    glUseProgram(shader_program);

    std::vector<model_node*> nodes;
    current_model->root->collect(nodes);
    
    for (auto n : nodes) {
        if (n->shape) {
            glm::mat4 model_matrix = n->get_world_matrix();
            glm::mat4 mvp = proj_matrix * view_matrix * model_matrix;
            
            glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform4fv(uniform_color, 1, glm::value_ptr(n->color));
            
            n->shape->draw();
        }
    }
}

// framebuffer callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window; // suppress warning
    win_w = width; win_h = height;
    glViewport(0,0,width,height);
    proj_matrix = glm::perspective(glm::radians(60.0f), (float)width/(float)height, 0.1f, 100.0f);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv; // suppress warnings
    if (!glfwInit()) {
        cerr << "Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(win_w, win_h, "3D Modeler Assignment", NULL, NULL);
    if (!window) {
        cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        cerr << "Failed to init GLEW\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0,0,win_w,win_h);
    glEnable(GL_DEPTH_TEST);

    // Create shader program
    if (!create_shader_program()) {
        cerr << "Failed to create shader program\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Initialize camera
    view_matrix = glm::lookAt(camera_pos, camera_target, glm::vec3(0,1,0));
    proj_matrix = glm::perspective(glm::radians(60.0f), (float)win_w/(float)win_h, 0.1f, 100.0f);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, handle_key);

    current_model = new model_t();

    cout << "3D Modeler Application\n";
    cout << "Press M for Modelling mode, I for Inspection mode. Esc to quit.\n";
    cout << "\nModelling Mode:\n";
    cout << "  1-4: Add sphere/cylinder/box/cone\n";
    cout << "  5: Remove current shape\n";
    cout << "  R/T/G: Rotation/Translation/Scale mode\n";
    cout << "  X/Y/Z: Select axis\n";
    cout << "  +/-: Apply transform\n";
    cout << "  C: Change color\n";
    cout << "  S: Save model\n";
    cout << "\nInspection Mode:\n";
    cout << "  L: Load model\n";
    cout << "  R: Rotate entire model\n";
    cout << "  X/Y/Z: Select axis\n";
    cout << "  +/-: Apply rotation\n\n";

    while (!glfwWindowShouldClose(window)) {
        draw_scene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    delete current_model;
    glDeleteProgram(shader_program);
    glfwTerminate();
    return 0;
}