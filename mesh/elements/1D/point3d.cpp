#include "point3d.h"

using namespace mesh;

void Point3D::normalize()
{
	double l = 1.0 / sqrt(x * x + y * y + z * z);
	x *= l;
	y *= l;
	z *= l;
}

double Point3D::length() const
{
	return sqrt(x * x + y * y + z * z);
}

const Point3D Point3D::operator-() const
{
	return Point3D(-x, -y, -z);
}

void Point3D::flip()
{
	x = -x;
	y = -y;
	z = -z;
}

const Point3D Point3D::operator-(const Point3D &p) const
{
	return Point3D(x - p.x, y - p.y, z - p.z);
}

const Point3D Point3D::operator+(const Point3D &p) const
{
	return Point3D(x + p.x, y + p.y, z + p.z);
}

const Point3D Point3D::operator*(double scalar) const
{
	return Point3D(x * scalar, y * scalar, z * scalar);
}

const Point3D Point3D::operator/(double scalar) const
{
	double xscalar = 1. / scalar;
	return Point3D(x * xscalar, y * xscalar, z * xscalar);
}

double Point3D::scalar_product_with(const Point3D & p)
{
	return x * p.x + y * p.y + z * p.z;
}

Point3D & Point3D::operator*=(double scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
}

Point3D & Point3D::operator+=(const Point3D &p)
{
	x += p.x;
	y += p.y;
	z += p.z;
	return *this;
}

Point3D & Point3D::operator-=(const Point3D &p)
{
	x -= p.x;
	y -= p.y;
	z -= p.z;
	return *this;
}

Point3D & Point3D::operator=(const Point3D &p)
{
	x = p.x;
	y = p.y;
	z = p.z;
	return *this;
}

Point3D & Point3D::operator/=(double scalar)
{
	x /= scalar;
	y /= scalar;
	z /= scalar;
	return *this;
}
