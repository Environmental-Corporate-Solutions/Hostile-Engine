#pragma once
#include "../GraphicsTypes.h"
#include "../GraphicsHelpers.h"
#include "../GpuDevice.h"
#include "../ResourceLoader.h"
#include "MaterialBuffer.h"

#include <vector>

namespace Hostile
{
    class Pipeline;
    using PipelinePtr = std::shared_ptr<Pipeline>;
    class Pipeline : public IGraphicsResource
    {
    public:
        enum class InputLayout
        {
            Primitive,
            Skinned,
            Frame,
            Count
        };

        enum class Buffer
        {
            Scene,
            Light,
            Material,
            Object
        };


        Pipeline(GpuDevice& _device, const std::string& _name);
        const InputLayout& GetInputLayout() const;
        const std::vector<Buffer>& Buffers() const;
        //MaterialInputMap& MaterialInputs();
        //const size_t MaterialInputsSize() const;

        const MaterialBufferPtr& MaterialBuffer() const;
        const std::vector<MaterialTexture>& Textures() const;
        struct DrawBatch
        {
            MaterialPtr material = nullptr;
            VertexBufferPtr mesh = nullptr;
            UINT32 stencil = 0;
            std::vector<ShaderObject> instances{};
        };

        void AddInstance(DrawCall& _draw_call);
        void Draw(
            CommandList& _cmd, 
            GraphicsResource& _constants, 
            GraphicsResource& _lights
        );

        void DrawInstanced(
            CommandList& _cmd,
            VertexBufferPtr& _mesh,
            GraphicsResource& _constants,
            D3D12_GPU_DESCRIPTOR_HANDLE& _lights,
            UINT _count
        );

        static constexpr TypeID type_id = 1;
        static TypeID TypeID() { return type_id; }
        static PipelinePtr Create(GpuDevice& _gpu, std::string _name);
    private:
        void Init(const nlohmann::json& _data);
    private:
        InputLayout m_input_layout = InputLayout::Primitive;
        std::vector<Buffer> m_buffers{};
        //MaterialInputMap m_material_inputs{};
        //size_t m_material_inputs_size = 0;

        ComPtr<ID3D12PipelineState> m_pipeline = nullptr;
        ComPtr<ID3D12RootSignature> m_root_signature = nullptr;

        std::vector<DrawBatch> m_draws{ 0 };

        MaterialBufferPtr m_material_buffer;
        std::vector<MaterialTexture> m_textures;
        friend class Graphics;
    };
}