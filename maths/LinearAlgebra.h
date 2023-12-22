#pragma once

#include "../Common.h"
#include "../Types.h"

namespace LA
{
	class Vector2
	{
	public:
		Vector2(float32 xx, float32 yy);
		Vector2();
		Vector2(const Vector2& v);
		Vector2& operator=(const Vector2& v);

		float32 operator[](int32 i) const;
		float32& operator[](int32 i);

		Vector2& operator+=(const Vector2& v);
		Vector2& operator-=(const Vector2& v);
		Vector2& operator*=(float32 f);
		Vector2& operator/=(float32 f);

		bool operator==(const Vector2& v);
		bool operator!=(const Vector2& v);

		float32 x, y;

	private:
		const static uint32 NUMBER_ELEMENTS = 2u;
	};

	Vector2 operator-(const Vector2& v);
	Vector2 operator+(const Vector2& v, const Vector2& w);
	Vector2 operator-(const Vector2& v, const Vector2& w);
	Vector2 operator*(const Vector2& v, float32 f);
	Vector2 operator/(const Vector2& v, float32 f);
	float32 Dot(const Vector2& v, const Vector2& w);
	float32 LengthSquared(const Vector2& v);
	float32 Length(const Vector2& v);
	Vector2	Normalize(const Vector2& v);
	float32 MinComponent(const Vector2& v);
	float32 MaxComponent(const Vector2& v);

	class Vector3
	{
	public:
		Vector3(float32 xx, float32 yy, float32 zz);
		Vector3();
		Vector3(const Vector3& v);
		Vector3& operator=(const Vector3& v);

		float32 operator[](int32 i) const;
		float32& operator[](int32 i);

		Vector3& operator+=(const Vector3& v);
		Vector3& operator-=(const Vector3& v);
		Vector3& operator*=(float32 f);
		Vector3& operator/=(float32 f);

		bool operator==(const Vector3& v);
		bool operator!=(const Vector3& v);

		float32 x, y, z;
	private:
		const static uint32 NUMBER_ELEMENTS = 3u;
	};

	Vector3 operator-(const Vector3& v);
	Vector3 operator+(const Vector3& v, const Vector3& w);
	Vector3 operator-(const Vector3& v, const Vector3& w);
	Vector3 operator*(const Vector3& v, float32 f);
	Vector3 operator/(const Vector3& v, float32 f);
	float32 Dot(const Vector3& v, const Vector3& w);
	Vector3 Cross(const Vector3& v, const Vector3& w);
	float32 LengthSquared(const Vector3& v);
	float32 Length(const Vector3& v);
	Vector3	Normalize(const Vector3& v);
	float32 MinComponent(const Vector3& v);
	float32 MaxComponent(const Vector3& v);


	class Matrix
	{
	public:

	private:

	};
}