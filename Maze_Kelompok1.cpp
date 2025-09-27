#include <glut.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846f 
#endif

// dimensi windows
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 400;
const int MAP_WIDTH = WINDOW_WIDTH / 2;
const int MAP_HEIGHT = WINDOW_HEIGHT;

// properti player
struct Player {
    float x, y;
    float dir;
    float radius;

    Player(float px, float py) : x(px), y(py), dir(0.0f), radius(8.0f) {}
};

// struct untuk objek penanda (Sprite)
struct GameMarker {
    float x, y;
    float size;
    // Menggunakan type untuk membedakan visual 3D
    int type; // 0=START (Bendera), 1=FINISH (Kunci)
    float r, g, b; // Warna 2D
    std::string name;
};

// struct untuk dinding
struct Wall {
    float x1, y1, x2, y2;

    Wall(float px1, float py1, float px2, float py2)
        : x1(px1), y1(py1), x2(px2), y2(py2) {
    }
};

// struct untuk sinar pancaran dari player
struct Ray {
    float x, y;
    float dx, dy;
    float angle;
    float distance;
    bool hit;
    float hitX, hitY;

    Ray(float angle_offset)
        : x(0.0f), y(0.0f), dx(0.0f), dy(0.0f), hitX(0.0f), hitY(0.0f),
        angle(angle_offset),
        distance(500.0f),
        hit(false)
    {}
};

// variabel global
Player player(60.0f, 60.0f);
std::vector<Wall> maze;
std::vector<Ray> rays;
std::vector<GameMarker> markers;
const float RAY_LENGTH = 500.0f;
const int NUM_RAYS = 600;
const float FOV = 60.0f;

bool keys[256] = { false };


// --- PROTOTIPE FUNGSI ---
void init();
void initMaze();
void initRays();
void initMarkers();
void drawMarker2D(const GameMarker& marker);
void setOrthoViewport(int x, int y, int width, int height, float left, float right, float bottom, float top);
void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b);
void renderMarkers3D(float slice_width);
void display();
void update(int value);
void keyDown(unsigned char key, int x, int y);
void keyUp(unsigned char key, int x, int y);

void initMarkers() {
    // Penanda START (type 0: Bendera, 2D warna Hijau)
    markers.push_back({ player.x, player.y, 10.0f, 0, 0.2f, 0.8f, 0.2f, "START" });

    // Penanda FINISH (type 1: Kunci, 2D warna Merah)
    markers.push_back({ 560.0f, 40.0f, 10.0f, 1, 0.8f, 0.2f, 0.2f, "FINISH" });
}

void drawMarker2D(const GameMarker& marker) {
    float size = marker.size;

    if (marker.type == 0) {
        // START: Tiang + kain bendera kotak-kotak
        // Tiang
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(marker.x - size * 0.10f, marker.y - size);
        glVertex2f(marker.x + size * 0.10f, marker.y - size);
        glVertex2f(marker.x + size * 0.10f, marker.y + size);
        glVertex2f(marker.x - size * 0.10f, marker.y + size);
        glEnd();

        // Kain bendera (pola checker)
        int cols = 4, rows = 3;
        float flag_w = size * 1.6f;
        float flag_h = size * 1.2f;
        float left = marker.x + size * 0.10f;
        float top = marker.y - flag_h * 0.5f;

        for (int i = 0; i < cols; ++i) {
            for (int j = 0; j < rows; ++j) {
                bool black = ((i + j) % 2 == 0);
                float r = black ? 0.1f : 0.95f;
                float g = black ? 0.1f : 0.95f;
                float b = black ? 0.1f : 0.95f;
                glColor3f(r, g, b);

                float x1 = left + i * (flag_w / cols);
                float y1 = top + j * (flag_h / rows);
                float x2 = x1 + (flag_w / cols);
                float y2 = y1 + (flag_h / rows);

                glBegin(GL_QUADS);
                glVertex2f(x1, y1);
                glVertex2f(x2, y1);
                glVertex2f(x2, y2);
                glVertex2f(x1, y2);
                glEnd();
            }
        }
    }
    else {
        // FINISH: Kunci (cincin + batang + gigi)
        glColor3f(0.95f, 0.75f, 0.10f); // emas

        float cx = marker.x;
        float cy = marker.y;

        // Cincin (donat sederhana dari pita segi-banyak)
        float outer = size * 0.90f;
        float inner = size * 0.55f;
        int seg = 24;
        for (int k = 0; k < seg; ++k) {
            float a0 = 2.0f * M_PI * k / seg;
            float a1 = 2.0f * M_PI * (k + 1) / seg;

            float ox0 = cx + std::cos(a0) * outer, oy0 = cy + std::sin(a0) * outer;
            float ox1 = cx + std::cos(a1) * outer, oy1 = cy + std::sin(a1) * outer;
            float ix0 = cx + std::cos(a0) * inner, iy0 = cy + std::sin(a0) * inner;
            float ix1 = cx + std::cos(a1) * inner, iy1 = cy + std::sin(a1) * inner;

            glBegin(GL_QUADS);
            glVertex2f(ix0, iy0);
            glVertex2f(ix1, iy1);
            glVertex2f(ox1, oy1);
            glVertex2f(ox0, oy0);
            glEnd();
        }

        // Batang kunci
        float stemW = size * 0.30f;
        float stemL = size * 1.60f;
        float stemLeft = cx + outer;
        float stemTop = cy - stemW * 0.5f;
        glBegin(GL_QUADS);
        glVertex2f(stemLeft, stemTop);
        glVertex2f(stemLeft + stemL, stemTop);
        glVertex2f(stemLeft + stemL, stemTop + stemW);
        glVertex2f(stemLeft, stemTop + stemW);
        glEnd();

        // Gigi kunci
        float toothW = stemW * 0.60f;
        float toothH = stemW * 0.90f;
        for (int t = 0; t < 2; ++t) {
            float tx = stemLeft + stemL * (0.40f + 0.18f * t);
            float ty = stemTop + stemW;
            glBegin(GL_QUADS);
            glVertex2f(tx, ty);
            glVertex2f(tx + toothW, ty);
            glVertex2f(tx + toothW, ty + toothH);
            glVertex2f(tx, ty + toothH);
            glEnd();
        }
    }
}

void setOrthoViewport(int x, int y, int width, int height,
    float left, float right, float bottom, float top) {
    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawQuad(float x1, float y1, float x2, float y2,
    float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

// Menggambar objek penanda 3D sesuai tipe
void drawMarker3D(float left_x, float top_y, float right_x, float bottom_y, float brightness, int type) {
    float width = right_x - left_x;
    float height = bottom_y - top_y;

    if (type == 0) { // BENDERA START (Pola Kotak-kotak)
        int num_blocks = 4;
        float block_width = width / num_blocks;
        float block_height = height / num_blocks;

        for (int i = 0; i < num_blocks; ++i) {
            for (int j = 0; j < num_blocks; ++j) {
                float r, g, b;
                if ((i + j) % 2 == 0) { // Kotak Putih
                    r = 0.9f; g = 0.9f; b = 0.9f;
                }
                else { // Kotak Hitam/Abu-abu
                    r = 0.1f; g = 0.1f; b = 0.1f;
                }

                float x1 = left_x + i * block_width;
                float y1 = top_y + j * block_height;
                float x2 = x1 + block_width;
                float y2 = y1 + block_height;

                drawQuad(x1, y1, x2, y2, r * brightness, g * brightness, b * brightness);
            }
        }
    }
    else if (type == 1) { // KUNCI FINISH (Kotak Emas + Batang Vertikal)
        float r_gold = 0.9f;
        float g_gold = 0.7f;
        float b_gold = 0.0f;

        // Kotak Kunci (Bagian Kepala)
        float head_width = width;
        float head_height = height / 3.0f;
        drawQuad(left_x, top_y, right_x, top_y + head_height,
            r_gold * brightness, g_gold * brightness, b_gold * brightness);

        // Batang Kunci
        float stem_width = width / 3.0f;
        float stem_left = left_x + width / 2.0f - stem_width / 2.0f;
        drawQuad(stem_left, top_y + head_height, stem_left + stem_width, bottom_y,
            r_gold * brightness, g_gold * brightness, b_gold * brightness);
    }
}

void renderMarkers3D(float slice_width) {
    float player_angle_rad = player.dir * M_PI / 180.0f;
    float cos_dir = std::cos(player_angle_rad);
    float sin_dir = std::sin(player_angle_rad);

    for (const auto& marker : markers) {
        float dx = marker.x - player.x;
        float dy = marker.y - player.y;

        // 1. Transformasi ke Koordinat Pemain (Kamera)
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > RAY_LENGTH) continue;

        float sprite_x = dx * cos_dir + dy * sin_dir;
        float sprite_y = dy * cos_dir - dx * sin_dir;

        if (sprite_y <= 0) continue;

        // 2. Proyeksi ke Layar
        float inverse_distance = 1.0f / sprite_y;
        float half_fov_tan = std::tan(FOV / 2.0f * M_PI / 180.0f);

        float screen_x_center = MAP_WIDTH / 2.0f + (sprite_x / half_fov_tan * inverse_distance) * (MAP_WIDTH / 2.0f);

        // PERBAIKAN STABILITAS: Skala vertikal harus dihitung dengan perbandingan rasio
        float marker_height_scale = 1.0f; // Misal 1.0 = tinggi dinding
        float marker_screen_height = (marker_height_scale / sprite_y) * MAP_HEIGHT;

        // Marker diasumsikan berbentuk kubus/silinder di 3D, jadi lebar sama dengan tingginya.
        float marker_screen_width = marker_screen_height * (marker.size / 100.0f); // Skala lebar relatif

        float top_y = MAP_HEIGHT / 2.0f - marker_screen_height / 2.0f;
        float bottom_y = MAP_HEIGHT / 2.0f + marker_screen_height / 2.0f;
        float left_x = screen_x_center - marker_screen_width / 2.0f;
        float right_x = screen_x_center + marker_screen_width / 2.0f;

        float brightness = 1.0f - (distance / RAY_LENGTH);
        if (brightness > 1.0f) brightness = 1.0f;
        if (brightness < 0.2f) brightness = 0.2f;

        // ********** LOGIC CEK KEDALAMAN (Z-DEPTH) **********
        int start_slice = static_cast<int>(left_x / slice_width);
        int end_slice = static_cast<int>(right_x / slice_width);

        start_slice = std::max(0, start_slice);
        end_slice = std::min(NUM_RAYS - 1, end_slice);

        bool should_draw = false;
        for (int i = start_slice; i <= end_slice; ++i) {
            if (distance < rays[i].distance) {
                should_draw = true;
                break;
            }
        }

        if (should_draw) {
            // Panggil fungsi gambar 3D yang baru
            drawMarker3D(left_x, top_y, right_x, bottom_y, brightness, marker.type);
        }
        // *********************************************************
    }
}

// --- FUNGSI MAZE & RAYCASTING ---

void initMaze() {
    // titik- titik dinding
    std::vector<std::vector<float>> maze_data = {
        {0, 0, 40, 0}, {0, 40, 40, 40}, {0, 0, 0, 40}, {40, 0, 80, 0},
        {40, 40, 80, 40}, {80, 0, 120, 0}, {120, 0, 120, 40}, {120, 0, 160, 0},
        {120, 40, 160, 40}, {160, 0, 200, 0}, {200, 0, 240, 0}, {200, 40, 240, 40},
        {240, 0, 280, 0}, {280, 0, 320, 0}, {320, 0, 320, 40}, {320, 0, 360, 0},
        {360, 0, 400, 0}, {360, 40, 400, 40}, {400, 0, 440, 0}, {400, 40, 440, 40},
        {440, 0, 480, 0}, {440, 40, 480, 40}, {480, 0, 520, 0}, {520, 0, 560, 0},
        {560, 0, 560, 40}, {560, 0, 600, 0}, {600, 0, 600, 40}, {0, 40, 0, 80},
        {40, 80, 80, 80}, {80, 40, 80, 80}, {80, 80, 120, 80}, {160, 40, 160, 80},
        {160, 80, 200, 80}, {240, 80, 240, 80}, {280, 80, 280, 80}, {320, 80, 320, 80},
        {320, 120, 360, 120}, {360, 80, 360, 80}, {400, 80, 400, 80}, {440, 120, 480, 120},
        {480, 120, 520, 120}, {520, 120, 560, 120}, {560, 120, 600, 120}, {600, 80, 600, 80},
        {0, 160, 40, 160}, {0, 120, 0, 160}, {40, 160, 80, 160}, {80, 160, 120, 160},
        {160, 120, 160, 160}, {200, 120, 200, 160}, {240, 120, 240, 160}, {280, 120, 280, 160},
        {280, 160, 320, 160}, {320, 160, 360, 160}, {400, 120, 400, 160}, {400, 160, 440, 160},
        {440, 120, 440, 160}, {480, 160, 520, 160}, {520, 160, 560, 160}, {600, 120, 600, 160},
        {0, 200, 40, 200}, {0, 160, 0, 200}, {80, 200, 120, 200}, {120, 160, 120, 200},
        {160, 160, 160, 200}, {200, 200, 240, 200}, {240, 160, 240, 200}, {240, 200, 280, 200},
        {280, 200, 320, 200}, {360, 160, 360, 200}, {360, 200, 400, 200}, {400, 160, 400, 200},
        {440, 200, 480, 200}, {480, 160, 480, 200}, {520, 200, 560, 200}, {560, 160, 560, 200},
        {600, 160, 600, 200}, {0, 200, 0, 240}, {40, 240, 80, 240}, {80, 200, 80, 240},
        {120, 240, 160, 240}, {160, 200, 160, 240}, {160, 240, 200, 240}, {200, 240, 240, 240},
        {240, 240, 280, 240}, {320, 200, 320, 240}, {360, 200, 360, 240}, {400, 240, 440, 240},
        {440, 200, 440, 240}, {440, 240, 480, 240}, {520, 200, 520, 240}, {600, 200, 600, 240},
        {0, 240, 0, 280}, {40, 240, 40, 280}, {80, 280, 120, 280}, {120, 240, 120, 280},
        {160, 280, 200, 280}, {200, 280, 240, 280}, {280, 240, 280, 280}, {320, 240, 320, 280},
        {360, 240, 360, 280}, {400, 240, 400, 280}, {440, 280, 480, 280}, {480, 240, 520, 280},
        {520, 240, 520, 280}, {560, 240, 560, 280}, {600, 240, 600, 280}, {0, 280, 0, 320},
        {40, 280, 40, 320}, {40, 320, 80, 320}, {80, 320, 120, 320}, {120, 320, 160, 320},
        {160, 280, 160, 320}, {200, 280, 200, 320}, {240, 320, 280, 320}, {280, 280, 280, 320},
        {320, 280, 320, 320}, {320, 320, 360, 320}, {360, 320, 400, 320}, {400, 320, 440, 320},
        {440, 280, 440, 320}, {480, 320, 520, 320}, {520, 320, 560, 320}, {560, 280, 560, 320},
        {600, 280, 600, 320}, {0, 320, 0, 360}, {40, 320, 40, 360}, {80, 360, 120, 360},
        {120, 360, 160, 360}, {160, 360, 200, 360}, {200, 360, 240, 360}, {240, 320, 240, 360},
        {240, 360, 280, 360}, {280, 360, 320, 360}, {320, 360, 360, 360}, {360, 320, 360, 360},
        {440, 320, 440, 360}, {440, 360, 480, 360}, {480, 360, 520, 360}, {560, 320, 560, 360},
        {600, 320, 600, 360}, {0, 400, 40, 400}, {0, 360, 0, 400}, {40, 400, 80, 400},
        {80, 400, 120, 400}, {120, 400, 160, 400}, {160, 400, 200, 400}, {200, 400, 240, 400},
        {240, 400, 280, 400}, {280, 400, 320, 400}, {320, 400, 360, 400}, {360, 400, 400, 400},
        {400, 360, 400, 400}, {400, 400, 440, 400}, {440, 400, 480, 400}, {480, 400, 520, 400},
        {520, 400, 560, 400}, {560, 360, 560, 400}, {560, 400, 600, 400}, {600, 360, 600, 400}
    };

    for (const auto& wall_data : maze_data) {
        maze.emplace_back(wall_data[0], wall_data[1], wall_data[2], wall_data[3]);
    }
}

// inisialisasi sinar pancaran dari player
void initRays() {
    rays.clear();
    float angle_step = FOV / NUM_RAYS;
    float start_angle = -FOV / 2.0f;

    for (int i = 0; i < NUM_RAYS; i++) {
        rays.emplace_back(start_angle + i * angle_step);
    }
}

// fungsi untuk menghitung intersection antara dua garis, yang digunakan untuk ray casting
bool lineIntersection(float x1, float y1, float x2, float y2,
    float x3, float y3, float x4, float y4,
    float& ix, float& iy) {
    float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(denom) < 1e-10) return false;

    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        ix = x1 + t * (x2 - x1);
        iy = y1 + t * (y2 - y1);
        return true;
    }
    return false;
}

// jarak titik ke garis (untuk deteksi tabrakan)
float pointToLineDistance(float px, float py, float x1, float y1, float x2, float y2) {
    float line_length = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    if (line_length == 0) {
        return std::sqrt((px - x1) * (px - x1) + (py - y1) * (py - y1));
    }

    float t = ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / (line_length * line_length);
    t = std::max(0.0f, std::min(1.0f, t));

    float closest_x = x1 + t * (x2 - x1);
    float closest_y = y1 + t * (y2 - y1);

    return std::sqrt((px - closest_x) * (px - closest_x) + (py - closest_y) * (py - closest_y));
}

// mengecek tabrakan player dengan dinding
bool checkCollision(float x, float y) {
    for (const auto& wall : maze) {
        if (pointToLineDistance(x, y, wall.x1, wall.y1, wall.x2, wall.y2) < player.radius) {
            return true;
        }
    }
    return false;
}

// fungsi untuk memancarkan sinar dari player dan mendeteksi tabrakan dengan dinding
void castRays() {
    for (auto& ray : rays) {
        float ray_angle = (player.dir + ray.angle) * M_PI / 180.0f;
        ray.x = player.x;
        ray.y = player.y;
        ray.dx = std::cos(ray_angle);
        ray.dy = std::sin(ray_angle);

        ray.distance = RAY_LENGTH;
        ray.hit = false;
        ray.hitX = ray.x + ray.dx * RAY_LENGTH;
        ray.hitY = ray.y + ray.dy * RAY_LENGTH;

        for (const auto& wall : maze) {
            float ix, iy;
            if (lineIntersection(ray.x, ray.y, ray.x + ray.dx * RAY_LENGTH, ray.y + ray.dy * RAY_LENGTH,
                wall.x1, wall.y1, wall.x2, wall.y2, ix, iy)) {
                float dist = std::sqrt((ix - ray.x) * (ix - ray.x) + (iy - ray.y) * (iy - ray.y));
                if (dist < ray.distance) {
                    ray.distance = dist;
                    ray.hitX = ix;
                    ray.hitY = iy;
                    ray.hit = true;
                }
            }
        }
    }
}

void render2D() {
    setOrthoViewport(0, 0, MAP_WIDTH, MAP_HEIGHT, 0, 600, 400, 0);

    // menggambarkan maze dari titik-titik dinding
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINES);
    for (const auto& wall : maze) {
        glVertex2f(wall.x1, wall.y1);
        glVertex2f(wall.x2, wall.y2);
    }
    glEnd();

    // Gambar penanda Start/Finish di 2D
    for (const auto& marker : markers) {
        drawMarker2D(marker);
    }

    glPushMatrix();

    // menggambarkan sinar pancaran dari player
    glBegin(GL_LINES);
    for (const auto& ray : rays) {
        if (ray.hit) {
            glColor3f(0.9f, 0.9f, 0.9f);
        }
        else {
            glColor3f(0.3f, 0.3f, 0.3f);
        }
        glVertex2f(ray.x, ray.y);
        glVertex2f(ray.hitX, ray.hitY);
    }
    glEnd();

    // menggambar player
    glColor3f(0.7f, 0.2f, 0.5f);
    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex2f(player.x, player.y);
    glEnd();

    glPopMatrix();
}

void render3D() {
    setOrthoViewport(MAP_WIDTH, 0, MAP_WIDTH, MAP_HEIGHT, 0, MAP_WIDTH, MAP_HEIGHT, 0);

    // menggambar langit ruangan
    drawQuad(0, 0, MAP_WIDTH, MAP_HEIGHT / 2, 0.56f, 0.44f, 0.0f);

    // menggambar lantai ruangan
    drawQuad(0, MAP_HEIGHT / 2, MAP_WIDTH, MAP_HEIGHT, 0.86f, 0.75f, 0.14f);

    // Dinding
    float slice_width = (float)MAP_WIDTH / NUM_RAYS;
    for (int i = 0; i < NUM_RAYS; i++) {
        if (rays[i].hit) {
            float corrected_distance = rays[i].distance * std::cos(rays[i].angle * M_PI / 180.0f);

            float wall_height = (10.0f / corrected_distance) * MAP_HEIGHT;
            if (wall_height > MAP_HEIGHT) wall_height = MAP_HEIGHT;

            float brightness = 1.0f - (corrected_distance / RAY_LENGTH);
            if (brightness > 1.0f) brightness = 1.0f;
            if (brightness < 0.2f) brightness = 0.2f;

            float x = i * slice_width;
            float y = (MAP_HEIGHT - wall_height) / 2.0f;

            drawQuad(x, y, x + slice_width, y + wall_height, brightness, brightness, 0.016f);
        }
    }

    // Panggil fungsi render marker 3D
    renderMarkers3D(slice_width);
}

// fungsi untuk menampilkan semua yang telah dirender
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    render2D();
    render3D();

    glutSwapBuffers();
}

// fungsi update untuk mengatur logika permainan
void update(int value) {
    const float move_speed = 2.0f;
    const float turn_speed = 3.0f;

    // input handling
    if (keys['a'] || keys['A']) {
        player.dir -= turn_speed;
    }
    if (keys['d'] || keys['D']) {
        player.dir += turn_speed;
    }

    float new_x = player.x;
    float new_y = player.y;

    if (keys['w'] || keys['W']) {
        float angle = player.dir * M_PI / 180.0f;
        new_x = player.x + std::cos(angle) * move_speed;
        new_y = player.y + std::sin(angle) * move_speed;
    }
    if (keys['s'] || keys['S']) {
        float angle = player.dir * M_PI / 180.0f;
        new_x = player.x - std::cos(angle) * move_speed;
        new_y = player.y - std::sin(angle) * move_speed;
    }

    // mendeteksi tabrakan sebelum memperbarui posisi player
    if (!checkCollision(new_x, player.y)) {
        player.x = new_x;
    }
    if (!checkCollision(player.x, new_y)) {
        player.y = new_y;
    }

    // menjaga agar arah tetap dalam 0-360 derajat
    if (player.dir < 0) player.dir += 360;
    if (player.dir >= 360) player.dir -= 360;

    castRays();
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// fungsi untuk menangani input keyboard
void keyDown(unsigned char key, int x, int y) {
    keys[key] = true;
    if (key == 27) exit(0); // key 27 adalah ESC
}

void keyUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// inisialisasi OpenGL
void init() {
    glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Maze Kelompok 1 - Start & Finish");

    init();
    initMaze();
    initMarkers();
    initRays();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutTimerFunc(0, update, 0);

    std::cout << "Cara Main:" << std::endl;
    std::cout << "W/S - Maju/Mundur" << std::endl;
    std::cout << "A/D - Belok Kanan/Kiri" << std::endl;
    std::cout << "ESC - Keluar" << std::endl;

    glutMainLoop();
    return 0;
}
