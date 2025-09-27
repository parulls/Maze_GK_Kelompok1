#include <glut.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>

#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Dimensi windows
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 400;
const int MAP_WIDTH = WINDOW_WIDTH / 2;
const int MAP_HEIGHT = WINDOW_HEIGHT;

// Untuk mengatur status permainan
enum GameState {
    PLAYING,
    WON,
    GAME_OVER,
    MENU
};

// Properti player
struct Player {
    float x, y;        // posisi player
    float dir;         // arah hadap player (dalam derajat)
    float radius;      // radius untuk collision detection
    bool hasKey;       // apakah player sudah mengambil kunci
    int lives;         // jumlah nyawa
    bool invulnerable; // status kebal sementara setelah terkena damage
    float invulnerableTime; // waktu kebal tersisa

    Player(float px, float py) : x(px), y(py), dir(0.0f), radius(8.0f), hasKey(false),
        lives(3), invulnerable(false), invulnerableTime(0.0f) {
    }
};

// Struct untuk dinding maze
struct Wall {
    float x1, y1, x2, y2;  // koordinat titik awal dan akhir dinding

    Wall(float px1, float py1, float px2, float py2)
        : x1(px1), y1(py1), x2(px2), y2(py2) {
    }
};

// Struct untuk sinar pancaran dari player (ray casting)
struct Ray {
    float x, y;        // titik asal sinar
    float dx, dy;      // arah sinar
    float angle;       // sudut sinar relatif terhadap player
    float distance;    // jarak sampai tabrakan
    bool hit;          // apakah sinar mengenai dinding
    float hitX, hitY;  // koordinat titik tabrakan
    int hitType;       // 0=wall, 1=goal, 2=obstacle

    Ray(float angle_offset) : angle(angle_offset), distance(500.0f), hit(false), hitType(0) {}
};

// Struct untuk goal (pintu keluar)
struct Goal {
    float x, y;        // posisi goal
    float width, height; // ukuran goal
    bool requiresKey;   // apakah memerlukan kunci

    Goal(float px, float py, float w, float h, bool needsKey = false)
        : x(px), y(py), width(w), height(h), requiresKey(needsKey) {
    }
};

// Struct untuk obstacle (rintangan bergerak)
struct Obstacle {
    float x, y;        // posisi obstacle
    float radius;      // radius obstacle
    float speed;       // kecepatan gerak
    float direction;   // arah gerak
    float minX, maxX, minY, maxY; // batas area gerak

    Obstacle(float px, float py, float r, float spd, float dir,
        float minx, float maxx, float miny, float maxy)
        : x(px), y(py), radius(r), speed(spd), direction(dir),
        minX(minx), maxX(maxx), minY(miny), maxY(maxy) {
    }
};

// Struct untuk key/kunci
struct Key {
    float x, y;        // posisi kunci
    float size;        // ukuran kunci
    bool collected;    // apakah sudah diambil

    Key(float px, float py, float s) : x(px), y(py), size(s), collected(false) {}
};

// Variabel global
Player player(10.0f, 10.0f);               // Player dimulai di posisi (60, 60)
std::vector<Wall> maze;                     // Daftar dinding maze
std::vector<Ray> rays;                      // Daftar sinar untuk ray casting
std::vector<Obstacle> obstacles;            // Daftar rintangan bergerak
Goal goal(540.0f, 180.0f, 30.0f, 40.0f, true); // Goal di ujung maze, memerlukan kunci
Key gameKey(320.0f, 200.0f, 8.0f);        // Kunci di tengah maze

// Konstanta ray casting
const float RAY_LENGTH = 500.0f;
const int NUM_RAYS = 600;
const float FOV = 60.0f;

// Game state variables
GameState gameState = PLAYING;
clock_t startTime;                          // waktu mulai permainan
const float GAME_DURATION = 60.0f;         // durasi permainan dalam detik (1 menit)
float gameTime = 0.0f;                     // waktu bermain saat ini
bool keys[256] = { false };                // status keyboard

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());

// Fungsi untuk mendapatkan waktu saat ini dalam detik
float getCurrentTime() {
    return (float)(clock() - startTime) / CLOCKS_PER_SEC;
}

// Fungsi untuk mendapatkan waktu tersisa
float getTimeRemaining() {
    float elapsed = getCurrentTime();
    return std::max(0.0f, GAME_DURATION - elapsed);
}

// Fungsi untuk mengecek apakah posisi aman (tidak bertabrakan dengan dinding)
bool isSafePosition(float x, float y, float radius) {
    for (const auto& wall : maze) {
        float distance = std::sqrt((x - wall.x1) * (x - wall.x1) + (y - wall.y1) * (y - wall.y1));
        if (distance < radius + 20.0f) return false;

        distance = std::sqrt((x - wall.x2) * (x - wall.x2) + (y - wall.y2) * (y - wall.y2));
        if (distance < radius + 20.0f) return false;

        // Cek jarak ke garis
        float line_length = std::sqrt((wall.x2 - wall.x1) * (wall.x2 - wall.x1) + (wall.y2 - wall.y1) * (wall.y2 - wall.y1));
        if (line_length == 0) continue;

        float t = ((x - wall.x1) * (wall.x2 - wall.x1) + (y - wall.y1) * (wall.y2 - wall.y1)) / (line_length * line_length);
        t = std::max(0.0f, std::min(1.0f, t));

        float closest_x = wall.x1 + t * (wall.x2 - wall.x1);
        float closest_y = wall.y1 + t * (wall.y2 - wall.y1);

        distance = std::sqrt((x - closest_x) * (x - closest_x) + (y - closest_y) * (y - closest_y));
        if (distance < radius + 15.0f) return false;
    }
    return true;
}

// Fungsi untuk menghasilkan posisi random yang aman
std::pair<float, float> generateSafeRandomPosition(float radius) {
    std::uniform_real_distribution<float> xDist(50.0f, 550.0f);
    std::uniform_real_distribution<float> yDist(50.0f, 350.0f);

    int attempts = 0;
    while (attempts < 100) {
        float x = xDist(gen);
        float y = yDist(gen);

        // Pastikan tidak terlalu dekat dengan player spawn dan goal
        if (std::sqrt((x - 60) * (x - 60) + (y - 60) * (y - 60)) < 80) {
            attempts++;
            continue;
        }
        if (std::sqrt((x - goal.x) * (x - goal.x) + (y - goal.y) * (y - goal.y)) < 80) {
            attempts++;
            continue;
        }

        if (isSafePosition(x, y, radius)) {
            return std::make_pair(x, y);
        }
        attempts++;
    }

    // Fallback positions jika tidak bisa menemukan posisi yang aman
    std::vector<std::pair<float, float>> fallbackPositions = {
        {150, 150}, {300, 100}, {400, 250}, {200, 300}, {450, 150}
    };

    std::uniform_int_distribution<int> fallbackDist(0, fallbackPositions.size() - 1);
    return fallbackPositions[fallbackDist(gen)];
}

// Inisialisasi maze dengan dinding-dinding
void initMaze() {
    // Data maze dalam format [x1, y1, x2, y2] untuk setiap dinding
    std::vector<std::vector<float>> maze_data = {
        // Baris atas
        {0, 0, 40, 0}, {0, 40, 40, 40}, {0, 0, 0, 40}, {40, 0, 80, 0},
        {40, 40, 80, 40}, {80, 0, 120, 0}, {120, 0, 120, 40}, {120, 0, 160, 0},
        {120, 40, 160, 40}, {160, 0, 200, 0}, {200, 0, 240, 0}, {200, 40, 240, 40},
        {240, 0, 280, 0}, {280, 0, 320, 0}, {320, 0, 320, 40}, {320, 0, 360, 0},
        {360, 0, 400, 0}, {360, 40, 400, 40}, {400, 0, 440, 0}, {400, 40, 440, 40},
        {440, 0, 480, 0}, {440, 40, 480, 40}, {480, 0, 520, 0}, {520, 0, 560, 0},
        {560, 0, 560, 40}, {560, 0, 600, 0}, {600, 0, 600, 40},

        // Baris kedua
        {0, 40, 0, 80}, {40, 80, 80, 80}, {80, 40, 80, 80}, {80, 80, 120, 80},
        {160, 40, 160, 80}, {200, 40, 200, 80}, {240, 40, 240, 80}, {280, 40, 280, 80},
        {320, 40, 320, 80}, {360, 40, 360, 80}, {400, 80, 440, 80}, {440, 80, 480, 80},
        {480, 80, 520, 80}, {520, 40, 520, 80}, {560, 40, 560, 80}, {600, 40, 600, 80},

        // Baris ketiga
        {0, 80, 0, 120}, {40, 120, 80, 120}, {80, 120, 120, 120}, {120, 120, 160, 120},
        {160, 80, 160, 120}, {160, 120, 200, 120}, {240, 80, 240, 120}, {280, 80, 280, 120},
        {320, 80, 320, 120}, {320, 120, 360, 120}, {360, 80, 360, 120}, {400, 80, 400, 120},
        {440, 120, 480, 120}, {480, 120, 520, 120}, {520, 120, 560, 120}, {560, 120, 600, 120},
        {600, 80, 600, 120},

        // Baris keempat
        {0, 160, 40, 160}, {0, 120, 0, 160}, {40, 160, 80, 160}, {80, 160, 120, 160},
        {160, 120, 160, 160}, {200, 120, 200, 160}, {240, 120, 240, 160}, {280, 120, 280, 160},
        {280, 160, 320, 160}, {320, 160, 360, 160}, {400, 120, 400, 160}, {400, 160, 440, 160},
        {440, 120, 440, 160}, {480, 160, 520, 160}, {520, 160, 560, 160}, {600, 120, 600, 160},

        // Baris kelima
        {0, 200, 40, 200}, {0, 160, 0, 200}, {80, 200, 120, 200}, {120, 160, 120, 200},
        {160, 160, 160, 200}, {200, 200, 240, 200}, {240, 160, 240, 200}, {240, 200, 280, 200},
        {280, 200, 320, 200}, {360, 160, 360, 200}, {360, 200, 400, 200}, {400, 160, 400, 200},
        {440, 200, 480, 200}, {480, 160, 480, 200}, {520, 200, 560, 200}, {560, 160, 560, 200},
        {600, 160, 600, 200},

        // Baris keenam
        {0, 200, 0, 240}, {40, 240, 80, 240}, {80, 200, 80, 240}, {120, 240, 160, 240},
        {160, 200, 160, 240}, {160, 240, 200, 240}, {200, 240, 240, 240}, {240, 240, 280, 240},
        {320, 200, 320, 240}, {360, 200, 360, 240}, {400, 240, 440, 240}, {440, 200, 440, 240},
        {440, 240, 480, 240}, {520, 200, 520, 240}, {600, 200, 600, 240},

        // Baris ketujuh
        {0, 240, 0, 280}, {40, 240, 40, 280}, {80, 280, 120, 280}, {120, 240, 120, 280},
        {160, 280, 200, 280}, {200, 280, 240, 280}, {280, 240, 280, 280}, {320, 240, 320, 280},
        {360, 240, 360, 280}, {400, 240, 400, 280}, {440, 280, 480, 280}, {480, 280, 520, 280},
        {520, 240, 520, 280}, {560, 240, 560, 280}, {600, 240, 600, 280},

        // Baris kedelapan
        {0, 280, 0, 320}, {40, 280, 40, 320}, {40, 320, 80, 320}, {80, 320, 120, 320},
        {120, 320, 160, 320}, {160, 280, 160, 320}, {200, 280, 200, 320}, {240, 320, 280, 320},
        {280, 280, 280, 320}, {320, 280, 320, 320}, {320, 320, 360, 320}, {360, 320, 400, 320},
        {400, 320, 440, 320}, {440, 280, 440, 320}, {480, 320, 520, 320}, {520, 320, 560, 320},
        {560, 280, 560, 320}, {600, 280, 600, 320},

        // Baris kesembilan
        {0, 320, 0, 360}, {40, 320, 40, 360}, {80, 360, 120, 360}, {120, 360, 160, 360},
        {160, 360, 200, 360}, {200, 360, 240, 360}, {240, 320, 240, 360}, {240, 360, 280, 360},
        {280, 360, 320, 360}, {320, 360, 360, 360}, {360, 320, 360, 360}, {440, 320, 440, 360},
        {440, 360, 480, 360}, {480, 360, 520, 360}, {560, 320, 560, 360}, {600, 320, 600, 360},

        // Baris bawah
        {0, 400, 40, 400}, {0, 360, 0, 400}, {40, 400, 80, 400}, {80, 400, 120, 400},
        {120, 400, 160, 400}, {160, 400, 200, 400}, {200, 400, 240, 400}, {240, 400, 280, 400},
        {280, 400, 320, 400}, {320, 400, 360, 400}, {360, 400, 400, 400}, {400, 360, 400, 400},
        {400, 400, 440, 400}, {440, 400, 480, 400}, {480, 400, 520, 400}, {520, 400, 560, 400},
        {560, 360, 560, 400}, {560, 400, 600, 400}, {600, 360, 600, 400}
    };

    // Membuat objek Wall dari setiap data dinding
    for (const auto& wall_data : maze_data) {
        maze.emplace_back(wall_data[0], wall_data[1], wall_data[2], wall_data[3]);
    }
}

// Inisialisasi rintangan bergerak dengan posisi random
void initObstacles() {
    obstacles.clear();

    std::uniform_real_distribution<float> speedDist(0.8f, 2.5f);
    std::uniform_real_distribution<float> dirDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> radiusDist(8.0f, 15.0f);

    // Buat 4-6 obstacles dengan posisi random
    std::uniform_int_distribution<int> numObstacles(4, 6);
    int numObs = numObstacles(gen);

    for (int i = 0; i < numObs; i++) {
        float radius = radiusDist(gen);
        auto pos = generateSafeRandomPosition(radius);
        float speed = speedDist(gen);
        float direction = dirDist(gen);

        // Set batas area gerak di sekitar posisi spawn
        float minX = std::max(50.0f, pos.first - 100.0f);
        float maxX = std::min(550.0f, pos.first + 100.0f);
        float minY = std::max(50.0f, pos.second - 80.0f);
        float maxY = std::min(350.0f, pos.second + 80.0f);

        obstacles.emplace_back(pos.first, pos.second, radius, speed, direction,
            minX, maxX, minY, maxY);
    }
}

// Inisialisasi sinar untuk ray casting
void initRays() {
    rays.clear();
    float angle_step = FOV / NUM_RAYS;      // langkah sudut antar sinar
    float start_angle = FOV / 2.0f;       // sudut awal (setengah FOV ke kiri)

    for (int i = 0; i < NUM_RAYS; i++) {
        rays.emplace_back(start_angle + i * angle_step);
    }
}

// Inisialisasi posisi key secara random
void initKey() {
    auto pos = generateSafeRandomPosition(gameKey.size);
    gameKey.x = pos.first;
    gameKey.y = pos.second;
    gameKey.collected = false;
}

// Fungsi untuk menghitung intersection antara dua garis (untuk ray casting)
bool lineIntersection(float x1, float y1, float x2, float y2,
    float x3, float y3, float x4, float y4,
    float& ix, float& iy) {

    // Menggunakan rumus intersection dua garis parametrik
    float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(denom) < 1e-10) return false;  // garis paralel

    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    // Cek apakah intersection dalam segment kedua garis
    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        ix = x1 + t * (x2 - x1);
        iy = y1 + t * (y2 - y1);
        return true;
    }
    return false;
}

// Fungsi untuk mengecek intersection ray dengan goal (pintu)
bool rayGoalIntersection(const Ray& ray, float& distance, float& ix, float& iy) {
    float rayEndX = ray.x + ray.dx * RAY_LENGTH;
    float rayEndY = ray.y + ray.dy * RAY_LENGTH;

    // Definisikan keempat sisi goal sebagai array of line segments
    struct LineSegment {
        float x1, y1, x2, y2;
    };

    LineSegment goalSides[4] = {
        {goal.x, goal.y, goal.x + goal.width, goal.y},                    // top
        {goal.x + goal.width, goal.y, goal.x + goal.width, goal.y + goal.height}, // right
        {goal.x + goal.width, goal.y + goal.height, goal.x, goal.y + goal.height}, // bottom
        {goal.x, goal.y + goal.height, goal.x, goal.y}                   // left
    };

    float closestDist = RAY_LENGTH;
    bool found = false;

    for (int i = 0; i < 4; i++) {
        float tempX, tempY;
        if (lineIntersection(ray.x, ray.y, rayEndX, rayEndY,
            goalSides[i].x1, goalSides[i].y1,
            goalSides[i].x2, goalSides[i].y2,
            tempX, tempY)) {
            float dist = std::sqrt((tempX - ray.x) * (tempX - ray.x) + (tempY - ray.y) * (tempY - ray.y));
            if (dist < closestDist) {
                closestDist = dist;
                ix = tempX;
                iy = tempY;
                found = true;
            }
        }
    }

    if (found) {
        distance = closestDist;
        return true;
    }
    return false;
}

// Fungsi untuk mengecek intersection ray dengan obstacle
bool rayObstacleIntersection(const Ray& ray, const Obstacle& obstacle, float& distance, float& ix, float& iy) {
    // Menggunakan formula intersection ray dengan lingkaran
    float dx = ray.dx;
    float dy = ray.dy;
    float fx = ray.x - obstacle.x;
    float fy = ray.y - obstacle.y;
    
    float a = dx * dx + dy * dy;
    float b = 2 * (fx * dx + fy * dy);
    float c = (fx * fx + fy * fy) - obstacle.radius * obstacle.radius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return false; // Tidak ada intersection
    }
    
    float t1 = (-b - std::sqrt(discriminant)) / (2 * a);
    float t2 = (-b + std::sqrt(discriminant)) / (2 * a);
    
    float t = (t1 > 0) ? t1 : t2;
    if (t > 0 && t <= RAY_LENGTH) {
        ix = ray.x + t * dx;
        iy = ray.y + t * dy;
        distance = t;
        return true;
    }
    
    return false;
}

// Menghitung jarak dari titik ke garis (untuk collision detection)
float pointToLineDistance(float px, float py, float x1, float y1, float x2, float y2) {
    float line_length = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    if (line_length == 0) {
        return std::sqrt((px - x1) * (px - x1) + (py - y1) * (py - y1));
    }

    // Proyeksi titik ke garis
    float t = ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / (line_length * line_length);
    t = std::max(0.0f, std::min(1.0f, t));

    float closest_x = x1 + t * (x2 - x1);
    float closest_y = y1 + t * (y2 - y1);

    return std::sqrt((px - closest_x) * (px - closest_x) + (py - closest_y) * (py - closest_y));
}

// Mengecek collision player dengan dinding
bool checkWallCollision(float x, float y) {
    for (const auto& wall : maze) {
        if (pointToLineDistance(x, y, wall.x1, wall.y1, wall.x2, wall.y2) < player.radius) {
            return true;
        }
    }
    return false;
}

// Mengecek collision player dengan obstacle
bool checkObstacleCollision(float x, float y) {
    if (player.invulnerable) return false;

    for (const auto& obstacle : obstacles) {
        float dx = x - obstacle.x;
        float dy = y - obstacle.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < player.radius + obstacle.radius) {
            return true;
        }
    }
    return false;
}

// Fungsi untuk menangani tabrakan dengan obstacle
void handleObstacleCollision() {
    if (!player.invulnerable) {
        player.lives--;
        player.invulnerable = true;
        player.invulnerableTime = 2.0f; // 2 detik kebal

        std::cout << "BOOM! Nyawa tersisa: " << player.lives << std::endl;

        if (player.lives <= 0) {
            gameState = GAME_OVER;
            std::cout << "Game Over!!" << std::endl;
        }
    }
}

// Mengecek collision player dengan kunci
void checkKeyCollection() {
    if (!gameKey.collected) {
        float dx = player.x - gameKey.x;
        float dy = player.y - gameKey.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < player.radius + gameKey.size) {
            gameKey.collected = true;
            player.hasKey = true;
            std::cout << "Kunci berhasil diambil! Sekarang Anda bisa membuka pintu!" << std::endl;
        }
    }
}

// Mengecek apakah player mencapai goal
void checkGoalReached() {
    float dx = player.x - (goal.x + goal.width / 2);
    float dy = player.y - (goal.y + goal.height / 2);
    float distance = std::sqrt(dx * dx + dy * dy);

    // Jika player dekat dengan goal dan memiliki kunci (jika diperlukan)
    if (distance < player.radius + 20.0f && (!goal.requiresKey || player.hasKey)) {
        gameState = WON;
        gameTime = getCurrentTime();
        std::cout << "Selamat! Anda menyelesaikan maze dalam " << std::fixed << std::setprecision(1) << gameTime << " detik!" << std::endl;
    }
}

// Update posisi obstacles
void updateObstacles() {
    for (auto& obstacle : obstacles) {
        float radians = obstacle.direction * M_PI / 180.0f;
        float dx = obstacle.speed * cos(radians);
        float dy = obstacle.speed * sin(radians);

        float newX = obstacle.x + dx;
        float newY = obstacle.y + dy;

        // Pantulkan jika mencapai batas area
        if (newX <= obstacle.minX || newX >= obstacle.maxX) {
            obstacle.direction = 180.0f - obstacle.direction;
        }
        if (newY <= obstacle.minY || newY >= obstacle.maxY) {
            obstacle.direction = -obstacle.direction;
        }

        // Update posisi
        obstacle.x = std::max(obstacle.minX, std::min(obstacle.maxX, newX));
        obstacle.y = std::max(obstacle.minY, std::min(obstacle.maxY, newY));
    }
}

// Update player invulnerability
void updatePlayer() {
    if (player.invulnerable) {
        player.invulnerableTime -= 0.016f; // Kurangi waktu kebal (asumsi 60 FPS)
        if (player.invulnerableTime <= 0) {
            player.invulnerable = false;
            player.invulnerableTime = 0;
        }
    }
}

// Fungsi translasi player berdasarkan transformasi geometri 2D
void translatePlayer(float dx, float dy) {
    float new_x = player.x + dx;
    float new_y = player.y + dy;

    // Cek collision dengan dinding sebelum bergerak
    if (!checkWallCollision(new_x, player.y)) {
        // Cek collision dengan obstacle
        if (checkObstacleCollision(new_x, player.y)) {
            handleObstacleCollision();
        }
        else {
            player.x = new_x;
        }
    }

    if (!checkWallCollision(player.x, new_y)) {
        // Cek collision dengan obstacle
        if (checkObstacleCollision(player.x, new_y)) {
            handleObstacleCollision();
        }
        else {
            player.y = new_y;
        }
    }
}

// Ray casting untuk mendeteksi dinding, pintu, dan rintangan
void castRays() {
    for (auto& ray : rays) {
        float ray_angle = (player.dir + ray.angle) * M_PI / 180.0f;
        ray.x = player.x;
        ray.y = player.y;
        ray.dx = std::cos(ray_angle);
        ray.dy = std::sin(ray_angle);

        // Nilai default jika tidak ada tabrakan
        ray.distance = RAY_LENGTH;
        ray.hit = false;
        ray.hitX = ray.x + ray.dx * RAY_LENGTH;
        ray.hitY = ray.y + ray.dy * RAY_LENGTH;
        ray.hitType = 0; // default: wall

        // Cek tabrakan dengan semua dinding
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
                    ray.hitType = 0; // wall
                }
            }
        }

        // Cek tabrakan dengan goal (pintu)
        float goalDist, goalX, goalY;
        if (rayGoalIntersection(ray, goalDist, goalX, goalY)) {
            if (goalDist < ray.distance) {
                ray.distance = goalDist;
                ray.hitX = goalX;
                ray.hitY = goalY;
                ray.hit = true;
                ray.hitType = 1; // goal/door
            }
        }

        // Cek tabrakan dengan obstacles
        for (const auto& obstacle : obstacles) {
            float obsDist, obsX, obsY;
            if (rayObstacleIntersection(ray, obstacle, obsDist, obsX, obsY)) {
                if (obsDist < ray.distance) {
                    ray.distance = obsDist;
                    ray.hitX = obsX;
                    ray.hitY = obsY;
                    ray.hit = true;
                    ray.hitType = 2; // obstacle
                }
            }
        }
    }
}

// Set viewport dan proyeksi orthogonal
void setOrthoViewport(int x, int y, int width, int height,
    float left, float right, float bottom, float top) {
    glViewport(x, y, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Menggambar persegi panjang dengan warna tertentu
void drawQuad(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

// Menggambar lingkaran
void drawCircle(float x, float y, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; i++) {
        float angle = i * 2.0f * M_PI / 32.0f;
        glVertex2f(x + radius * cos(angle), y + radius * sin(angle));
    }
    glEnd();
}

// Render teks sederhana menggunakan bitmap
void renderBitmapString(float x, float y, void* font, const std::string& string) {
    glRasterPos2f(x, y);
    for (char c : string) {
        glutBitmapCharacter(font, c);
    }
}

// Render tampilan 2D (top-down view)
void render2D() {
    setOrthoViewport(0, 0, MAP_WIDTH, MAP_HEIGHT, 0, 600, 400, 0);

    // Gambar maze walls
    glColor3f(0.4f, 0.6f, 0.6f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (const auto& wall : maze) {
        glVertex2f(wall.x1, wall.y1);
        glVertex2f(wall.x2, wall.y2);
    }
    glEnd();

    // Gambar rays dari player[
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (const auto& ray : rays) {
        if (ray.hit) {
            if (ray.hitType == 0) glColor3f(0.9f, 0.9f, 0.9f);      // wall - putih
            else if (ray.hitType == 1) glColor3f(0.0f, 1.0f, 0.0f); // goal - hijau
            else if (ray.hitType == 2) glColor3f(1.0f, 0.0f, 0.0f); // obstacle - merah
        }
        else {
            glColor3f(0.3f, 0.3f, 0.3f);
        }
        glVertex2f(ray.x, ray.y);
        glVertex2f(ray.hitX, ray.hitY);
    }
    glEnd();

    // Gambar obstacles
    for (const auto& obstacle : obstacles) {
        drawCircle(obstacle.x, obstacle.y, obstacle.radius, 0.8f, 0.2f, 0.2f);
    }

    // Gambar kunci jika belum diambil
    if (!gameKey.collected) {
        drawCircle(gameKey.x, gameKey.y, gameKey.size, 1.0f, 1.0f, 0.0f);
    }

    // Gambar goal
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(goal.x, goal.y);
    glVertex2f(goal.x + goal.width, goal.y);
    glVertex2f(goal.x + goal.width, goal.y + goal.height);
    glVertex2f(goal.x, goal.y + goal.height);
    glEnd();

    // Gambar player dengan efek kedip jika invulnerable
    if (!player.invulnerable || (int)(getCurrentTime() * 10) % 2 == 0) {
        drawCircle(player.x, player.y, player.radius, 0.7f, 0.2f, 0.5f);

    }
}

// Render tampilan 3D (first-person view) dengan pintu dan rintangan
void render3D() {
    setOrthoViewport(MAP_WIDTH, 0, MAP_WIDTH, MAP_HEIGHT, 0, MAP_WIDTH, MAP_HEIGHT, 0);

    // Gambar langit/ceiling dengan warna abu-abu terang
    drawQuad(0, 0, MAP_WIDTH, MAP_HEIGHT / 2, 0.7f, 0.7f, 0.8f);

    // Gambar lantai dengan warna yang lebih gelap dari dinding
    drawQuad(0, MAP_HEIGHT / 2, MAP_WIDTH, MAP_HEIGHT, 0.2f, 0.3f, 0.3f);

    // Gambar dinding, pintu, dan rintangan berdasarkan ray casting
    float slice_width = (float)MAP_WIDTH / NUM_RAYS;
    for (int i = 0; i < NUM_RAYS; i++) {
        if (rays[i].hit) {
            // Koreksi fish-eye effect
            float corrected_distance = rays[i].distance * std::cos(rays[i].angle * M_PI / 180.0f);

            // Hitung tinggi berdasarkan jarak
            float wall_height = (20.0f / corrected_distance) * MAP_HEIGHT;
            if (wall_height > MAP_HEIGHT) wall_height = MAP_HEIGHT;

            // Hitung kecerahan berdasarkan jarak
            float brightness = 1.0f - (corrected_distance / RAY_LENGTH);
            if (brightness > 1.0f) brightness = 1.0f;
            if (brightness < 0.1f) brightness = 0.1f;

            float x = i * slice_width;
            float y = (MAP_HEIGHT - wall_height) / 2.0f;

            // Pilih warna berdasarkan tipe objek yang terkena ray
            if (rays[i].hitType == 0) {
                // Dinding - warna abu-abu kebiruan dengan brightness
                drawQuad(x, y, x + slice_width, y + wall_height,
                    brightness * 0.5f, brightness * 0.6f, brightness * 0.6f);
            }
            else if (rays[i].hitType == 1) {
                // Pintu/Goal - warna hijau dengan efek berkilau
                float greenIntensity = brightness * (0.8f + 0.2f * sin(getCurrentTime() * 5.0f));
                if (!player.hasKey) {
                    // Pintu terkunci - warna hijau gelap dengan efek merah
                    drawQuad(x, y, x + slice_width, y + wall_height,
                        brightness * 0.3f, greenIntensity * 0.6f, brightness * 0.1f);
                } else {
                    // Pintu terbuka - warna hijau cerah
                    drawQuad(x, y, x + slice_width, y + wall_height,
                        brightness * 0.2f, greenIntensity, brightness * 0.2f);
                }
            }
            else if (rays[i].hitType == 2) {
                // Obstacle - warna merah dengan efek pulsing
                float redIntensity = brightness * (0.7f + 0.3f * sin(getCurrentTime() * 8.0f));
                drawQuad(x, y, x + slice_width, y + wall_height,
                    redIntensity, brightness * 0.1f, brightness * 0.1f);
            }
        }
    }

    // Gambar HUD (Head-Up Display) di tampilan 3D
    glColor3f(1.0f, 1.0f, 1.0f);

    // Status kunci
    if (player.hasKey) {
        renderBitmapString(10, MAP_HEIGHT - 30, GLUT_BITMAP_HELVETICA_18, "KEY: COLLECTED");
        glColor3f(1.0f, 1.0f, 0.0f);
        drawCircle(200, MAP_HEIGHT - 25, 8, 1.0f, 1.0f, 0.0f);
    }
    else {
        glColor3f(0.7f, 0.7f, 0.7f);
        renderBitmapString(10, MAP_HEIGHT - 30, GLUT_BITMAP_HELVETICA_18, "FIND THE KEY!");
    }

    // Nyawa di 3D view
    glColor3f(1.0f, 1.0f, 1.0f);
    std::ostringstream livesStr;
    livesStr << "LIVES: " << player.lives;
    renderBitmapString(10, MAP_HEIGHT - 55, GLUT_BITMAP_HELVETICA_18, livesStr.str());

    // Timer di 3D view
    float timeRemaining = getTimeRemaining();
    std::ostringstream timeStr;
    int minutes = (int)timeRemaining / 60;
    int seconds = (int)timeRemaining % 60;
    timeStr << "TIME: " << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    if (timeRemaining <= 10.0f) {
        glColor3f(1.0f, 0.0f, 0.0f);
    }
    else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    renderBitmapString(MAP_WIDTH - 150, MAP_HEIGHT - 30, GLUT_BITMAP_HELVETICA_18, timeStr.str());

    // Status invulnerable
    if (player.invulnerable) {
        glColor3f(1.0f, 0.0f, 1.0f);
        renderBitmapString(10, MAP_HEIGHT - 80, GLUT_BITMAP_HELVETICA_18, "INVULNERABLE!");
    }

    // Crosshair (bidikan tengah)
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    // Garis horizontal
    glVertex2f(MAP_WIDTH / 2 - 10, MAP_HEIGHT / 2);
    glVertex2f(MAP_WIDTH / 2 + 10, MAP_HEIGHT / 2);
    // Garis vertikal
    glVertex2f(MAP_WIDTH / 2, MAP_HEIGHT / 2 - 10);
    glVertex2f(MAP_WIDTH / 2, MAP_HEIGHT / 2 + 10);
    glEnd();

    // Indikator arah ke pintu jika sudah punya kunci
    if (player.hasKey) {
        glColor3f(0.0f, 1.0f, 0.0f);
        renderBitmapString(MAP_WIDTH - 180, MAP_HEIGHT - 55, GLUT_BITMAP_HELVETICA_12, "TEMUKAN PNTU KELUAR!");
    }
}

// Render layar kemenangan
void renderWinScreen() {
    setOrthoViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // Background kemenangan
    drawQuad(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.7f, 0.3f, 0.0f);

    // Judul kemenangan
    glColor3f(1.0f, 1.0f, 0.0f);
    renderBitmapString(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 50, GLUT_BITMAP_TIMES_ROMAN_24, "SELAMAT!");

    // Waktu penyelesaian
    std::ostringstream timeStr;
    timeStr << "Waktu: " << std::fixed << std::setprecision(2) << gameTime << " detik";
    glColor3f(1.0f, 1.0f, 1.0f);
    renderBitmapString(WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 - 10, GLUT_BITMAP_HELVETICA_18, timeStr.str());

    // Instruksi restart
    renderBitmapString(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 50, GLUT_BITMAP_HELVETICA_12, "Ketik R untuk mulai ulang atau ESC untuk keluar");
}

// Render layar game over
void renderGameOverScreen() {
    setOrthoViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    // Background game over
    drawQuad(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.3f, 0.0f, 0.0f);

    // Judul game over
    glColor3f(1.0f, 0.0f, 0.0f);
    renderBitmapString(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 + 50, GLUT_BITMAP_TIMES_ROMAN_24, "GAME OVER!");

    // Penyebab game over
    glColor3f(1.0f, 1.0f, 1.0f);
    if (player.lives <= 0) {
        renderBitmapString(WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 + 20, GLUT_BITMAP_HELVETICA_18, "Kamu kehabisan nyawa!");
    }
    if (getTimeRemaining() <= 0) {
        renderBitmapString(WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 + 20, GLUT_BITMAP_HELVETICA_18, "Waktu habis!");
    }

    // Instruksi restart
    renderBitmapString(WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 50, GLUT_BITMAP_HELVETICA_12, "Ketik R untuk mulai ulang atau ESC untuk keluar");
}

// Fungsi utama display
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == PLAYING) {
        render2D();   // Tampilan top-down di kiri
        render3D();   // Tampilan first-person di kanan
    }
    else if (gameState == WON) {
        renderWinScreen();  // Layar kemenangan
    }
    else if (gameState == GAME_OVER) {
        renderGameOverScreen(); // Layar game over
    }

    glutSwapBuffers();
}

// Update logika permainan
void update(int value) {
    if (gameState == PLAYING) {
        // Cek apakah waktu habis
        if (getTimeRemaining() <= 0) {
            gameState = GAME_OVER;
            std::cout << "Game Over! Waktu habis!" << std::endl;
            glutPostRedisplay();
            glutTimerFunc(16, update, 0);
            return;
        }

        float moveSpeed = 2.5f;    // Kecepatan gerak player
        float rotSpeed = 3.0f;     // Kecepatan rotasi player

        // Kontrol pergerakan dengan implementasi translasi manual
        if (keys['w'] || keys['W']) {
            // Maju sesuai arah hadap player
            translatePlayer(moveSpeed * cos(player.dir * M_PI / 180.0f),
                moveSpeed * sin(player.dir * M_PI / 180.0f));
        }
        if (keys['s'] || keys['S']) {
            // Mundur berlawanan arah hadap player
            translatePlayer(-moveSpeed * cos(player.dir * M_PI / 180.0f),
                -moveSpeed * sin(player.dir * M_PI / 180.0f));
        }
        if (keys['a'] || keys['A']) {
            player.dir -= rotSpeed; // Rotasi kiri
        }
        if (keys['d'] || keys['D']) {
            player.dir += rotSpeed; // Rotasi kanan
        }

        // Normalisasi sudut agar tetap dalam 0-360 derajat
        if (player.dir < 0) player.dir += 360;
        if (player.dir >= 360) player.dir -= 360;

        // Update sistem permainan
        updatePlayer();         // Update status player (invulnerability)
        updateObstacles();      // Update posisi rintangan
        checkKeyCollection();   // Cek apakah kunci diambil
        checkGoalReached();     // Cek apakah mencapai tujuan
        castRays();            // Update ray casting
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);  // ~60 FPS
}

// Reset permainan ke kondisi awal
void resetGame() {
    gameState = PLAYING;
    player.x = 60.0f;
    player.y = 60.0f;
    player.dir = 0.0f;
    player.hasKey = false;
    player.lives = 3;
    player.invulnerable = false;
    player.invulnerableTime = 0.0f;

    startTime = clock();
    gameTime = 0.0f;

    // Reset dan randomize obstacles
    initObstacles();

    // Reset dan randomize key position
    initKey();

    std::cout << "Game direset! Posisi kunci dan rintangan telah diacak ulang!" << std::endl;
    std::cout << "Anda memiliki " << player.lives << " nyawa dan " << GAME_DURATION << " detik untuk menyelesaikan maze!" << std::endl;
}

// Handling input keyboard saat tombol ditekan
void keyDown(unsigned char key, int x, int y) {
    keys[key] = true;

    if (key == 27) {  // ESC key
        exit(0);
    }

    if (key == 'r' || key == 'R') {  // Reset game
        resetGame();
    }
}

// Handling input keyboard saat tombol dilepas
void keyUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Inisialisasi OpenGL
void init() {
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);  // Background gelap
    glDisable(GL_DEPTH_TEST);                  // Tidak perlu depth testing untuk 2D
    glEnable(GL_BLEND);                        // Enable blending untuk transparansi
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);                  // Smooth lines
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

// Fungsi utama program
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Permainan Labirin - Kelompok 1");

    // Inisialisasi sistem
    init();
    initMaze();        // Buat maze
    initRays();        // Buat rays untuk ray casting
    initObstacles();   // Buat rintangan bergerak
    initKey();         // Inisialisasi posisi kunci

    // Set waktu mulai
    startTime = clock();

    // Register callback functions
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutTimerFunc(0, update, 0);

    // Instruksi permainan
    std::cout << "=== Game Labirin Kelompok 1 ===" << std::endl;
    std::cout << "  W/S - Maju/Mundur" << std::endl;
    std::cout << "  A/D - Belok Kiri/Kanan" << std::endl;
    std::cout << "  R   - Reset Game" << std::endl;
    std::cout << "  ESC - Keluar" << std::endl;
    std::cout << "Selamat bermain! Perhatikan tampilan 3D di sebelah kanan!" << std::endl;

    glutMainLoop();
    return 0;
}