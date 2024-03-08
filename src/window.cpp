#include "window.h"

#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vterm.h"

Window g_window;

vec4f makeColor(u32 col)
{
    return vec4f(
        ((col >> 16) & 0xFF) / 255.0f,
        ((col >> 8) & 0xFF) / 255.0f,
        ((col) & 0xFF) / 255.0f,
        ((col >> 24) & 0xFF) / 255.0f
    );
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    debug_assertf(false, "GLFW Error %d: %s\n", error, description);
}

void render_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (action)
    {
    case GLFW_PRESS:
        g_window.inputs.keys[key] = true;
        break;
    case GLFW_REPEAT:
        g_window.inputs.repeat[key] = true;
        break;
    case GLFW_RELEASE:
        g_window.inputs.keys[key] = false;
        break;
    default: break;
    }
}

void render_glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    g_window.inputs.buttons[button] = action != GLFW_RELEASE;
}

void render_glfw_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_window.inputs.scroll = vec2f((float) xoffset, (float) yoffset);
}

void window_open(const char* title, int w, int h)
{
    static bool gl_init = false;
    if (!gl_init)
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        gl_init = true;
    }

    g_window.window = glfwCreateWindow(w, h, title, NULL, NULL);
    g_window.width = w;
    g_window.height = h;
    if (!g_window.window)
    {
        debug_assertf(false, "Failed to create GLFW window");
        exit(1);
        return;
    }

    glfwMakeContextCurrent(g_window.window);
    glfwSwapInterval(1);

    glewExperimental = TRUE;
    if (glewInit() != GLEW_OK) return;

    glfwSetKeyCallback(g_window.window, render_glfw_key_callback);
    glfwSetMouseButtonCallback(g_window.window, render_glfw_mouse_button_callback);
    glfwSetScrollCallback(g_window.window, render_glfw_mouse_scroll_callback);

    glGenFramebuffers(1, &g_window.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_window.framebuffer);

    glGenTextures(1, &g_window.framebuffer_color);
    glBindTexture(GL_TEXTURE_2D, g_window.framebuffer_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_window.framebuffer_color, 0);

    glGenRenderbuffers(1, &g_window.framebuffer_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, g_window.framebuffer_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_window.framebuffer_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    debug_assertf(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glfwShowWindow(g_window.window);

    g_window.shader = shader_load(GLSL(
        layout(location = 0) in vec4 position;
        layout(location = 1) in vec4 normal;
        layout(location = 2) in vec4 color;
        layout(location = 3) in vec2 texture;

        out vec4 frag_position;
        out vec4 frag_normal;
        out vec4 frag_color;
        out vec2 frag_texture;

        uniform mat4 view_transform;
        uniform mat4 model_transform;

        void main() {
            frag_position = model_transform * position;
            frag_normal = model_transform * normal;
            frag_color = color;
            frag_texture = texture;

            gl_Position = view_transform * frag_position;
        }
    ), GLSL(
        in vec4 frag_position;
        in vec4 frag_normal;
        in vec4 frag_color;
        in vec2 frag_texture;

        out vec4 color;

        uniform sampler2D texture_map;

        void main() {
            color = texture(texture_map, frag_texture) * frag_color;
        }
    ));
    g_window.font_texture = texture_load("rl_text16.png", vec2i(16, 16));

    g_window.vb = new VertexBuffer;
    vb_init(g_window.vb);
}

bool frame_start()
{
    memcpy(g_window.inputs.last_keys, g_window.inputs.keys, sizeof(g_window.inputs.keys));
    memset(g_window.inputs.repeat, 0, sizeof(g_window.inputs.repeat));
    memcpy(g_window.inputs.last_buttons, g_window.inputs.buttons, sizeof(g_window.inputs.buttons));
    g_window.inputs.scroll = vec2f(0.0f, 0.0f);

    glfwPollEvents();

    vec2d pos;
    glfwGetCursorPos(g_window.window, &pos.x, &pos.y);

    g_window.inputs.mouse_delta = pos.cast<float>() - g_window.inputs.mouse_pos;
    g_window.inputs.mouse_pos = pos.cast<float>();

    if (glfwWindowShouldClose(g_window.window))
        return false;

    glBindFramebuffer(GL_FRAMEBUFFER, g_window.framebuffer);
    glViewport(0, 0, g_window.width, g_window.height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    return true;
}

void frame_end()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_window.width, g_window.height);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_window.framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, g_window.width, g_window.height, 0, 0, g_window.width, g_window.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glfwSwapBuffers(g_window.window);
    g_window.frame_count++;
}

bool input_key_pressed(int key)
{
    return g_window.inputs.keys[key]
        && (!g_window.inputs.last_keys[key]
            || g_window.inputs.repeat[key]
            && (g_window.inputs.keys[GLFW_KEY_LEFT_CONTROL]
                || g_window.inputs.keys[GLFW_KEY_RIGHT_CONTROL]));
}
bool input_key_down(int key) { return g_window.inputs.keys[key]; }
bool input_key_released(int key) { return !g_window.inputs.keys[key] && g_window.inputs.last_keys[key]; }

bool input_mouse_button_pressed(int button) { return g_window.inputs.buttons[button] && !g_window.inputs.last_buttons[button]; }
bool input_mouse_button_down(int button) { return g_window.inputs.buttons[button]; }
bool input_mouse_button_released(int button) { return !g_window.inputs.buttons[button] && g_window.inputs.last_buttons[button]; }

InputTextResult input_handle_text(sstring& text, int& cursor)
{
    debug_assert(cursor >= 0 && cursor <= text.size());
    if (input_key_pressed(GLFW_KEY_ESCAPE))
    {
        return InputTextResult::Canceled;
    }
    bool shift = g_window.inputs.keys[GLFW_KEY_LEFT_SHIFT] || g_window.inputs.keys[GLFW_KEY_RIGHT_SHIFT];
    bool ctrl = g_window.inputs.keys[GLFW_KEY_LEFT_CONTROL] || g_window.inputs.keys[GLFW_KEY_RIGHT_CONTROL];
    bool alt = g_window.inputs.keys[GLFW_KEY_LEFT_ALT] || g_window.inputs.keys[GLFW_KEY_RIGHT_ALT];

    for (int k = GLFW_KEY_A; k <= GLFW_KEY_Z; ++k)
    {
        if (input_key_pressed(k))
        {
            char c = shift ? k : k + 'a' - 'A';
            if (cursor < text.size()) text.insert(cursor, c);
            else                      text.append(c);
            cursor++;
        }
    }
    for (int k = 0; k <= 9; ++k)
    {
        if (input_key_pressed(GLFW_KEY_0 + k))
        {
            char c = shift ? ")!@#$%^&*("[k] : k + '0';
            if (cursor < text.size()) text.insert(cursor, c);
            else                      text.append(c);
            cursor++;
        }
        if (input_key_pressed(GLFW_KEY_KP_0 + k))
        {
            char c = k + '0';
            if (cursor < text.size()) text.insert(cursor, c);
            else                      text.append(c);
            cursor++;
        }
    }
    int symbols[]{ GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_COMMA, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_KP_ADD, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_MULTIPLY, GLFW_KEY_KP_DIVIDE, GLFW_KEY_KP_DECIMAL };
    const char* symbols_text = "-=[]\\;',./`+-*/.";
    const char* alt_symbols_text = "_+{}|:\"<>?~+-*/.";
    for (int i = 0; i < sizeof(symbols) / sizeof(symbols[0]); ++i)
    {
        if (input_key_pressed(symbols[i]))
        {
            char c = shift ? alt_symbols_text[i] : symbols_text[i];
            if (cursor < text.size()) text.insert(cursor, c);
            else                      text.append(c);
            cursor++;
        }
    }

    if (input_key_pressed(GLFW_KEY_BACKSPACE))
    {
        // @Todo: Ctrl + Backspace should delete the previous word
        if (cursor == text.size())
        {
            text.pop(1);
            cursor--;
        }
        else
        {
            char* data = text.mut_str();
            memmove(data + cursor - 1, data + cursor, text.size() - (size_t) cursor);
            text.pop(1);
            cursor--;
        }
    }
    if (input_key_pressed(GLFW_KEY_DELETE))
    {
        // @Todo: Ctrl + Delete should delete the next word
        if (cursor < text.size())
        {
            char* data = text.mut_str();
            memmove(data + cursor, data + cursor + 1, text.size() - (size_t) cursor);
            text.pop(1);
        }
    }

    if (input_key_pressed(GLFW_KEY_ENTER))
    {
        return InputTextResult::Finished;
    }
    return InputTextResult::Continue;
}

struct CharTexture
{
    char c;
    rect2f uv;
};

rect2f makeCharTexture(int x, int y)
{
    float x0 = x * 8.0f + 0.01f;
    float y0 = y * 16.0f + 0.01f;
    float u0 = x0 / 256;
    float v0 = y0 / 256;
    float u1 = (x0 + 7.98f) / 256;
    float v1 = (y0 + 15.98f) / 256;
    return rect2f(vec2f(u0, v0), vec2f(u1, v1));
}

rect2f getCharTexture(char c)
{
    static rect2f char_map[256]{ rect2f() };
    static bool char_map_initialized = false;
    if (!char_map_initialized)
    {
        char_map[' '] = makeCharTexture(31, 0);
        for (int i = 0; i < 26; ++i)
        {
            char_map['A' + i] = makeCharTexture(i, 0);
            char_map['a' + i] = makeCharTexture(i, 1);
        }
        for (int i = 0; i < 5; ++i)
        {
            char_map['0' + i] = makeCharTexture(i + 26, 0);
            char_map['0' + i + 5] = makeCharTexture(i + 26, 1);
        }
        for (int i = 0; i < 32; ++i)
        {
            char c = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"[i];
            char_map[c] = makeCharTexture(i, 2);
        }
        // SpecialChars
        for (int i = 1; i < MaxSpecialChars; ++i)
        {
            char_map[i] = makeCharTexture(i - 1, 3);
        }
        char_map_initialized = true;
    }
    return char_map[c];
}

rect2f getTexture(int id)
{
    vec2i size = g_window.font_texture->size;
    vec2i grid = g_window.font_texture->grid_size;

    int tw = size.x / grid.x;
    int th = size.y / grid.y;

    int tx = id % tw;
    int ty = id / tw;
    float x0 = tx * grid.x + 0.5f;
    float y0 = ty * grid.y + 0.5f;
    float u0 = x0 / size.x;
    float v0 = y0 / size.y;
    float u1 = (x0 + grid.x - 1) / size.x;
    float v1 = (y0 + grid.y - 1) / size.y;
    return rect2f(vec2f(u0, v0), vec2f(u1, v1));
}

void vb_push_vertex(VertexBuffer* vb, vec4f p, vec4f n, vec4f c, vec2f t)
{
    vb->vertices.push_back(p.x); vb->vertices.push_back(p.y); vb->vertices.push_back(p.z); vb->vertices.push_back(p.w);
    vb->vertices.push_back(n.x); vb->vertices.push_back(n.y); vb->vertices.push_back(n.z); vb->vertices.push_back(n.w);
    vb->vertices.push_back(c.x); vb->vertices.push_back(c.y); vb->vertices.push_back(c.z); vb->vertices.push_back(c.w);
    vb->vertices.push_back(t.x); vb->vertices.push_back(t.y);
}

void vb_push_draw(VertexBuffer* vb, u32 type, std::initializer_list<int> idx)
{
    int vertices = 0;
    for (int i : idx)
        vertices = scalar::max(vertices, i + 1);

    if (vertices + vb->next_index > UINT16_MAX)
    {
        vb->index_offset += vb->next_index;
        vb->next_index = 0;
        vb->commands.emplace_back(type, (u32)idx.size(), (u32)vb->indices.size(), vb->index_offset);
    }
    else if (vb->commands.empty()
        || vb->commands.back().mode != type)
    {
        vb->commands.emplace_back(type, (u32)idx.size(), (u32)vb->indices.size(), vb->index_offset);
    }
    else
    {
        vb->commands.back().count += (u32)idx.size();
    }

    for (int i : idx)
        vb->indices.push_back(i + vb->next_index);
    vb->next_index += vertices;
}

void vb_push_quad(VertexBuffer* vb, vec2f p0, vec2f p1, vec4f color, vec2f t0, vec2f t1)
{
    vec4f n(0, 0, 1, 0);
    vb_push_vertex(vb, vec4f(p0.x, p0.y, 0, 1), n, color, vec2f(t0.x, t0.y));
    vb_push_vertex(vb, vec4f(p1.x, p0.y, 0, 1), n, color, vec2f(t1.x, t0.y));
    vb_push_vertex(vb, vec4f(p1.x, p1.y, 0, 1), n, color, vec2f(t1.x, t1.y));
    vb_push_vertex(vb, vec4f(p0.x, p1.y, 0, 1), n, color, vec2f(t0.x, t1.y));

    vb_push_draw(vb, GL_TRIANGLES, { 0, 1, 2, 2, 3, 0 });
}

void vb_init(VertexBuffer* vb)
{
    // Create vao, vbo, vboi
    glGenVertexArrays(1, &vb->vao);
    glBindVertexArray(vb->vao);
    glGenBuffers(1, &vb->vbo);
    glGenBuffers(1, &vb->vboi);
    glBindBuffer(GL_ARRAY_BUFFER, vb->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vb->vboi);

    // Set vertex attributes
    int vertex_size = 4 + 4 + 4 + 2;
    int vertex_stride = vertex_size * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, vertex_stride, (void*)0); // position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(4 * sizeof(float))); // normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(8 * sizeof(float))); // color
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, vertex_stride, (void*)(12 * sizeof(float))); // texture

    vb->last_shader = UINT32_MAX;
}

void vb_clear(VertexBuffer* vb)
{
    vb->vertices.clear();
    vb->indices.clear();
    vb->commands.clear();

    vb->index_offset = 0;
    vb->next_index = 0;
    vb->last_shader = UINT32_MAX;
}

void vb_upload(VertexBuffer* vb)
{
    int vertex_size = 4 + 4 + 4 + 2;
    int count = (int)vb->vertices.size() / vertex_size;

    glBindVertexArray(vb->vao);
    glBindBuffer(GL_ARRAY_BUFFER, vb->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vb->vboi);

    glBufferData(GL_ARRAY_BUFFER, vb->vertices.size() * sizeof(float), vb->vertices.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vb->indices.size() * sizeof(u16), vb->indices.data(), GL_STATIC_DRAW);
}

void vb_draw(VertexBuffer* vb)
{
    u32 last_shader = 0;

    glBindVertexArray(vb->vao);
    glBindBuffer(GL_ARRAY_BUFFER, vb->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vb->vboi);

    for (u32 i = 0; i < vb->commands.size(); i++)
    {
        const VertexBuffer::DrawCommandData& cmd = vb->commands[i];
        glDrawElementsBaseVertex(cmd.mode, cmd.count, GL_UNSIGNED_SHORT, (void*)(cmd.offset * 2llu), cmd.base);
    }
}

void render_buffer(TextBuffer* term, float zoom)
{
    vb_clear(g_window.vb);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    float tw = (float) term->w;
    float th = (float) term->h;
    float zw = tw / zoom;
    float zh = th / zoom;
    float pct = 25.0f / 80.0f;
    float hwd = (tw - zw) / 2;
    float hhd = (th - zh) / 2;
    mat4f camera_transform = ortho(hwd, tw - hwd, hhd, th - hhd);
    mat4f i;

    glUseProgram(g_window.shader);
    glUniformMatrix4fv(glGetUniformLocation(g_window.shader, "view_transform"), 1, GL_FALSE, camera_transform.m);
    glUniformMatrix4fv(glGetUniformLocation(g_window.shader, "model_transform"), 1, GL_FALSE, i.m);
    glUniform1i(glGetUniformLocation(g_window.shader, "texture_map"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_window.font_texture->handle);

    rect2f empty = getTexture(TileEmpty - 64);
    rect2f blank = getTexture(TileFull - 64);

    for (int y = 0; y < term->h; ++y)
    {
        for (int x = 0; x < term->w; ++x)
        {
            TextBuffer::Char& ch = term->buffer[x + y * term->w];
            if ((ch.bg & 0xFFFFFF) != 0)
            {
                vb_push_quad(g_window.vb, vec2f((float)x, (float)y), vec2f(x + 1.0f, y + 1.0f), makeColor(ch.bg), blank.lower, blank.upper);
            }
            if (ch.text[0] > TileEmpty)
            {
                rect2f texture = getTexture(ch.text[0] - 64);
                vec2f t0(texture.lower.x, texture.upper.y);
                vec2f t1(texture.upper.x, texture.lower.y);
                vb_push_quad(g_window.vb, vec2f((float)x, (float)y), vec2f(x + 1.0f, y + 1.0f), makeColor(ch.color[0]), t0, t1);
            }
            else if (ch.text[1] == 0xFFFF)
            {
                rect2f texture = getCharTexture(ch.text[0]);
                vec2f t0(texture.lower.x, texture.upper.y);
                vec2f t1(texture.upper.x, texture.lower.y);
                float x0 = (float)x + 0.25f;
                vb_push_quad(g_window.vb, vec2f(x0, (float)y), vec2f(x0 + 0.5f, y + 1.0f), makeColor(ch.color[0]), t0, t1);
            }
            else
            {
                for (int j = 0; j < 2; ++j)
                {
                    if (ch.text[j] != TileEmpty)
                    {
                        rect2f texture = getCharTexture(ch.text[j]);
                        vec2f t0(texture.lower.x, texture.upper.y);
                        vec2f t1(texture.upper.x, texture.lower.y);
                        float x0 = j == 0 ? (float)x : (float)x + 0.5f;
                        vb_push_quad(g_window.vb, vec2f(x0, (float)y), vec2f(x0 + 0.5f, y + 1.0f), makeColor(ch.color[j]), t0, t1);
                    }
                }
            }
            if ((ch.overlay & 0xFFFFFF) != 0)
            {
                vb_push_quad(g_window.vb, vec2f((float)x, (float)y), vec2f(x + 1.0f, y + 1.0f), makeColor(ch.overlay), blank.lower, blank.upper);
            }
        }
    }

    vb_upload(g_window.vb);
    vb_draw(g_window.vb);
}

Texture* get_fallback_texture()
{
    static Texture* fallback_texture = nullptr;
    if (!fallback_texture)
    {
        u32 tx;
        glGenTextures(1, &tx);
        Texture* tex = new Texture;
        tex->handle = tx;
        tex->size = tex->grid_size = vec2i(16, 16);

        u32 px[16 * 16]{ 0 };
        for (int x = 0; x < 16; x++)
            for (int y = 0; y < 16; y++)
                px[x + y * 16] = ((x + y) % 2) ? 0xFFFF00FF : 0xFF000000;

        glBindTexture(GL_TEXTURE_2D, tx);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
    return fallback_texture;
}

Texture* texture_load(const char* name, vec2i grid_size)
{
    int x, y, c;
    u8* img_data = stbi_load(name, &x, &y, &c, 4);
    if (!img_data)
    {
        return get_fallback_texture();
    }

    u32 tx;
    glGenTextures(1, &tx);
    if (grid_size.zero()) grid_size = vec2i(x, y);
    Texture* tex = new Texture;
    tex->handle = tx;
    tex->size = vec2i(x, y);
    tex->grid_size = grid_size;

    glBindTexture(GL_TEXTURE_2D, tx);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
    stbi_image_free(img_data);
    return tex;
}

u32 shader_load(const char* vs, const char* fs)
{
    u32 pid = glCreateProgram();

    u32 vs_id = glCreateShader(GL_VERTEX_SHADER);
    u32 fs_id = glCreateShader(GL_FRAGMENT_SHADER);

    int Result = GL_FALSE;
    int info_log_length;

    //render_print("Creating shader program %u\n", pid);

    glShaderSource(vs_id, 1, &vs, NULL);
    glCompileShader(vs_id);

    glGetShaderiv(vs_id, GL_COMPILE_STATUS, &Result);
    if (Result == GL_FALSE) {
        glGetShaderiv(vs_id, GL_INFO_LOG_LENGTH, &info_log_length);
        char* error_msg = new char[info_log_length + 1];
        glGetShaderInfoLog(vs_id, info_log_length, NULL, error_msg);
        fprintf(stderr, "Error compiling vertex shader: %s", error_msg);
        abort();
    }

    glShaderSource(fs_id, 1, &fs, NULL);
    glCompileShader(fs_id);

    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &Result);
    if (Result == GL_FALSE) {
        glGetShaderiv(fs_id, GL_INFO_LOG_LENGTH, &info_log_length);
        char* error_msg = new char[info_log_length + 1];
        glGetShaderInfoLog(fs_id, info_log_length, NULL, error_msg);
        fprintf(stderr, "Error compiling fragment shader: %s", error_msg);
        abort();
    }

    glAttachShader(pid, vs_id);
    glAttachShader(pid, fs_id);
    glLinkProgram(pid);

    glGetProgramiv(pid, GL_LINK_STATUS, &Result);
    if (Result == GL_FALSE) {
        glGetProgramiv(pid, GL_INFO_LOG_LENGTH, &info_log_length);
        char* error_msg = new char[info_log_length + 1];
        glGetProgramInfoLog(pid, info_log_length, NULL, error_msg);
        fprintf(stderr, "Error linking shader: %s", error_msg);
        abort();
    }
    glDetachShader(pid, vs_id);
    glDetachShader(pid, fs_id);
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);

    return pid;
}
