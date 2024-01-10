#include "stdafx.h"
#include "Texture.h"

#include <directxtk12/ResourceUploadBatch.h>
#include <directxtk12/WICTextureLoader.h>
#include <filesystem>
namespace Hostile
{
    TexturePtr Texture::Create(GpuDevice& _device, const std::string& _name)
    {
        TexturePtr texture = std::make_shared<Texture>(_device, 
            std::filesystem::path(_name).filename().string());
        texture->Init(_name);
        return texture;
    }

    void Texture::SetBindPoint(UINT _bind_point)
    {
        m_bind_point = _bind_point;
    }

    void Texture::Bind(CommandList& _cmd)
    {
        _cmd->SetGraphicsRootDescriptorTable(
            m_bind_point,
            m_gpu_handle
        );
    }

    void Texture::Init(const std::string& _path)
    {
        ResourceUploadBatch upload_batch(m_device.Device().Get());
        upload_batch.Begin();

        ThrowIfFailed(
            CreateWICTextureFromFile(
                m_device.Device().Get(),
                upload_batch,
                ConvertToWideString(_path).c_str(),
                &m_texture
            )
        );

        m_resource_heap_index = m_device.ResourceHeap().Allocate();
        m_gpu_handle 
            = m_device.ResourceHeap().GetGpuHandle(m_resource_heap_index);
        m_device->CreateShaderResourceView(
            m_texture.Get(),
            nullptr,
            m_device.ResourceHeap().GetCpuHandle(m_resource_heap_index)
        );
        upload_batch.End(m_device.Queue().Get()).wait();
    }
}
