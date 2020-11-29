#include "Vector3D.h"
#include <math.h>

vector3d::vector3d()  //constructor
{
    x = 0;
    y = 0;
    z = 0;
}

vector3d::vector3d(float x1, float y1, float z1)  //construct with values.
{
    x = x1;
    y = y1;
    z = z1;
}

vector3d::vector3d(const vector3d& vec)
{
    x = vec.x;
    y = vec.y;
    z = vec.z;
}

//addition
vector3d vector3d ::operator+(const vector3d& vec)
{
    //Returns a new vector summing the values for each component with the 
    //corresponding component in the added vector
    return vector3d(x + vec.x, y + vec.y, z + vec.z);
}


vector3d& vector3d ::operator+=(const vector3d& vec)
{
    //Returns ‘this’ pointer (i.e. self-reference summing the values for 
    //each component with the corresponding component in the added vector

    x += vec.x;
    y += vec.y;
    z += vec.z;
    return *this;
}

//substraction//
vector3d vector3d ::operator-(const vector3d& vec)
{
    //similar to addition
    return vector3d(x - vec.x, y - vec.y, z - vec.z);
}
vector3d& vector3d::operator-=(const vector3d& vec)
{
    //similar to addition
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
    return *this;
}

//scalar multiplication
vector3d vector3d ::operator*(float value)
{
    //similar to subtraction
    return vector3d(x * value, y * value, z * value);
}
vector3d& vector3d::operator*=(float value)
{
    //similar to subtraction
    x *= value;
    y *= value;
    z *= value;
    return *this;
}

//scalar division
vector3d vector3d ::operator/(float value)
{
    assert(value != 0); //prevent divide by 0
    //similar to multiplication
    return vector3d(x / value, y / value, z / value);
}
vector3d& vector3d ::operator/=(float value)
{
    assert(value != 0);
    //similar to multiplication
    x /= value;
    y /= value;
    z /= value;
    return *this;
}
vector3d& vector3d::operator=(const vector3d& vec)
{
    //similar to addition   
    return vector3d(1, 1, 1);
}

//Dot product
float vector3d::dot_product(const vector3d& vec)
{
    //returns (x1*x2 + y1*y2 + x1*z2) where these are the terms from
    // each vector 
    return (x * vec.x + y * vec.y + x * vec.z);
}

//cross product
vector3d vector3d::cross_product(const vector3d& vec)
{
    //Calculate the terms (ni,nj,nk) using the dot product formula 
    //Then use to construct a vector using those terms and return

    // as an example using vec to represent second vector
    // the term ni in the output (new)vector is calculated as 

// float ni=y*vec.z-z*vec.y;

    return vector3d(
        y * vec.z - z * vec.y,
        z * vec.x - x * vec.z,
        x * vec.y - y * vec.x
    );
}

float vector3d::magnitude()
{
    //return square root of sum of the squared components
    return float( sqrt( (x * x + y * y + z + z) ) );
}
float vector3d::square()
{
    return x * x + y * y + z * z;
}
vector3d vector3d::normalization()
{
    float temp = magnitude();
    return vector3d(x / temp, y / temp, z / temp);
}
float vector3d::distance(const vector3d& vec)
{
    return (1);
}
float vector3d::show_X()
{
    return x;
}
float vector3d::show_Y()
{
    return y;
}
float vector3d::show_Z()
{
    return z;
}
void vector3d::disp()
{
    cout << x << " " << y << " " << z << endl;
}
