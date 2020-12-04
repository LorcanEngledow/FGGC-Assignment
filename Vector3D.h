#pragma once

#include<iostream>
#include<math.h>
#include<assert.h>
using namespace std;

class vector3d
{
public:
    float x, y, z;

    //Constructors
    vector3d();  //constructor	
    vector3d(float x1, float y1, float z1 = 0);  //construct with values.
    vector3d(const vector3d& vec); //copy constructor
    vector3d(float xI, float yI, float zI, float xD, float yD, float zD);  //construct with two points
    

        //Arithemetic Operators – note use of overloading
    vector3d operator+(const vector3d& vec);   //addition
    vector3d& operator+=(const vector3d& vec);//assign new result to vector
    vector3d operator-(const vector3d& vec); //substraction
    vector3d& operator-=(const vector3d& vec);//assign new result to vector
    vector3d operator*(float value);    //multiplication
    vector3d& operator*=(float value);  //assign new result to vector.
    vector3d operator/(float value);    //division
    vector3d& operator/=(float value);  //assign new result to vector
    vector3d& operator=(const vector3d& vec);

    //Vector operations
    float dot_product(const vector3d& vec); //scalar dot_product
    vector3d cross_product(const vector3d& vec); //cross_product
    vector3d normalization();   //normalized vector

    //Scalar operations
    float square(); //gives square of the vector
    float distance(const vector3d& vec); //distance between two vectors
    float magnitude();  //magnitude of the vector


    //Display operations 
    float show_X(); //return x
    float show_Y(); //return y
    float show_Z(); //return z
    void disp();    //display value of vectors
};
