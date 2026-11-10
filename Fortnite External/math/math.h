#pragma once
#include <atomic>
#include <mutex>
#include <dwmapi.h>
#include <xmmintrin.h>
#include <immintrin.h>
#include <array>
#include <vector>
#include <cstdlib>
#include <random>
#include <direct.h>
#include <fstream>
#include <string>
#include <sstream>
#include <numbers>

#include "../Menu/imgui/imgui.h"
#include "../Menu/imgui/imgui_impl_dx11.h"
#include "../Menu/imgui/imgui_impl_win32.h"
#include "../Driver/communication.h"

inline int Width = GetSystemMetrics(SM_CXSCREEN);
inline int Height = GetSystemMetrics(SM_CYSCREEN);
extern float ScreenCenterX;
extern float ScreenCenterY;
extern int CenterX;
extern int CenterY;

class Vector2 {
public:
	double x;
	double y;

	float Length() const {
		return std::sqrt(x * x + y * y);
	}

	Vector2 Normalized() const {
		float len = Length();
		if (len == 0.0f)
			return Vector2(0.0f, 0.0f);
		return Vector2(x / len, y / len);
	}

	inline double Distance(Vector2 v) {
		return double(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0)));
	}

	auto ToImVec2() -> mvec {
		return { (float)this->x, (float)this->y };
	}

	bool is_valid() const { return this->x && this->y; }

	Vector2 operator * (double number) const {
		return Vector2(x * number, y * number);
	}

	Vector2 operator+(const Vector2& other) const {
		return Vector2(x + other.x, y + other.y);
	}

	Vector2 operator-(const Vector2& other) const {
		return Vector2(x - other.x, y - other.y);
	}

	Vector2 operator/(float scalar) const {
		return Vector2(x / scalar, y / scalar);
	}

	Vector2 operator * (float scalar) const {
		return Vector2(x * scalar, y * scalar);
	}
};

class Vector3 {
public:
	Vector3() : x(0.f), y(0.f), z(0.f) {}

	Vector3(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
	~Vector3() {}

	Vector3& operator=(const Vector3& other) {
		if (this != &other) {
			x = other.x;
			y = other.y;
			z = other.z;
		}
		return *this;
	}

	double x;
	double y;
	double z;

	inline double Dot(Vector3 v) {
		return x * v.x + y * v.y + z * v.z;
	}

	inline double Distance(Vector3 v) const {
		return double(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
	}

	inline double Length() const {
		return sqrt(x * x + y * y + z * z);
	}

	Vector3 normalized() const {
		double len = Length();
		if (len == 0.0) {
			return Vector3(0.0, 0.0, 0.0);
		}
		return Vector3(x / len, y / len, z / len);
	}

	Vector3 operator+(Vector3 v) const {
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3 operator-(Vector3 v) const {
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3 operator*(double flNum) const { return Vector3(x * flNum, y * flNum, z * flNum); }
};

struct FPlane : Vector3 {
	double W;

	FPlane() : W(0) {}
	FPlane(double W) : W(W) {}
};

struct Matrix4x4 {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
};

class FMatrix {
public:
	double m[4][4];
	FPlane XPlane, YPlane, ZPlane, WPlane;

	FMatrix() : XPlane(), YPlane(), ZPlane(), WPlane() {}
	FMatrix(FPlane XPlane, FPlane YPlane, FPlane ZPlane, FPlane WPlane)
		: XPlane(XPlane), YPlane(YPlane), ZPlane(ZPlane), WPlane(WPlane) {
	}

	Matrix4x4 ToMatrix4x4() const {
		Matrix4x4 Result;
		Result._11 = (float)XPlane.x; Result._12 = (float)XPlane.y; Result._13 = (float)XPlane.z; Result._14 = (float)XPlane.W;
		Result._21 = (float)YPlane.x; Result._22 = (float)YPlane.y; Result._23 = (float)YPlane.z; Result._24 = (float)YPlane.W;
		Result._31 = (float)ZPlane.x; Result._32 = (float)ZPlane.y; Result._33 = (float)ZPlane.z; Result._34 = (float)ZPlane.W;
		Result._41 = (float)WPlane.x; Result._42 = (float)WPlane.y; Result._43 = (float)WPlane.z; Result._44 = (float)WPlane.W;
		return Result;
	}
};

struct FQuat {
	double x;
	double y;
	double z;
	double w;
};

struct FTransform final {
public:
	FQuat Rotation;
	Vector3 Translation;
	uint8_t Pad_1C[0x8];
	Vector3 Scale3D;
	uint8_t Pad_2C[0x8];

public:
	PFORCEINLINE FMatrix ToMatrixWithScale() const {
		const Vector3 Scale(
			(Scale3D.x == 0.0) ? 1.0 : Scale3D.x,
			(Scale3D.y == 0.0) ? 1.0 : Scale3D.y,
			(Scale3D.z == 0.0) ? 1.0 : Scale3D.z);

		const double x2 = Rotation.x + Rotation.x;
		const double y2 = Rotation.y + Rotation.y;
		const double z2 = Rotation.z + Rotation.z;
		const double xx2 = Rotation.x * x2;
		const double yy2 = Rotation.y * y2;
		const double zz2 = Rotation.z * z2;
		const double yz2 = Rotation.y * z2;
		const double wx2 = Rotation.w * x2;
		const double xy2 = Rotation.x * y2;
		const double wz2 = Rotation.w * z2;
		const double xz2 = Rotation.x * z2;
		const double wy2 = Rotation.w * y2;

		FMatrix Matrix = FMatrix();

		Matrix.WPlane.x = Translation.x;
		Matrix.WPlane.y = Translation.y;
		Matrix.WPlane.z = Translation.z;

		Matrix.XPlane.x = (1.0 - (yy2 + zz2)) * Scale.x;
		Matrix.YPlane.y = (1.0 - (xx2 + zz2)) * Scale.y;
		Matrix.ZPlane.z = (1.0 - (xx2 + yy2)) * Scale.z;

		Matrix.ZPlane.y = (yz2 - wx2) * Scale.z;
		Matrix.YPlane.z = (yz2 + wx2) * Scale.y;

		Matrix.YPlane.x = (xy2 - wz2) * Scale.y;
		Matrix.XPlane.y = (xy2 + wz2) * Scale.x;

		Matrix.ZPlane.x = (xz2 + wy2) * Scale.z;
		Matrix.XPlane.z = (xz2 - wy2) * Scale.x;

		Matrix.XPlane.W = 0.0f;
		Matrix.YPlane.W = 0.0f;
		Matrix.ZPlane.W = 0.0f;
		Matrix.WPlane.W = 1.0f;

		return Matrix;
	}
};

template<class type>
class tarray {
public:
	tarray() : data(nullptr), count(std::int32_t()), maxx(std::int32_t()) {}
	tarray(type* data, std::int32_t count, std::int32_t maxx) : data(data), count(count), maxx(maxx) {}

	const bool is_valid() const noexcept {
		return !(this->data == nullptr);
	}

	const std::int32_t size() const noexcept {
		return this->count;
	}

	type& operator[](std::int32_t index) noexcept {
		return this->data[index];
	}

	const type& operator[](std::int32_t index) const noexcept {
		return this->data[index];
	}

	bool is_valid_index(std::int32_t index) const noexcept {
		return index < this->size();
	}

	type* data;
	std::int32_t count;
	std::int32_t maxx;
};

inline Matrix4x4 MatrixMultiplication(Matrix4x4 pM1, Matrix4x4 pM2) {
	Matrix4x4 pOut;
	pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
	pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
	pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
	pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
	pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
	pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
	pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
	pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
	pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
	pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
	pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
	pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
	pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
	pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
	pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
	pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

	return pOut;
}

inline Matrix4x4 Matrix(Vector3 rot) {
	float radPitch = (rot.x * float(std::numbers::pi) / 180.f);
	float radYaw = (rot.y * float(std::numbers::pi) / 180.f);
	float radRoll = (rot.z * float(std::numbers::pi) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	Matrix4x4 matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = 0;
	matrix.m[3][1] = 0;
	matrix.m[3][2] = 0;
	matrix.m[3][3] = 1.f;

	return matrix;
}

namespace math {
	template < typename _value_t >
		requires std::is_arithmetic_v< _value_t >
	using enough_float_t = std::conditional_t< sizeof(_value_t) <= sizeof(float), float, double >;

	template < typename _ret_t >
		requires std::is_floating_point_v< _ret_t >
	inline constexpr auto k_pi = static_cast<_ret_t>(std::numbers::pi);

	template < typename _ret_t >
		requires std::is_floating_point_v< _ret_t >
	inline constexpr auto k_pi2 = static_cast<_ret_t>(k_pi< double > *2.0);

	template < typename _ret_t >
		requires std::is_floating_point_v< _ret_t >
	inline constexpr auto k_rad_pi = static_cast<_ret_t>(180.0 / k_pi< double >);

	template < typename _ret_t >
		requires std::is_floating_point_v< _ret_t >
	inline constexpr auto k_deg_pi = static_cast<_ret_t>(k_pi< double > / 180.0);

	template < typename _value_t >
		requires std::is_arithmetic_v< _value_t >
	constexpr auto to_deg(const _value_t rad) {
		using ret_t = enough_float_t< _value_t >;

		return static_cast<ret_t>(rad * k_rad_pi< ret_t >);
	}

	template < typename _value_t >
		requires std::is_arithmetic_v< _value_t >
	constexpr auto to_rad(const _value_t deg) {
		using ret_t = enough_float_t< _value_t >;

		return static_cast<ret_t>(deg * k_deg_pi< ret_t >);
	}

	double degree_to_radian(double degree) {
		return static_cast<double>(degree * (std::numbers::pi / 180));
	}

	template<typename t>
	struct vec4 {
		t x, y, z, w;
	};

	namespace m {

		struct smat4_f64 {
			__m256d col0, col1, col2, col3;

			vec4<double> transform(const Vector3& v) const noexcept {
				__m256d vx = _mm256_set1_pd(v.x);
				__m256d vy = _mm256_set1_pd(v.y);
				__m256d vz = _mm256_set1_pd(v.z);
				__m256d r = _mm256_mul_pd(vx, col0);
				r = _mm256_fmadd_pd(vy, col1, r);
				r = _mm256_fmadd_pd(vz, col2, r);
				r = _mm256_add_pd(r, col3);
				vec4<double> out;
				_mm256_storeu_pd(reinterpret_cast<double*>(&out), r);
				return out;
			}
		};

		template<typename t>
		struct alignas(sizeof(t) * 4) mat4 {
			t m[4][4]{};

			t* operator[](size_t i) { return m[i]; }
			const t* operator[](size_t i) const { return m[i]; }

			smat4_f64 to_simd() const noexcept {
				return {
					_mm256_loadu_pd(&m[0][0]),
					_mm256_loadu_pd(&m[1][0]),
					_mm256_loadu_pd(&m[2][0]),
					_mm256_loadu_pd(&m[3][0])
				};
			}
		};

	}

	using matrix4 = m::mat4<double>;
	using matrix4_simd = m::smat4_f64;

	static constexpr double rad_to_deg = 180.0 / std::numbers::pi;

}
