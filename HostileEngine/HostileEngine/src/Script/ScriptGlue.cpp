#include "stdafx.h"
#include "ScriptGlue.h"
#include "Engine.h"
#include "flecs.h"
#include "Input.h"
#include "InputKeyCodes.h"
#include "ScriptEngine.h"
#include "TransformSys.h"
#include "CollisionData.h"
#include "Camera.h"
#include "CameraComponent.h"
#include "PhysicsProperties.h"
#include "Graphics/GraphicsTypes.h"
#include "Graphics/Resources/Material.h"


namespace Script
{
	using namespace Hostile;

	#define ADD_INTERNAL_CALL(FuncName) mono_add_internal_call("HostileEngine.InternalCalls::" #FuncName, FuncName)
	static std::unordered_map<MonoType*, std::function<void(flecs::entity)>> s_EntityAddComponentFuncs;
	static std::unordered_map<MonoType*, std::function<bool(flecs::entity)>> s_EntityHasComponentFuncs;

	template<typename ... Comp>
	struct ComponentGroup {};

	using AllComponents =
		ComponentGroup
		<
		Transform,
		CollisionEventData, 
		//CollisionData, 
		Hostile::Camera, Rigidbody, Material
		>;


	//adapter for c# pointers 
    struct Vec2
    {
        float x;
        float y;
    };

	struct Vec3
	{
		float x;
		float y;
		float z;
	};

	struct CollisionContactData
    {
		uint64_t entity1ID;  // flecs::entity
		uint64_t entity2ID;  // flecs::entity
		Vec3 collisionNormal; 
		Vec3 contactPoint1;
		Vec3 contactPoint2;
	};

	static void Debug_Log(MonoString* monoString)
	{
		char* to_print = mono_string_to_utf8(monoString);
		Log::Info(to_print);
		mono_free(to_print);
	}

	static void Entity_AddComponent(uint64_t id, MonoReflectionType* componentType)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());

		MonoType* monoComponentType = mono_reflection_type_get_type(componentType);
		assert(s_EntityAddComponentFuncs.find(monoComponentType)!=s_EntityAddComponentFuncs.end(), "This Component Type was not registered !!");
		s_EntityAddComponentFuncs.at(monoComponentType)(entity);
	}

	static bool Entity_HasComponent(uint64_t id, MonoReflectionType* componentType)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());

		MonoType* monoComponentType = mono_reflection_type_get_type(componentType);
		assert(s_EntityHasComponentFuncs.find(monoComponentType)!=s_EntityHasComponentFuncs.end(), "This Component Type was not registered !!");
		return s_EntityHasComponentFuncs.at(monoComponentType)(entity);
	}

	static void TransformComponent_GetPosition(uint64_t id, Vec3* toReturn)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		const Transform* transform = entity.get<Transform>();
		auto& pos = transform->position;
		toReturn->x = pos.x;
		toReturn->y = pos.y;
		toReturn->z = pos.z;
	}

	static void TransformComponent_SetPosition(uint64_t id, Vec3* toSet)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		Transform* transform = entity.get_mut<Transform>();
		transform->position.x = toSet->x;
		transform->position.y = toSet->y;
		transform->position.z = toSet->z;
	}

	static void TransformComponent_GetScale(uint64_t id, Vec3* toReturn)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		const Transform* transform = entity.get<Transform>();
		auto& scale = transform->scale;
		toReturn->x = scale.x;
		toReturn->y = scale.y;
		toReturn->z = scale.z;
	}

	static void TransformComponent_SetScale(uint64_t id, Vec3* toSet)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		Transform* transform = entity.get_mut<Transform>();
		transform->scale.x = toSet->x;
		transform->scale.y = toSet->y;
		transform->scale.z = toSet->z;
	}

	static void ContactDataComponent_HasCollisionData(uint64_t id, bool* has)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		*has = entity.has<CollisionData>();
	}

	static void ContactDataComponent_GetCollisionData(uint64_t id, CollisionContactData* toReturn)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		const CollisionData* collisionData = entity.get<CollisionData>();
		toReturn->entity1ID = collisionData->entity1.id();
		toReturn->entity2ID = collisionData->entity2.id();
		toReturn->collisionNormal = {collisionData->collisionNormal.x,collisionData->collisionNormal.y,collisionData->collisionNormal.z};
		toReturn->contactPoint1 = { collisionData->contactPoints.first.x,collisionData->contactPoints.first.y,collisionData->contactPoints.first.z };
		toReturn->contactPoint2 = { collisionData->contactPoints.second.x,collisionData->contactPoints.second.y,collisionData->contactPoints.second.z };
	}
	static void CollisionEventDataComponent_GetNumCollidingEntities(uint64_t id, int* toReturn) {
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		assert(entity.has<CollisionEventData>());

		const CollisionEventData* eData = entity.get<CollisionEventData>();

		*toReturn= static_cast<int>(eData->m_collidingEntities.size());
	}

	static void CollisionEventDataComponent_GetCollidingEntityID(uint64_t id, size_t index, size_t* collidingId) {
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(id);
		assert(entity.is_valid());
		assert(entity.has<CollisionEventData>());

		const CollisionEventData* eData=entity.get<CollisionEventData>();

		if (index < eData->m_collidingEntities.size()) {
			auto it = eData->m_collidingEntities.begin();
			std::advance(it, index);
			*collidingId= *it;
		}
	}


	static bool Input_IsPressed_Key(KeyCode key)
	{
		return Input::IsPressed(key);
	}

	static bool Input_IsPressed_Mouse(MouseCode mouse)
	{
		return Input::IsPressed(mouse);
	}

	static bool Input_IsTriggered_Key(KeyCode key)
	{
		return Input::IsTriggered(key);
	}

	static bool Input_IsTriggered_Mouse(MouseCode mouse)
	{
		return Input::IsTriggered(mouse);
	}

	static bool Input_IsRepeating_Key(KeyCode key)
	{
		return Input::IsRepeating(key);
	}

	static bool Input_IsReleased_Key(KeyCode key)
	{
		return Input::IsReleased(key);
	}

	static bool Input_IsReleased_Mouse(MouseCode mouse)
	{
		return Input::IsReleased(mouse);
	}

	#pragma region CameraScripting
	static void Camera_GetPosition(uint64_t _id, Vec3* _getter )
	{
		
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(_id);
		assert(entity.is_valid());
		const Hostile::Camera* cameraData = entity.get<Hostile::Camera>();
		_getter->x = cameraData->m_view_info.m_position.x;
		_getter->z = cameraData->m_view_info.m_position.z;
		_getter->y = cameraData->m_view_info.m_position.y;
	}

	static void Camera_SetPosition(uint64_t _id, Vec3* _set)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(_id);
		assert(entity.is_valid());
		Hostile::Transform* cameraData = entity.get_mut<Hostile::Transform>();
		cameraData->position.x  = _set->x;
		cameraData->position.y  = _set->y;
		cameraData->position.z  = _set->z;
	}

	static void Camera_ChangeCamera(uint64_t _cameraID)
	{
		const auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(_cameraID);
		assert(entity.is_valid());
		SceneCamera::ChangeCamera(entity.name().c_str());
		
		
	}

    void Camera_GetRight(uint64_t _id, Vec3* _right)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        const Hostile::Camera* camera_data = entity.get<Hostile::Camera>();
        _right->x = camera_data->m_view_info.m_right.x;
        _right->y = camera_data->m_view_info.m_right.y;
        _right->z = camera_data->m_view_info.m_right.z;
    }

    void Camera_GetUp(uint64_t _id, Vec3* _up)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        const Hostile::Camera* camera_data = entity.get<Hostile::Camera>();
        _up->x = camera_data->m_view_info.m_right.x;
        _up->y = camera_data->m_view_info.m_right.y;
        _up->z = camera_data->m_view_info.m_right.z;
    }

    void Camera_GetForward(uint64_t _id, Vec3* _up)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        const Hostile::Camera* camera_data = entity.get<Hostile::Camera>();
        _up->x = camera_data->m_view_info.m_right.x;
        _up->y = camera_data->m_view_info.m_right.y;
        _up->z = camera_data->m_view_info.m_right.z;
    }

    void Camera_Pitch(uint64_t _id, float* _degree)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        camera_data->m_view_info.m_forward = DirectX::SimpleMath::Vector3::TransformNormal(
            camera_data->m_view_info.m_forward,
            Matrix::CreateFromAxisAngle(camera_data->m_view_info.m_right, XMConvertToRadians(*_degree))
        );
    }

    void Camera_Yaw(uint64_t _id, float* _degree)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        camera_data->m_view_info.m_forward = DirectX::SimpleMath::Vector3::TransformNormal(
            camera_data->m_view_info.m_forward,
            Matrix::CreateFromAxisAngle(camera_data->m_view_info.m_up, XMConvertToRadians(*_degree))
        );
    }

    void Camera_MoveForward(uint64_t _id, float* _speed)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        Hostile::Transform* transform = entity.get_mut<Hostile::Transform>();
        transform->position = Vector3::Transform(
            transform->position,
            Matrix::CreateTranslation(camera_data->m_view_info.m_forward * *_speed)
        );
    }

    void Camera_MoveRight(uint64_t _id, float* _speed)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        Hostile::Transform* transform = entity.get_mut<Hostile::Transform>();
        transform->position = Vector3::Transform(
            transform->position,
            Matrix::CreateTranslation(camera_data->m_view_info.m_right * *_speed)
        );
    }

    void Camera_MoveUp(uint64_t _id, float* _speed)
    {
        const auto& world = IEngine::Get().GetWorld();
        auto entity = world.entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        Hostile::Transform* transform = entity.get_mut<Hostile::Transform>();
        transform->position = Vector3::Transform(
            transform->position,
            Matrix::CreateTranslation(camera_data->m_view_info.m_up * *_speed)
        );
    }

    void Camera_GetFarNear(uint64_t _id, Vec2* _far_near)
    {
        auto entity = IEngine::Get().GetWorld().entity(_id);
        assert(entity.is_valid());
        const Hostile::Camera* camera_data = entity.get<Hostile::Camera>();
        _far_near->x = camera_data->m_projection_info.m_far;
        _far_near->y = camera_data->m_projection_info.m_near;
    }

    void Camera_SetPerspective(uint64_t _id, float* _fovY, float* _aspectRatio, float* _near, float* _far)
    {
        auto entity = IEngine::Get().GetWorld().entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        camera_data->m_projection_matrix = XMMatrixPerspectiveFovRH(*_fovY, *_aspectRatio, *_near, *_far);
        camera_data->m_projection_info.m_fovY = *_fovY;
        camera_data->m_projection_info.m_aspectRatio = *_aspectRatio;
        camera_data->m_projection_info.m_near = *_near;
        camera_data->m_projection_info.m_far = *_far;
    }

    void Camera_LookAt(uint64_t _id, Vec3* _eyePos, Vec3* _focusPos, Vec3* _globalUp)
    {
        auto entity = IEngine::Get().GetWorld().entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        Hostile::Transform* transform = entity.get_mut<Hostile::Transform>();
        transform->position = Vector3{ _eyePos->x, _eyePos->y, _eyePos->z };
        camera_data->m_view_info.m_forward = Vector3{ _focusPos->x - _eyePos->x,
        _focusPos->y - _eyePos->y, _focusPos->z - _eyePos->z };
        camera_data->m_view_info.m_forward.Normalize();
        Vector3 global_up = { _globalUp->x, _globalUp->y, _globalUp->z };
        camera_data->m_view_info.m_right = global_up.Cross(camera_data->m_view_info.m_forward);
        camera_data->m_view_info.m_right.Normalize();
        camera_data->m_view_info.m_up = camera_data->m_view_info.m_forward.Cross(camera_data->m_view_info.m_right);
        camera_data->m_view_info.m_up.Normalize();

        camera_data->m_view_matrix = XMMatrixLookAtRH(camera_data->m_view_info.m_position,
            { _focusPos->x, _focusPos->y, _focusPos->z }, global_up);
    }

    void Camera_LookTo(uint64_t _id, Vec3* _eyePos, Vec3* _lookDirection, Vec3* _relativeUp)
    {
        auto entity = IEngine::Get().GetWorld().entity(_id);
        assert(entity.is_valid());
        Hostile::Camera* camera_data = entity.get_mut<Hostile::Camera>();
        Hostile::Transform* transform = entity.get_mut<Hostile::Transform>();
        transform->position = Vector3{ _eyePos->x, _eyePos->y, _eyePos->z };
        camera_data->m_view_info.m_forward = Vector3{ _lookDirection->x, _lookDirection->y, _lookDirection->z };
        camera_data->m_view_info.m_forward.Normalize();
        Vector3 global_up = { _relativeUp->x, _relativeUp->y, _relativeUp->z };
        camera_data->m_view_info.m_right = global_up.Cross(camera_data->m_view_info.m_forward);
        camera_data->m_view_info.m_right.Normalize();
        camera_data->m_view_info.m_up = camera_data->m_view_info.m_forward.Cross(camera_data->m_view_info.m_right);
        camera_data->m_view_info.m_up.Normalize();

        camera_data->m_view_matrix = XMMatrixLookToRH(camera_data->m_view_info.m_position,
            camera_data->m_view_info.m_forward, global_up);
    }

    //Matrix View() const;
    //Matrix Projection() const;

    //Matrix ViewProjection() const;
    //void SetDefaultID(int _id);
    //int GetDefaultID();
	#pragma endregion CameraScripting

	static void RigidbodyComponent_AddForce(uint64_t _id, Vec3* _force)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(_id);
		assert(entity.is_valid());

		Rigidbody* rigidbody = entity.get_mut<Rigidbody>();

		rigidbody->m_force.x += _force->x;
		rigidbody->m_force.y += _force->y;
		rigidbody->m_force.z += _force->z;
	}

	static void RigidbodyComponent_AddTorque(uint64_t _id, Vec3* _angularForce)
	{
		auto& world = IEngine::Get().GetWorld();
		auto entity = world.entity(_id);
		assert(entity.is_valid());

		Rigidbody* rigidbody = entity.get_mut<Rigidbody>();

		rigidbody->m_torque.x += _angularForce->x;
		rigidbody->m_torque.y += _angularForce->y;
		rigidbody->m_torque.z += _angularForce->z;
	}

#pragma region Renderer
    static void MaterialComponent_GetColor(uint64_t _id, Vec3* _color, MonoString* _mono_string)
    {
        char* name = mono_string_to_utf8(_mono_string);
        
        flecs::world& world = IEngine::Get().GetWorld();
        flecs::entity e = world.entity(_id);
        assert(e.is_valid());

        const Renderer* renderer = e.get<Renderer>();
        Vector3 value = renderer->m_material->MaterialBuffer()->GetValue<Vector3>(name);
        _color->x = value.x;
        _color->y = value.y;
        _color->z = value.z;

        mono_free(name);
    }

    static void MaterialComponent_SetColor(uint64_t _id, Vec3* _color, MonoString* _mono_string)
    {
        char* name = mono_string_to_utf8(_mono_string);

        flecs::world& world = IEngine::Get().GetWorld();
        flecs::entity e = world.entity(_id);
        assert(e.is_valid());

        const Renderer* renderer = e.get<Renderer>();
        Vector3 color = { _color->x, _color->y, _color->z };
        renderer->m_material->MaterialBuffer()->SetValue<Vector3>(name, color);

        mono_free(name);
    }
#pragma endregion Renderer
	void ScriptGlue::RegisterFunctions()
	{
		ADD_INTERNAL_CALL(Debug_Log);

		ADD_INTERNAL_CALL(Entity_AddComponent);
		ADD_INTERNAL_CALL(Entity_HasComponent);

		ADD_INTERNAL_CALL(TransformComponent_GetPosition);
		ADD_INTERNAL_CALL(TransformComponent_SetPosition);
		ADD_INTERNAL_CALL(TransformComponent_GetScale);
		ADD_INTERNAL_CALL(TransformComponent_SetScale);

		ADD_INTERNAL_CALL(ContactDataComponent_HasCollisionData);
		ADD_INTERNAL_CALL(ContactDataComponent_GetCollisionData);

		ADD_INTERNAL_CALL(CollisionEventDataComponent_GetCollidingEntityID);
		ADD_INTERNAL_CALL(CollisionEventDataComponent_GetNumCollidingEntities);

		ADD_INTERNAL_CALL(Input_IsPressed_Key);
		ADD_INTERNAL_CALL(Input_IsPressed_Mouse);

		ADD_INTERNAL_CALL(Input_IsTriggered_Key);
		ADD_INTERNAL_CALL(Input_IsTriggered_Mouse);

		ADD_INTERNAL_CALL(Input_IsRepeating_Key);

		ADD_INTERNAL_CALL(Input_IsReleased_Key);
		ADD_INTERNAL_CALL(Input_IsReleased_Mouse);
		ADD_INTERNAL_CALL(Camera_GetPosition);
		ADD_INTERNAL_CALL(Camera_SetPosition);
        
        ADD_INTERNAL_CALL(Camera_GetRight);
        ADD_INTERNAL_CALL(Camera_GetUp);
        ADD_INTERNAL_CALL(Camera_GetForward);
        ADD_INTERNAL_CALL(Camera_Pitch);
        ADD_INTERNAL_CALL(Camera_Yaw);
        ADD_INTERNAL_CALL(Camera_MoveForward);
        ADD_INTERNAL_CALL(Camera_MoveRight);
        ADD_INTERNAL_CALL(Camera_MoveUp);

        ADD_INTERNAL_CALL(Camera_GetFarNear);
        ADD_INTERNAL_CALL(Camera_LookAt);
        ADD_INTERNAL_CALL(Camera_LookTo);

		ADD_INTERNAL_CALL(RigidbodyComponent_AddForce);
		ADD_INTERNAL_CALL(RigidbodyComponent_AddTorque);

        ADD_INTERNAL_CALL(MaterialComponent_GetColor);
        ADD_INTERNAL_CALL(MaterialComponent_SetColor);
	}

	//helper
	template<typename ... Component>
	static void RegisterComponent()
	{
		([]()
			{
				std::string_view typeName{ typeid(Component).name() };

				//process of extracting out ComponentName
				//since it will look like "struct Namespace::ComponentName"
				size_t pos = typeName.find_last_of(':');
				if (pos == std::string::npos)//if it has no name space
				{
					//since it will look like "struct ComponentName"
					pos = typeName.find_last_of(' ');
				}

				std::string managedTypeName = std::string("HostileEngine.") + typeName.substr(pos + 1).data();

				MonoType* managedType = mono_reflection_type_from_name(managedTypeName.data(), ScriptEngine::GetCoreAssemblyImage());

				if (managedType == nullptr)
				{
					Log::Error("( Type : {} ) Is Not Existing in C#", managedTypeName);
					return;
				}

				s_EntityAddComponentFuncs[managedType] = [](flecs::entity entity) { entity.add<Component>(); };
				s_EntityHasComponentFuncs[managedType] = [](flecs::entity entity) { return entity.has<Component>(); };
			}(),
				//expand it with templates
				...);
	}

	template<typename ... Component>
	static void RegisterComponent(ComponentGroup<Component ...>)
	{
		RegisterComponent<Component ...>();
	}

	void ScriptGlue::RegisterComponents()
	{
		s_EntityAddComponentFuncs.clear();
		s_EntityHasComponentFuncs.clear();
		RegisterComponent(AllComponents{});
	}
}
