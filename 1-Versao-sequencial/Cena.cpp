// Cena 3D: vetores, materiais, esferas, chao, luz, camera e interseccao raio-objeto.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//Matematica
struct Vec3 {
    double x, y, z;
    Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
};

static double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static double comprimento(const Vec3& a) { return sqrt(dot(a, a)); }
static Vec3 normalizar(const Vec3& a) { return a * (1.0 / comprimento(a)); }
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static Vec3 refletir(const Vec3& d, const Vec3& n) { return d - n * (2.0 * dot(d, n)); }
static double limitar(double v, double lo, double hi) { return min(hi, max(lo, v)); }

//CENA
struct Raio { Vec3 origem, direcao; };

struct Material { Vec3 cor; double ka, kd, ks, brilho, reflexo; };
struct Esfera { Vec3 centro; double raio; Material mat; };
struct Hit { Vec3 ponto, normal; Material mat; };

struct Cena {
    vector<Esfera> esferas;
    Material chao;
    double yChao = -1.0;
    Vec3 luz;
    Vec3 camera;
    double fov = 60.0, yaw = 0.0, pitch = 0.0;
};

static Cena criarCena() {
    Cena c;
    c.camera = Vec3(0, 0.5, 3.5);
    c.luz = Vec3(0, 8, -1);
    c.chao = {Vec3(0.8, 0.8, 0.8), 0.15, 0.8, 0.2, 20, 0.0};
    c.esferas.push_back({Vec3(0, 0, -1), 1.0, {Vec3(0.9, 0.2, 0.2), 0.25, 0.7, 0.6, 60, 0.35}});
    c.esferas.push_back({Vec3(2.2, -0.4, -2.0), 0.6, {Vec3(0.2, 0.3, 0.9), 0.25, 0.7, 0.6, 60, 0.35}});
    return c;
}

static const double EPS = 1e-4;

static bool intersecta(const Cena& cena, const Raio& r, double tmax, Hit& hit) {
    bool achou = false;
    double melhor = tmax;

    for (const Esfera& e : cena.esferas) {
        Vec3 oc = r.origem - e.centro;
        double b = dot(oc, r.direcao);
        double disc = b * b - (dot(oc, oc) - e.raio * e.raio);
        if (disc < 0) continue;

        double s = sqrt(disc), t = -b - s;
        if (t < EPS) t = -b + s;
        if (t < EPS || t >= melhor) continue;

        melhor = t;
        achou = true;
        hit.ponto = r.origem + r.direcao * t;
        hit.normal = normalizar(hit.ponto - e.centro);
        hit.mat = e.mat;
    }

    if (fabs(r.direcao.y) > 1e-9) {   // chao
        double t = (cena.yChao - r.origem.y) / r.direcao.y;
        if (t > EPS && t < melhor) {
            achou = true;
            hit.ponto = r.origem + r.direcao * t;
            hit.normal = Vec3(0, 1, 0);
            hit.mat = cena.chao;

            if (((int)floor(hit.ponto.x) + (int)floor(hit.ponto.z)) & 1)
                hit.mat.cor = Vec3(0.15, 0.15, 0.15);

            double f = limitar((t - 12.0) / 30.0, 0.0, 1.0);
            hit.mat.cor = hit.mat.cor * (1.0 - f) + Vec3(0.475, 0.475, 0.475) * f;
        }
    }
    return achou;
}