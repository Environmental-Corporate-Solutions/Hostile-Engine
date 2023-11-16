#include "stdafx.h"
#include "DirectPipeline.h"

#include <deque>
#include <map>

#include <directxtk12/CommonStates.h>
#include <directxtk12/EffectPipelineStateDescription.h>

#include "Geometry.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace Hostile
{
    static PipelineNodeResource ReadResource(nlohmann::json const& _data)
    {
        PipelineNodeResource resource{};
        resource.name = _data["Name"].get<std::string>();

        if (_data.contains("Dimensions"))
            resource.desc.dimensions = { _data["Dimensions"][0].get<float>(), _data["Dimensions"][1].get<float>() };

        if (_data.contains("Format"))
            resource.desc.format = PipelineNodeResource::FormatFromString(_data["Format"].get<std::string>());

        return resource;
    }

    void Hostile::PipelineNodeGraph::Read(std::string _name, std::string _path)
    {
        using namespace nlohmann;
        std::ifstream stream(_path + _name);

        json data = json::parse(stream);

        m_name = data["Name"];

        auto const& nodes = data["PipelineNodes"];
        for (auto const& pipelineNode : nodes)
        {
            PipelineNodePtr pNode = std::make_shared<PipelineNode>();

            pNode->name = pipelineNode["Name"].get<std::string>();
            std::string frequency = pipelineNode["Frequency"].get<std::string>();
            pNode->frequency = PipelineNode::FrequencyFromString(frequency);
            if (pNode->frequency == PipelineNode::Frequency::INVALID)
                Log::Error("PipelineNode " + pNode->name + " unknown Frequency: " + frequency);
            m_nodes.push_back(pNode);

            for (json const& input : pipelineNode["Inputs"])
            {
                //PipelineNodeResource resource = ReadResource(input);
                //m_resources.push_back(std::make_shared<PipelineNodeResource>(resource));
                PipelineNodeResourceHandle rh;
                std::string name = input["Name"].get<std::string>();
                rh.type = PipelineNodeResourceHandle::TypeFromString(input["Type"].get<std::string>());
                if (m_resources.find(name) != m_resources.end())
                {
                    rh.resource = m_resources[name];
                }
                pNode->inputs.push_back(rh);
            }

            for (auto const& output : pipelineNode["Outputs"])
            {
                if (m_resources.find(output["Name"].get<std::string>()) == m_resources.end())
                {
                    PipelineNodeResource resource = ReadResource(output);
                    resource.producer = m_nodes.back();
                    m_resources[resource.name] = std::make_shared<PipelineNodeResource>(resource);
                }
                PipelineNodeResourceHandle rh;
                rh.resource = m_resources[output["Name"].get<std::string>()];
                rh.type = PipelineNodeResourceHandle::TypeFromString(output["Type"].get<std::string>());
                pNode->outputs.push_back(rh);
            }
        }
    }

    void PipelineNodeGraph::Compile(GpuDevice& _device, PipelineMap& _pipelines)
    {
        for (auto& node : m_nodes)
        {
            for (auto& input : node->inputs)
            {
                if (PipelineNodeResourcePtr resource = input.resource.lock())
                {
                    //if (PipelineNodeResourcePtr r = FindNodeResource(resource->name))
                    {
                        //resource->outputHandle = r;
                        //resource->desc = r->desc;
                        //resource->producer = r->producer;
                        if (PipelineNodePtr n = resource->producer.lock())
                            n->edges.push_back(node);
                    }
                }
            }
        }

        std::unordered_map<std::string, uint8_t> visited;
        std::deque<PipelineNodeHandle> stack;
        std::vector<PipelineNodePtr> sortedNodes;
        for (auto const& node : m_nodes)
        {
            stack.push_back(node);
            visited.emplace(node->name, 0);
        }

        while (!stack.empty())
        {
            PipelineNodePtr nodeHandle = stack.back().lock();
            if (visited[nodeHandle->name] == 2)
            {
                stack.pop_back();
                continue;
            }

            if (visited[nodeHandle->name] == 1)
            {
                visited[nodeHandle->name] = 2;

                sortedNodes.push_back(nodeHandle);

                stack.pop_back();
                continue;
            }

            visited[nodeHandle->name] = 1;
            if (nodeHandle->edges.size() == 0)
                continue;

            for (auto& r : nodeHandle->edges)
            {
                if (!visited[r.lock()->name])
                    stack.push_back(r);
            }
        }

        m_nodes.clear();
        while (!sortedNodes.empty())
        {
            m_nodes.push_back(sortedNodes.back());
            sortedNodes.pop_back();
        }
        std::vector<PipelineNodePtr> allocations;
        std::vector<PipelineNodePtr> deallocations;
        std::vector<PipelineNodeResourcePtr> free_list;
        for (auto& node : m_nodes)
        {
            for (auto& input : node->inputs)
            {
                input.resource.lock()->ref_count++;
            }
        }

        for (auto resource = m_resources.begin(); resource != m_resources.end();)
        {
            if (resource->second->ref_count == 0 && resource->second->name != "Final")
            {
                Log::Debug("Unused Resource: " + resource->second->name);
                resource = m_resources.erase(resource);
            }
            else
            {
                ++resource;
            }
        }

        UINT num_resources = 0;
        UINT total_size = 0;
        for (auto& node : m_nodes)
        {
            for (auto& output : node->outputs)
            {
                if (output.resource.expired())
                {
                    continue;
                }
                auto& out = output.resource.lock();
                if (out->producer.lock() == node)
                {
                    if (!free_list.empty())
                    {
                        PipelineNodeResourcePtr& r = free_list.front();
                        auto& d = r->desc;
                        auto& d2 = output.resource.lock()->desc;
                        if (d2.dimensions.x <= d.dimensions.x
                            && d2.dimensions.y <= d.dimensions.y
                            && d2.format == d.format)
                        {
                            out->alias = r->name;
                            out->desc.offset = r->desc.offset;
                            Log::Debug("Allocating: " + out->name
                                + " in same spot as " + r->name);
                        }
                        else
                        {
                            num_resources++;
                            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                            if (out->desc.format == DXGI_FORMAT_D32_FLOAT || out->desc.format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
                                || out->desc.format == DXGI_FORMAT_D24_UNORM_S8_UINT || out->desc.format == DXGI_FORMAT_D16_UNORM)
                                flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                            D3D12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                                out->desc.format, out->desc.dimensions.x, out->desc.dimensions.y,
                                1, 1, 1, 0, flags);
                            D3D12_RESOURCE_ALLOCATION_INFO info = _device->GetResourceAllocationInfo(0, 1, &resource_desc);
                            total_size += info.SizeInBytes;
                            out->desc.size = info.SizeInBytes;
                            Log::Debug("Allocating: " + out->name);
                        }
                    }
                    else
                    {
                        num_resources++;
                        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                        if (out->desc.format == DXGI_FORMAT_D32_FLOAT || out->desc.format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
                            || out->desc.format == DXGI_FORMAT_D24_UNORM_S8_UINT || out->desc.format == DXGI_FORMAT_D16_UNORM)
                            flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

                        D3D12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                            out->desc.format, out->desc.dimensions.x, out->desc.dimensions.y,
                            1, 1, 1, 0, flags);
                        D3D12_RESOURCE_ALLOCATION_INFO info = _device->GetResourceAllocationInfo(0, 1, &resource_desc);
                        out->desc.size = info.SizeInBytes;
                        total_size += info.SizeInBytes;
                        Log::Debug("Allocating: " + out->name);
                    }
                }
            }

            for (auto& input : node->inputs)
            {
                input.resource.lock()->ref_count--;
                if (input.resource.lock()->ref_count == 0)
                {
                    free_list.push_back(input.resource.lock());
                }
            }
        }

        D3D12_HEAP_DESC heap_desc = CD3DX12_HEAP_DESC(
            total_size,
            D3D12_HEAP_TYPE_DEFAULT,
            0UI64,
            D3D12_HEAP_FLAG_NONE
        );
        ThrowIfFailed(_device->CreateHeap(&heap_desc, IID_PPV_ARGS(&m_heap)));
        for (auto& [name, resource] : m_resources)
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            if (resource->desc.format == DXGI_FORMAT_D32_FLOAT || resource->desc.format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT
                || resource->desc.format == DXGI_FORMAT_D24_UNORM_S8_UINT || resource->desc.format == DXGI_FORMAT_D16_UNORM)
                flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(
                resource->desc.format, resource->desc.dimensions.x, resource->desc.dimensions.y,
                1, 1, 1, 0, flags);
            RenderTarget::RenderTargetCreateInfo create_info{};
            create_info.dimensions    = resource->desc.dimensions;
            create_info.format        = resource->desc.format;
            create_info.heap          = m_heap;
            create_info.placed        = true;
            create_info.offset        = resource->desc.offset;
            create_info.resource_desc = resource_desc;
            RenderTargetPtr render_target = RenderTarget::Create(_device, create_info);
            resource->resource = render_target;
        }
    }

    PipelineNodeResourcePtr PipelineNodeGraph::FindNodeResource(std::string _name)
    {
        if (m_resources.find(_name) != m_resources.end())
            return m_resources[_name];

        return nullptr;
    }

    PipelineNode::Frequency PipelineNode::FrequencyFromString(std::string& _str)
    {
        if (_str == "PER_OBJECT")
            return Frequency::PER_OBJECT;
        else if (_str == "PER_LIGHT")
            return Frequency::PER_LIGHT;
        else if (_str == "PER_FRAME")
            return Frequency::PER_FRAME;

        return Frequency::INVALID;
    }
    PipelineNodeResourceHandle::Type PipelineNodeResourceHandle::TypeFromString(std::string const& _str)
    {
        if (_str == "RENDER_TARGET")
            return Type::RENDER_TARGET;
        else if (_str == "REFERENCE")
            return Type::REFERENCE;
        else if (_str == "SHADER_RESOURCE")
            return Type::SHADER_RESOURCE;

        return Type::INVALID;
    }
    DXGI_FORMAT PipelineNodeResource::FormatFromString(std::string const& _str)
    {
        if (_str == "D32_FLOAT")
            return DXGI_FORMAT_D32_FLOAT;
        else if (_str == "D24_UNORM_S8_UINT")
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        else if (_str == "R8G8B8A8_UNORM")
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        else if (_str == "R32_FLOAT")
            return DXGI_FORMAT_R32_FLOAT;

        return DXGI_FORMAT_UNKNOWN;
    }
}