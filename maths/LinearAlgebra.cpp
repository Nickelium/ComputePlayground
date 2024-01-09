#include "LinearAlgebra.h"

namespace LA
{
	bool Vector2::operator!=(const Vector2& v)
	{
		return !(*this == v);
	}

	bool Vector2::operator==(const Vector2& v)
	{
		return x == v.x && y == v.y;
	}

	Vector2& Vector2::operator/=(float32 f)
	{
		ASSERT(f != 0.0f);
		return *this *= (1.0f / f);
	}

	Vector2& Vector2::operator*=(float32 f)
	{
		x *= f;
		y *= f;
		return *this;
	}

	Vector2& Vector2::operator-=(const Vector2& v)
	{
		return *this += -v;
	}

	Vector2& Vector2::operator+=(const Vector2& v)
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}

	Vector2& Vector2::operator=(const Vector2& v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}

	Vector2::Vector2(const Vector2& v) : Vector2(v.x, v.y)
	{

	}

	Vector2::Vector2(float32 xx, float32 yy) : x(xx), y(yy)
	{

	}

	Vector2::Vector2() : Vector2(0.0f, 0.0f)
	{

	}

	float32& Vector2::operator[](int32 i)
	{
		ASSERT(i >= 0 && i < NUMBER_ELEMENTS);
		if (i == 0)
			return x;
		else
			return y;
	}

	float32 Vector2::operator[](int32 i) const
	{
		ASSERT(i >= 0 && i < NUMBER_ELEMENTS);
		if (i == 0)
			return x;
		else
			return y;
	}

	bool Vector3::operator!=(const Vector3& v)
	{
		return !(*this == v);
	}

	bool Vector3::operator==(const Vector3& v)
	{
		return x == v.x && y == v.y;
	}

	Vector3& Vector3::operator/=(float32 f)
	{
		ASSERT(f != 0.0f);
		return *this *= (1.0f / f);
	}

	Vector3& Vector3::operator*=(float32 f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	Vector3& Vector3::operator-=(const Vector3& v)
	{
		return *this += -v;
	}

	Vector3& Vector3::operator+=(const Vector3& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	Vector3& Vector3::operator=(const Vector3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	Vector3::Vector3(const Vector3& v) : Vector3(v.x, v.y, v.z)
	{

	}

	Vector3::Vector3(float32 xx, float32 yy, float32 zz) : x(xx), y(yy), z(zz)
	{

	}

	Vector3::Vector3() : Vector3(0.0f, 0.0f, 0.0f)
	{

	}

	float32& Vector3::operator[](int32 i)
	{
		ASSERT(i >= 0 && i < NUMBER_ELEMENTS);
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else
			return z;
	}

	float32 Vector3::operator[](int32 i) const
	{
		ASSERT(i >= 0 && i < NUMBER_ELEMENTS);
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else
			return z;
	}

	Vector3 operator-(const Vector3& v)
	{
		return Vector3(-v.x, -v.y, -v.z);
	}

	Vector2 operator*(const Vector2& v, float32 f)
	{
		return Vector2(v.x * f, v.y * f);
	}

	Vector2 operator/(const Vector2& v, float32 f)
	{
		ASSERT(f != 0.0f);
		return v * (1.0f / f);
	}

	Vector2 operator+(const Vector2& v, const Vector2& w)
	{
		return Vector2(v.x + w.x, v.y + w.y);
	}

	Vector2 operator-(const Vector2& v, const Vector2& w)
	{
		return v + (-w);
	}

	Vector3 operator*(const Vector3& v, float32 f)
	{
		return Vector3(v.x * f, v.y * f, v.z * f);
	}

	Vector3 operator/(const Vector3& v, float32 f)
	{
		ASSERT(f != 0.0f);
		return v * (1.0f / f);
	}

	float32 Dot(const Vector2& v, const Vector2& w)
	{
		return v.x * w.x + v.y * w.y;
	}

	Vector3 Cross(const Vector3& v, const Vector3& w)
	{
		Vector3 a = v * float32(5.0f);
		return Vector3
		(
			(v.y * w.z) - (v.z * w.y),
			(v.z * w.x) - (v.x * w.z),
			(v.x * w.y) - (v.y * w.x)
		);
	}

	float32 LengthSquared(const Vector2& v)
	{
		return v.x * v.x + v.y * v.y;
	}

	float32 Length(const Vector2& v)
	{
		return std::sqrtf(LengthSquared(v));
	}

	Vector2 Normalize(const Vector2& v)
	{
		ASSERT(Length(v) != 0.0f);
		return v / Length(v);
	}

	float32 LengthSquared(const Vector3& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	float32 Length(const Vector3& v)
	{
		return std::sqrtf(LengthSquared(v));
	}

	Vector3 Normalize(const Vector3& v)
	{
		ASSERT(Length(v) != 0.0f);
		return v / Length(v);
	}

	float32 Dot(const Vector3& v, const Vector3& w)
	{
		return v.x * w.x + v.y * w.y + v.z * w.z;
	}

	Vector3 operator+(const Vector3& v, const Vector3& w)
	{
		return Vector3(v.x + w.x, v.y + w.y, v.z + w.z);
	}

	Vector2 operator-(const Vector2& v)
	{
		return Vector2(-v.x, -v.y);
	}

	Vector3 operator-(const Vector3& v, const Vector3& w)
	{
		return v + (-w);
	}
}
