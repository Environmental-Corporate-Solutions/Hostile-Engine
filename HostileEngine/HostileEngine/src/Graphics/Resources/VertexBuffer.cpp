#include "stdafx.h"
#include "VertexBuffer.h"
#include "../Geometry.h"
#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/BufferHelpers.h>

namespace Hostile
{
    VertexBufferPtr VertexBuffer::Create(
        GpuDevice& _device, const std::string& _name)
    {
        VertexBufferPtr vertex_buffer
            = std::make_shared<VertexBuffer>(_device, _name);

        vertex_buffer->Init();

        return vertex_buffer;
    }

    D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVBV()
    {
        return m_vbv;
    }

    D3D12_INDEX_BUFFER_VIEW VertexBuffer::GetIBV()
    {
        return m_ibv;
    }

    UINT VertexBuffer::Count()
    {
        return m_index_count;
    }

    void VertexBuffer::Init()
    {
        VertexCollection vertices;
        IndexCollection indices;
        if (m_name == "Cube")
        {
            ComputeBox(vertices, indices, XMFLOAT3(1, 1, 1), true, false);
        }
        else if (m_name == "Sphere")
        {
            ComputeSphere(vertices, indices, 1.0f, 16, true, false);
        }
        else if (m_name == "Dodecahedron")
        {
            ComputeDodecahedron(vertices, indices, 1, true);
        }
        else if (m_name == "Cylinder")
        {
            ComputeCylinder(vertices, indices, 1, 1, 8, true);
        }
        else if (m_name == "Square")
        {
            vertices.push_back(
                { Vector3{-1, -1, 0}, Vector3{0, 0, 0}, Vector2{0, 1} });
            vertices.push_back(
                { Vector3{-1,  1, 0}, Vector3{0, 0, 0}, Vector2{0, 0} });
            vertices.push_back(
                { Vector3{ 1,  1, 0}, Vector3{0, 0, 0}, Vector2{1, 0} });
            vertices.push_back(
                { Vector3{ 1, -1, 0}, Vector3{0, 0, 0}, Vector2{1, 1} });
            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);
            indices.push_back(0);
            indices.push_back(2);
            indices.push_back(3);
        }

        ResourceUploadBatch upload_batch(m_device.Device().Get());
        upload_batch.Begin();

        ThrowIfFailed(
            CreateStaticBuffer(
                m_device.Device().Get(),
                upload_batch,
                vertices.data(),
                vertices.size(),
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                &m_vertex_buffer
            ));
        m_vbv.BufferLocation = m_vertex_buffer->GetGPUVirtualAddress();
        m_vbv.SizeInBytes 
            = static_cast<UINT>(vertices.size() * sizeof(PrimitiveVertex));
        m_vbv.StrideInBytes = sizeof(PrimitiveVertex);

        ThrowIfFailed(CreateStaticBuffer(
            m_device.Device().Get(),
            upload_batch,
            indices.data(),
            indices.size(),
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            &m_index_buffer
        ));
        upload_batch.End(m_device.Queue().Get()).wait();
        m_ibv.BufferLocation = m_index_buffer->GetGPUVirtualAddress();
        m_ibv.Format = DXGI_FORMAT_R16_UINT;
        m_ibv.SizeInBytes
            = static_cast<UINT>(indices.size() * sizeof(uint16_t));
        m_index_count = static_cast<UINT>(indices.size());
    }
}