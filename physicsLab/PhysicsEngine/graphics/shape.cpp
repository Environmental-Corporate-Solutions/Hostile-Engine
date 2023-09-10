#include <opengl/glad/glad.h>
#include "shape.h"
#include <cmath>
#include "math/mathConstants.h"

using namespace graphics;

void Shape::SetupPolygonVAO()
{
    //1.polygon
    //VAO
    glGenVertexArrays(1, &m_polygonVAO);
    glBindVertexArray(m_polygonVAO);

    //VBO
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * m_vertices.size(),
        &m_vertices[0],
        GL_STATIC_DRAW
    );

    //(0) position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //(1) texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //EBO
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(unsigned int) * m_polygonIndices.size(),
        &m_polygonIndices[0],
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
}

Sphere::Sphere()
{
    GenerateShapeVertices(1.0f);
    GenerateIndices();
    SetupPolygonVAO();
}

//http://www.songho.ca/opengl/gl_sphere.html

void Sphere::GenerateShapeVertices(float radius)
{
    float x, y, z, xy, s,t;
    float sectorStep = 2 * math::PI / SECTOR_CNT;
    float stackStep = math::PI / STACK_CNT;
    float sectorAngle, stackAngle;

    m_vertices.clear();
    for(int i = 0; i <= STACK_CNT; ++i)
    {
        stackAngle = math::PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and collisionNormal, but different tex coords
        for(int j = 0; j <= SECTOR_CNT; ++j)
        {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            m_vertices.push_back(x);
            m_vertices.push_back(y);
            m_vertices.push_back(z);

            // vertex tex coord (s, t) range between [0, 1]
            s = (float)j / SECTOR_CNT;
            t = (float)i / STACK_CNT;
            m_vertices.push_back(s);
            m_vertices.push_back(t);
        }
    }
}

void Sphere::GenerateIndices()
{
    int k1, k2;
    for(int i = 0; i < STACK_CNT; ++i)
    {
        k1 = i * (SECTOR_CNT + 1);     // beginning of current stack
        k2 = k1 + SECTOR_CNT + 1;      // beginning of next stack

        for(int j = 0; j < SECTOR_CNT; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if(i != 0)
            {
                m_polygonIndices.push_back(k1);
                m_polygonIndices.push_back(k2);
                m_polygonIndices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if(i != (STACK_CNT-1))
            {
                m_polygonIndices.push_back(k1 + 1);
                m_polygonIndices.push_back(k2);
                m_polygonIndices.push_back(k2 + 1);
            }

            // store indices for lines
            // vertical lines for all stacks, k1 => k2
        }
        m_frameIndices.push_back(k1);
        m_frameIndices.push_back(k2);
    }
}
