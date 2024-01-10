#pragma once
#include "../ResourceLoader.h"
#include "../GraphicsTypes.h"
#include "../GpuDevice.h"

namespace Hostile
{
    class MaterialBuffer;
    using MaterialBufferPtr = std::unique_ptr<MaterialBuffer>;
    class MaterialBuffer
    {
    public:
        enum class Type
        {
            Float,
            Float2,
            Float3,
            Float4,
            Invalid
        };

        template<typename T>
        T GetValue(const std::string& _name)
        {
            if (m_inputs.find(_name)
                == m_inputs.end())
                throw std::invalid_argument("Value Name does not exist");
            Input input = m_inputs[_name];
            if (!std::holds_alternative<T>(input.value))
                throw std::invalid_argument("Value is not of type specified");
            return std::get<T>(input.value);
        }

        template<typename T>
        void SetValue(const std::string& _name, const T& _value)
        {
            if (m_inputs.find(_name)
                == m_inputs.end())
                throw std::invalid_argument("Value Name does not exist");
            Input& input = m_inputs[_name];
            if (!std::holds_alternative<T>(input.value))
                throw std::invalid_argument("Value is not of type specified");
            input.value = _value;
            memcpy(
                static_cast<UINT8*>(m_resource.Memory()) + input.offset,
                &input.value,
                type_sizes[static_cast<UINT>(input.type)]
            );
        }

        struct Input
        {
            std::string name;
            Type type;
            UINT offset;
            std::variant<float, Vector2, Vector3, Vector4> value;
        };

        using InputMap = std::unordered_map<std::string, Input>;
        InputMap& GetValues();
        void Bind(CommandList& _cmd);
        MaterialBufferPtr Clone();

        static Type TypeFromString(const std::string& _str);

        static constexpr std::array type_sizes = {
            sizeof(float), sizeof(Vector2), sizeof(Vector3),
            sizeof(Vector4)
        };

        static MaterialBufferPtr Create(
            const std::string& _name,
            UINT _bind_point,
            std::vector<Input>& _inputs
        );

        MaterialBuffer(
            const std::string& _name,
            UINT _bind_point
        ) : m_name(_name), m_bind_point(_bind_point) {}

    private:
        void Init(std::vector<Input>& _inputs);

        std::string m_name;
        UINT m_bind_point;
        UINT m_size = 0;

        InputMap m_inputs{};

        DirectX::GraphicsResource m_resource;
    };
}