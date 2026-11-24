// advanced_raytracer.cpp
#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include <vector>
#include <limits>
#include <cstdlib>

using std::shared_ptr;
using std::make_shared;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

// Vec3
struct vec3 {
    double x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(double a, double b, double c) : x(a), y(b), z(c) {}
    vec3 operator-() const { return vec3(-x, -y, -z); }
    vec3& operator+=(const vec3 &v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
    vec3 operator+(const vec3 &v) const { return vec3(x+v.x, y+v.y, z+v.z); }
    vec3 operator-(const vec3 &v) const { return vec3(x-v.x, y-v.y, z-v.z); }
    vec3 operator*(double t) const { return vec3(x*t, y*t, z*t); }
    vec3 operator/(double t) const { return *this * (1/t); }
    vec3 operator*(const vec3 &v) const { return vec3(x*v.x, y*v.y, z*v.z); }
    double length() const { return sqrt(x*x + y*y + z*z); }
    vec3 normalized() const { return *this / length(); }
};

inline vec3 operator*(double t, const vec3 &v) { return v * t; }
inline double dot(const vec3 &u, const vec3 &v) { return u.x*v.x + u.y*v.y + u.z*v.z; }
inline vec3 cross(const vec3 &u, const vec3 &v) {
    return vec3(u.y*v.z - u.z*v.y, u.z*v.x - u.x*v.z, u.x*v.y - u.y*v.x);
}
inline vec3 reflect(const vec3& v, const vec3& n) { return v - 2*dot(v,n)*n; }
inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    auto cos_theta = fmin(dot(-uv,n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length()*r_out_perp.length())) * n;
    return r_out_perp + r_out_parallel;
}

// Ray
struct ray {
    vec3 orig, dir;
    ray() {}
    ray(const vec3& o, const vec3& d) : orig(o), dir(d) {}
    vec3 at(double t) const { return orig + t*dir; }
};

// Material
class material;

struct hit_record {
    vec3 p;
    vec3 normal;
    double t;
    bool front_face;
    shared_ptr<material> mat_ptr;
    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.dir, outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const = 0;
};

// Sphere
struct sphere {
    vec3 center;
    double radius;
    shared_ptr<material> mat_ptr;
    sphere() {}
    sphere(vec3 c, double r, shared_ptr<material> m) : center(c), radius(r), mat_ptr(m) {}
    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
        vec3 oc = r.orig - center;
        auto a = dot(r.dir, r.dir);
        auto half_b = dot(oc, r.dir);
        auto c = dot(oc, oc) - radius*radius;
        auto discriminant = half_b*half_b - a*c;
        if (discriminant < 0) return false;
        auto sqrtd = sqrt(discriminant);

        double root = (-half_b - sqrtd)/a;
        if (root < t_min || root > t_max) {
            root = (-half_b + sqrtd)/a;
            if (root < t_min || root > t_max) return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center)/radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;
        return true;
    }
};

// Hittable list
struct hittable_list {
    std::vector<sphere> objects;
    void add(const sphere& obj) { objects.push_back(obj); }
    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
        hit_record temp_rec;
        bool hit_anything = false;
        double closest_so_far = t_max;
        for (const auto& obj : objects) {
            if (obj.hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
};

// Random helpers
inline double random_double() { return rand() / (RAND_MAX + 1.0); }
inline double random_double(double min, double max) { return min + (max-min)*random_double(); }
vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3(random_double(), random_double(), random_double())*2.0 - vec3(1,1,1);
        if (p.length()*p.length() >= 1) continue;
        return p;
    }
}

// Materials
class lambertian : public material {
public:
    vec3 albedo;
    lambertian(const vec3& a) : albedo(a) {}
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const override {
        vec3 scatter_direction = rec.normal + random_in_unit_sphere().normalized();
        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo;
        return true;
    }
};

class metal : public material {
public:
    vec3 albedo;
    double fuzz;
    metal(const vec3& a, double f) : albedo(a), fuzz(f < 1 ? f : 1) {}
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(r_in.dir.normalized(), rec.normal);
        scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere()*fuzz);
        attenuation = albedo;
        return (dot(scattered.dir, rec.normal) > 0);
    }
};

class dielectric : public material {
public:
    double ir; // index of refraction
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const override {
        attenuation = vec3(1.0,1.0,1.0);
        double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        vec3 unit_dir = r_in.dir.normalized();
        double cos_theta = fmin(dot(-unit_dir, rec.normal),1.0);
        double sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_dir, rec.normal);
        else
            direction = refract(unit_dir, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction);
        return true;
    }
private:
    static double reflectance(double cosine, double ref_idx) {
        // Schlick approximation
        auto r0 = (1-ref_idx)/(1+ref_idx);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine),5);
    }
};

// Ray color
vec3 ray_color(const ray& r, const hittable_list& world, int depth) {
    if (depth <= 0) return vec3(0,0,0);

    hit_record rec;
    if (world.hit(r, 0.001, infinity, rec)) {
        ray scattered;
        vec3 attenuation;
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth-1);
        return vec3(0,0,0);
    }

    vec3 unit_direction = r.dir.normalized();
    auto t = 0.5*(unit_direction.y + 1.0);
    return (1.0-t)*vec3(0.5, 0.7, 1.0) + t*vec3(0.1, 0.3, 0.6);
}

int main() {
    // Image
    const auto aspect_ratio = 1.0;
    const int image_width = 600;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 100;
    const int max_depth = 20;

    // World materials
    auto material_ground = make_shared<lambertian>(vec3(0.8, 0.8, 0.0));
    auto material_center = make_shared<lambertian>(vec3(0.1, 0.2, 0.5));
    auto material_left   = make_shared<dielectric>(1.5);
    auto material_right  = make_shared<metal>(vec3(0.8,0.6,0.2),0.0);

    // 36 frames animation
    for(int f=0; f<36; f++){
        double angle = f * 10.0 * pi / 180.0;
        hittable_list world;
        // Ground
        world.add(sphere(vec3(0,-1000,0), 1000, material_ground));
        // Center moving
        world.add(sphere(vec3(sin(angle)*0.5,0.5,-1.0), 0.5, material_center));
        // Left glass
        world.add(sphere(vec3(-1.0,0.5,-1.0),0.5,material_left));
        world.add(sphere(vec3(-1.0,0.5,-1.0),-0.45,material_left)); // hollow glass
        // Right metal
        world.add(sphere(vec3(1.0,0.5,-1.0),0.5,material_right));

        std::ofstream out("frame_" + std::to_string(f) + ".ppm");
        if (!out) {
            std::cerr << "Could not open frame_" << f << ".ppm for writing\n";
            return 1;
        }
        out << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for(int j=image_height-1;j>=0;j--){
            for(int i=0;i<image_width;i++){
                vec3 pixel_color(0,0,0);
                for(int s=0;s<samples_per_pixel;s++){
                    auto u = (i + random_double()) / (image_width-1);
                    auto v = (j + random_double()) / (image_height-1);
                    ray r(vec3(0,1,3), vec3(u-0.5, v-0.5, -1.0) - vec3(0,0,0));
                    pixel_color += ray_color(r, world, max_depth);
                }
                auto scale = 1.0 / samples_per_pixel;
                auto r_col = sqrt(scale*pixel_color.x);
                auto g_col = sqrt(scale*pixel_color.y);
                auto b_col = sqrt(scale*pixel_color.z);
                out << static_cast<int>(256*clamp(r_col,0.0,0.999)) << ' '
                    << static_cast<int>(256*clamp(g_col,0.0,0.999)) << ' '
                    << static_cast<int>(256*clamp(b_col,0.0,0.999)) << '\n';
            }
        }
        if (!out) {
            std::cerr << "Error while writing frame_" << f << ".ppm\n";
            return 1;
        }
        out.close();
        std::cout << "Frame " << f << " done!\n";
    }
    return 0;
}
