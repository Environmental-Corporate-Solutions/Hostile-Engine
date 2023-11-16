#pragma once
#include "../ResourceLoader.h"
#include "../GraphicsTypes.h"
#include "../GpuDevice.h"
#include "Pipeline.h"

namespace Hostile
{
    class Material;
    using MaterialPtr = std::shared_ptr<Material>;
    class Material : public IGraphicsResource
    {
    public:
        static MaterialPtr Create(
            GpuDevice& _device, const std::string& _name);

        void SetPipeline(PipelinePtr _pipeline);
        PipelinePtr GetPipeline();
        void Bind(CommandList& _cmd);

        void RenderImGui();

        MaterialBufferPtr& MaterialBuffer();

        Material(GpuDevice& _device, const std::string& _name)
            : IGraphicsResource(_device, _name) {}

        const std::string& Path();
    private:
        void Init(const nlohmann::json& _data);

        std::string m_path = "";
        std::unordered_map<std::string, TexturePtr> m_textures;
        std::unordered_map<std::string, MaterialTexture> m_material_textures;
        MaterialBufferPtr m_material_buffer;

        PipelinePtr m_pipeline;
    };
}
