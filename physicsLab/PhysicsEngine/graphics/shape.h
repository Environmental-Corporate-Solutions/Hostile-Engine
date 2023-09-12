#pragma once

#include <vector>
#include "textureImage.h"
#include "DirectXTK/SimpleMath.h"

using DirectX::SimpleMath::Vector3;

namespace graphics
{
    struct Shape
    {
    //protected:
        std::vector<float> m_vertices;

        std::vector<unsigned int> m_polygonIndices;
        std::vector<unsigned int> m_frameIndices;

        unsigned int m_polygonVAO;

    //public:
        Shape() :m_polygonVAO{} {}
        virtual ~Shape() {}

        virtual void GenerateShapeVertices(float) = 0;

        void SetupPolygonVAO();
    };

    class Sphere : public Shape
    {
    public:
        static constexpr int SECTOR_CNT = 72;
        static constexpr int STACK_CNT = 24;

    public:
        Sphere();
        void GenerateShapeVertices(float)override final;
        void GenerateIndices();
    };
}
