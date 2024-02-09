#pragma once

#include <vector>

#include "util/vector_math.h"

#define GLSL(SRC) "#version 330 core\n" # SRC

struct GLFWwindow;
struct TextBuffer;

struct Texture
{
    u32 handle;
    vec2i size;
    vec2i grid_size;
};

struct VertexBuffer
{
    struct DrawCommandData
    {
        u32 mode;
        u32 count;
        u32 offset;
        u32 base;

        DrawCommandData(u32 mode, u32 count, u32 offset, u32 base)
            : mode(mode), count(count), offset(offset), base(base)
        {
        }
    };

    u32 vao;
    u32 vbo;
    u32 vboi;

    std::vector<float> vertices;
    std::vector<u16> indices;

    u32 next_index;
    u32 index_offset;

    u32 last_shader;

    std::vector<DrawCommandData> commands;
};

struct Window
{
    GLFWwindow* window;

    u32 framebuffer;
    u32 framebuffer_color;
    u32 framebuffer_depth;

    int width, height;

    u32 shader;
    
    Texture* font_texture;

    VertexBuffer* vb;
};
extern Window g_window;

void window_open(const char* title, int w, int h);
bool frame_start();
void frame_end();

void vb_push_vertex(VertexBuffer* vb, vec4f p, vec4f n, vec4f c, vec2f t);
void vb_push_draw(VertexBuffer* vb, u32 type, std::initializer_list<int> idx);
void vb_push_quad(VertexBuffer* vb, vec2f p0, vec2f p1, vec4f color, vec2f t0, vec2f t1);
void vb_init(VertexBuffer* vb);
void vb_clear(VertexBuffer* vb);
void vb_upload(VertexBuffer* vb);
void vb_draw(VertexBuffer* vb);

void render_buffer(TextBuffer* term);

Texture* texture_load(const char* name, vec2i grid_size=vec2i(0,0));

u32 shader_load(const char* vtx, const char* frag);
