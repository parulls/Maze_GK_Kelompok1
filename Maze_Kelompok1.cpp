#include <glut.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
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

    Ray(float angle_offset) : angle(angle_offset), distance(500.0f), hit(false) {}
};

// variabel global
Player player(60.0f, 60.0f);
std::vector<Wall> maze;
std::vector<Ray> rays;
const float RAY_LENGTH = 500.0f;
const int NUM_RAYS = 600;
const float FOV = 60.0f;

bool keys[256] = { false };

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
    {200, 40, 200, 80}, {240, 40, 240, 80}, {280, 40, 280, 80}, {320, 40, 320, 80},
    {360, 40, 360, 80}, {400, 80, 440, 80}, {440, 80, 480, 80}, {480, 80, 520, 80},
    {520, 40, 520, 80}, {560, 40, 560, 80}, {600, 40, 600, 80}, {0, 80, 0, 120},
    {40, 120, 80, 120}, {80, 120, 120, 120}, {120, 120, 160, 120}, {160, 80, 160, 120},
    {160, 120, 200, 120}, {240, 80, 240, 120}, {280, 80, 280, 120}, {320, 80, 320, 120},
    {320, 120, 360, 120}, {360, 80, 360, 120}, {400, 80, 400, 120}, {440, 120, 480, 120},
    {480, 120, 520, 120}, {520, 120, 560, 120}, {560, 120, 600, 120}, {600, 80, 600, 120},
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
    {360, 240, 360, 280}, {400, 240, 400, 280}, {440, 280, 480, 280}, {480, 280, 520, 280},
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

        // nilai default jika tidak ada tabrakan
        ray.distance = RAY_LENGTH;
        ray.hit = false;
        ray.hitX = ray.x + ray.dx * RAY_LENGTH;
        ray.hitY = ray.y + ray.dy * RAY_LENGTH;

        // mengecek tabrakan dengan semua dinding
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



// merender tampilan 2D
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
}

// merender tampilan 3D
void render3D() {
    setOrthoViewport(MAP_WIDTH, 0, MAP_WIDTH, MAP_HEIGHT, 0, MAP_WIDTH, MAP_HEIGHT, 0);

    // menggambar langit ruangan
    drawQuad(0, 0, MAP_WIDTH, MAP_HEIGHT / 2, 0.56f, 0.44f, 0.0f);

    // menggambar lantai ruangan
    drawQuad(0, MAP_HEIGHT / 2, MAP_WIDTH, MAP_HEIGHT, 0.86f, 0.75f, 0.14f);

    // menggambar dinding berdasarkan jarak dari player
    float slice_width = (float)MAP_WIDTH / NUM_RAYS;
    for (int i = 0; i < NUM_RAYS; i++) {
        if (rays[i].hit) {
            // mengoreksi jarak untuk efek fish-eye (agar dinding tidak terlihat melengkung)
            float corrected_distance = rays[i].distance * std::cos(rays[i].angle * M_PI / 180.0f);

            // menghitung tinggi dinding berdasarkan jarak
            float wall_height = (10.0f / corrected_distance) * MAP_HEIGHT;
            if (wall_height > MAP_HEIGHT) wall_height = MAP_HEIGHT;

            // menghitung kecerahan berdasarkan jarak (semakin jauh semakin gelap)
            float brightness = 1.0f - (corrected_distance / RAY_LENGTH);
            if (brightness > 1.0f) brightness = 1.0f;
            if (brightness < 0.2f) brightness = 0.2f;

            float x = i * slice_width;
            float y = (MAP_HEIGHT - wall_height) / 2.0f;

            drawQuad(x, y, x + slice_width, y + wall_height, brightness, brightness, 0.016f);
        }
        // jika sinar tidak mengenai dinding, tidak perlu menggambar apapun
    }
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
    glutCreateWindow("Maze Kelompok 1");

    init();
    initMaze();
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