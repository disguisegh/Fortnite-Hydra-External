#pragma once
#include <vector>
#include <mutex>
#include <optional>
#include <chrono>
#include "../../sdk-fname-offsets/Sdk/sdk.h"

struct FVF {
    float x, y, z;
    FVF() : x(0.f), y(0.f), z(0.f) {}
    FVF(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    bool is_zero() const { return x == 0.f && y == 0.f && z == 0.f; }
    float dot(const FVF& v) const { return x*v.x + y*v.y + z*v.z; }
    FVF cross(const FVF& v) const {
        return FVF(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
    }
    float length() const { return sqrtf(x*x + y*y + z*z); }
    FVF normalized() const {
        float len = length();
        if (len < 1e-8f) return FVF();
        return FVF(x/len, y/len, z/len);
    }
    FVF operator+(const FVF& v) const { return FVF(x+v.x, y+v.y, z+v.z); }
    FVF operator-(const FVF& v) const { return FVF(x-v.x, y-v.y, z-v.z); }
    FVF operator*(float s) const { return FVF(x*s, y*s, z*s); }
    FVF operator/(float s) const { return FVF(x/s, y/s, z/s); }
};

struct FMF {
    float m[4][4]{};

    FMF identity() const {
        FMF r;
        r.m[0][0] = 1.f; r.m[1][1] = 1.f; r.m[2][2] = 1.f; r.m[3][3] = 1.f;
        return r;
    }

    FMF operator*(const FMF& o) const {
        FMF r;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 4; k++)
                    r.m[i][j] += m[i][k] * o.m[k][j];
        return r;
    }

    Vector3 transform(const FVF& v) const {
        return Vector3(
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]
        );
    }
};

inline FMF TransformToFMF(const FTransform& t) {
    FMatrix dm = t.ToMatrixWithScale();
    FMF r;
    r.m[0][0] = (float)dm.XPlane.x; r.m[0][1] = (float)dm.YPlane.x;
    r.m[0][2] = (float)dm.ZPlane.x; r.m[0][3] = (float)dm.WPlane.x;
    r.m[1][0] = (float)dm.XPlane.y; r.m[1][1] = (float)dm.YPlane.y;
    r.m[1][2] = (float)dm.ZPlane.y; r.m[1][3] = (float)dm.WPlane.y;
    r.m[2][0] = (float)dm.XPlane.z; r.m[2][1] = (float)dm.YPlane.z;
    r.m[2][2] = (float)dm.ZPlane.z; r.m[2][3] = (float)dm.WPlane.z;
    r.m[3][0] = 0.f; r.m[3][1] = 0.f; r.m[3][2] = 0.f; r.m[3][3] = 1.f;
    return r;
}

struct FKConvexElem {
    char shape_elem_fields[0x28];
    tarray<FVF> vertex_data;
    tarray<int32_t> index_data;
    char pad_chaos[0xa8];
    FTransform elem_transform;
};

struct WorldTriangle {
    Vector3 v[3];
};

struct Geometry {
    uintptr_t actor;
    Vector3 location;
    std::vector<WorldTriangle> triangles;
};

struct Ray {
    Vector3 origin;
    Vector3 direction;
    Vector3 invDir;
    double distance;
};

class raycast_c {
public:
    void rebuild_cache(uintptr_t Gworld) {
        auto rebuildStart = std::chrono::steady_clock::now();
        std::vector<Geometry> newGeo;

        auto levels = read<tarray<uintptr_t>>(Gworld + Offsets::Levels);
        if (!levels.data || levels.count <= 0) return;

        for (int li = 0; li < (std::min)(levels.count, 64); li++) {
            uintptr_t level = read<uintptr_t>((uintptr_t)levels.data + li * sizeof(uintptr_t));
            if (!level) continue;

            uintptr_t cluster = read<uintptr_t>(level + Offsets::ActorCluster);
            if (!cluster) continue;
            auto actors = read<tarray<uintptr_t>>(cluster + Offsets::Actors);
            if (!actors.data || actors.count <= 0 || actors.count > 16384) continue;

            for (int ai = 0; ai < actors.count; ai++) {
                uintptr_t actor = read<uintptr_t>((uintptr_t)actors.data + ai * sizeof(uintptr_t));
                if (!actor) continue;

                uint8_t relevant_byte = read<uint8_t>(actor + Offsets::bAlwaysRelevant);
                if ((relevant_byte & 0xfe) == 0x70) continue;

                uintptr_t root = read<uintptr_t>(actor + Offsets::RootComponent);
                if (!root) continue;

                FTransform comp2world = read<FTransform>(root + Offsets::ComponentToWorld);
                Vector3 loc(comp2world.Translation.x, comp2world.Translation.y, comp2world.Translation.z);
                if (loc.x == 0.0 && loc.y == 0.0 && loc.z == 0.0) continue;

                uintptr_t staticMesh = read<uintptr_t>(actor + Offsets::StaticMesh);
                if (!staticMesh) continue;

                uintptr_t bodySetup = read<uintptr_t>(staticMesh + Offsets::BodySetup);
                if (!bodySetup) continue;

                auto convexElems = read<tarray<FKConvexElem>>(bodySetup + 0x30 + 0x30);
                if (!convexElems.data || convexElems.count <= 0) continue;

                std::vector<FKConvexElem> elems(convexElems.count);
                read_buffer((uintptr_t)convexElems.data, elems.data(),
                            convexElems.count * sizeof(FKConvexElem));

                auto geo = build_geometry(elems, comp2world, actor, loc);
                if (geo.has_value())
                    newGeo.push_back(std::move(*geo));
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_geometry = std::move(newGeo);
        }
    }

    uintptr_t line_trace(const Vector3& position) const {
        constexpr double epsilon = 1e-8;
        const Vector3 origin = camera.Location;
        Vector3 delta = position - origin;
        double distance = delta.Length();

        if (distance < epsilon) return 0;
        if (position.x == 0.0 && position.y == 0.0 && position.z == 0.0) return 0;

        Vector3 direction = delta * (1.0 / distance);
        Vector3 invDir(
            fabs(direction.x) > epsilon ? 1.0 / direction.x : epsilon,
            fabs(direction.y) > epsilon ? 1.0 / direction.y : epsilon,
            fabs(direction.z) > epsilon ? 1.0 / direction.z : epsilon
        );

        Ray ray{ origin, direction, invDir, distance };

        std::lock_guard<std::mutex> lock(m_mtx);
        for (const auto& geom : m_geometry) {
            for (const auto& tri : geom.triangles) {
                if (intersect_triangle(ray, tri))
                    return geom.actor;
            }
        }
        return 0;
    }

    void render_geometry() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto* draw = ImGui::GetForegroundDrawList();
        if (!draw) return;

        for (const auto& geom : m_geometry) {
            for (const auto& tri : geom.triangles) {
                Vector2 sa = ProjectWorldToScreen(tri.v[0]);
                Vector2 sb = ProjectWorldToScreen(tri.v[1]);
                Vector2 sc = ProjectWorldToScreen(tri.v[2]);

                if (sa.x == 0.0 && sa.y == 0.0 && sb.x == 0.0 && sb.y == 0.0 && sc.x == 0.0 && sc.y == 0.0)
                    continue;

                ImVec2 pts[4] = {
                    ImVec2((float)sa.x, (float)sa.y),
                    ImVec2((float)sb.x, (float)sb.y),
                    ImVec2((float)sc.x, (float)sc.y),
                    ImVec2((float)sa.x, (float)sa.y)
                };
                draw->AddPolyline(pts, 4, IM_COL32(255, 255, 255, 50), ImDrawFlags_None, 0.1f);
            }
        }
    }

private:
    std::optional<Geometry> build_geometry(
        const std::vector<FKConvexElem>& elems,
        const FTransform& comp2world,
        uintptr_t actor,
        const Vector3& location) const
    {
        size_t triCount = 0;
        for (auto& e : elems) {
            if (e.index_data.data && e.index_data.count > 0)
                triCount += e.index_data.count / 3;
        }
        if (triCount == 0) return std::nullopt;

        Geometry geo;
        geo.actor = actor;
        geo.location = location;
        geo.triangles.reserve(triCount);

        FMF worldMat = TransformToFMF(comp2world);

        for (auto& e : elems) {
            if (!e.vertex_data.data || !e.index_data.data) continue;
            if (e.vertex_data.count <= 0 || e.index_data.count <= 0) continue;

            std::vector<FVF> verts(e.vertex_data.count);
            read_buffer((uintptr_t)e.vertex_data.data, verts.data(),
                        e.vertex_data.count * sizeof(FVF));

            std::vector<int32_t> indices(e.index_data.count);
            read_buffer((uintptr_t)e.index_data.data, indices.data(),
                        e.index_data.count * sizeof(int32_t));

            FMF localMat = TransformToFMF(e.elem_transform);
            FMF composed = localMat * worldMat;

            for (int i = 0; i + 2 < (int)indices.size(); i += 3) {
                int i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];
                if (i0 >= (int)verts.size() || i1 >= (int)verts.size() || i2 >= (int)verts.size())
                    continue;
                if (verts[i0].is_zero() || verts[i1].is_zero() || verts[i2].is_zero())
                    continue;

                WorldTriangle tri;
                tri.v[0] = composed.transform(verts[i0]);
                tri.v[1] = composed.transform(verts[i1]);
                tri.v[2] = composed.transform(verts[i2]);
                geo.triangles.push_back(tri);
            }
        }

        if (geo.triangles.empty()) return std::nullopt;
        return geo;
    }

    static Vector3 cross_vec(const Vector3& a, const Vector3& b) {
        return Vector3(a.y * b.z - a.z * b.y,
                       a.z * b.x - a.x * b.z,
                       a.x * b.y - a.y * b.x);
    }

    bool intersect_triangle(const Ray& ray, const WorldTriangle& tri) const {
        constexpr double epsilon = 1e-8;
        Vector3 v0 = tri.v[0];
        Vector3 v1 = tri.v[1];
        Vector3 v2 = tri.v[2];

        Vector3 dir = ray.direction;
        Vector3 org = ray.origin;

        Vector3 edge1 = v1 - v0;
        Vector3 edge2 = v2 - v0;
        Vector3 rayCrossEdge2 = cross_vec(dir, edge2);
        double det = edge1.Dot(rayCrossEdge2);

        if (det > -epsilon && det < epsilon) return false;

        double invDet = 1.0 / det;
        Vector3 originToV0 = org - v0;
        double u = invDet * originToV0.Dot(rayCrossEdge2);
        if (u < 0.0 || u > 1.0) return false;

        Vector3 originCrossEdge1 = cross_vec(originToV0, edge1);
        double v = invDet * dir.Dot(originCrossEdge1);
        if (v < 0.0 || u + v > 1.0) return false;

        double t = invDet * edge2.Dot(originCrossEdge1);
        return t > epsilon && t < ray.distance;
    }

    mutable std::mutex m_mtx;
    std::vector<Geometry> m_geometry;
};

inline raycast_c* g_raycast = new raycast_c();
