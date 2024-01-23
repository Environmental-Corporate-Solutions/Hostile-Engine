#include "stdafx.h"
#include "CameraComponent.h"

#include "Camera.h"
#include "Engine.h"
#include "TransformSys.h"

namespace Hostile
{
  ADD_SYSTEM(CameraSys)


    void CameraSys::OnCreate(flecs::world& _world)
  {

    _world.system("CameraEditorSys")
      .kind(flecs::PreUpdate)
      .kind<Editor>()
      .rate(.3f)
      .iter([this](flecs::iter const& _info) {OnEdit(_info); });

    _world.system<CameraData, Transform>("CameraSys")
      .term_at(2).optional()
      .kind(flecs::PreUpdate)
      .rate(.5f)
      .iter(OnUpdate);


    REGISTER_TO_SERIALIZER(CameraData, this);
    REGISTER_TO_DESERIALIZER(CameraData, this);
    IEngine::Get().GetGUI().RegisterComponent(
      "CameraData",
      std::bind(&CameraSys::GuiDisplay,
        this, std::placeholders::_1, std::placeholders::_2),
      [this](flecs::entity& _entity) { _entity.add<CameraData>(); });

  }


  void CameraSys::OnUpdate(_In_ flecs::iter _info, _In_ CameraData* _pCamera, _In_ Transform* _pTransform)
  {

    if (_info.is_set(2))
    {
      for (auto it : _info)
      {
        CameraData& cam = _pCamera[it];
        [[maybe_unused]] const Transform& _transform = _pTransform[it];
        UpdatePosition(cam, _transform.position);

      }

    }

    for (auto it : _info)
    {
      //_info.world().entity( ).parent();
      CameraData& cam = _pCamera[it];


      UpdateView(cam);
      if (cam.active == true)
      {
        if (cam.m_projection_info.changed)
          UpdateProjection(cam);

        Camera::ChangeCamera(&cam);

      }
    }



  }


  void CameraSys::OnEdit(flecs::iter _info)
  {
    auto ent = _info.world().entity("Scene Camera");

    CameraData* cam = ent.get_mut<Hostile::CameraData>();
    [[maybe_unused]] Transform* _transform = ent.get_mut<Transform>();
    //UpdatePosition(*cam, _transform->position);

    UpdateProjection(*cam);
    UpdateView(*cam);
    Camera::ChangeCamera(ent.id());


  }

  void   CameraSys::Write(_In_ const flecs::entity& _entity, std::vector<nlohmann::json>& _components, const std::string& type)
  {

    if (type == "ObjectName")
    {
      return;
    }

    const CameraData* _data = _entity.get<CameraData>();
    auto j = nlohmann::json::object();
    j["Type"] = "CameraData";
    j["Active"] = _data->active;
    j["Offset"] = WriteVec3(_data->_offset);
    j["Fov"] = _data->m_projection_info.m_fovY;
    j["Near"] = _data->m_projection_info.m_near;
    j["Far"] = _data->m_projection_info.m_far;
    j["Position"] = WriteVec3(_entity.get<Transform>()->position);
    j["Up_vector"] = WriteVec3(_data->m_view_info.m_up);
    j["Right_vector"] = WriteVec3(_data->m_view_info.m_right);
    j["Forward_vector"] = WriteVec3(_data->m_view_info.m_forward);
    _components.push_back(j);

  }

  void CameraSys::Read(flecs::entity& _object, nlohmann::json& _data, const std::string& type)
  {

    if (type == "CameraData")
    {
      CameraData _camdata;
      _camdata.active = _data["Active"];
      _camdata._offset = ReadVec3(_data["Offset"]);
      _camdata.m_projection_info.m_fovY = _data["Fov"];
      _camdata.m_projection_info.m_near = _data["Near"];
      _camdata.m_projection_info.m_far = _data["Far"];
      _camdata.m_view_info.m_position = ReadVec3(_data["Position"]);
      _camdata.m_view_info.m_up = ReadVec3(_data["Up_vector"]);
      _camdata.m_view_info.m_right = ReadVec3(_data["Right_vector"]);
      _camdata.m_view_info.m_forward = ReadVec3(_data["Forward_vector"]);

      _object.set<CameraData>(_camdata);


    }
  }

  void CameraSys::GuiDisplay(flecs::entity& _entity, const std::string& type)
  {
    bool is_open = ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
      ImGui::OpenPopup("Camera Popup");
    }
    if (ImGui::BeginPopup("Camera Popup"))
    {
      if (ImGui::Button("Remove Component"))
      {
        _entity.remove<CameraData>();
        ImGui::CloseCurrentPopup();

      }
      ImGui::EndPopup();
    }
    if (is_open)
    {


      CameraData* camera = _entity.get_mut<CameraData>();


      if (_entity.has<Transform>())
      {
        const Transform* transform = _entity.get<Transform>();
        Transform _local_transform = *transform;
        ImGui::DragFloat3("Position", &_local_transform.position.x, 0.1f);
        UpdatePosition(*camera, _local_transform.position);
        _entity.set<Transform>(_local_transform);

      }
      else
      {
        ImGui::DragFloat3("Position", &camera->m_view_info.m_position.x, 0.1f);
      }

      ImGui::DragFloat3("Offset", &camera->_offset.x, 0.1f);
      UpdateOffset(*camera, camera->_offset);
      ImGui::Checkbox("Active", &camera->active);

      _entity.set<CameraData>(*camera);
      ImGui::TreePop();
    }
  }

  Vector3 CameraSys::GetPosition(_In_ const  CameraData& _cam)
  {
    return _cam.m_view_info.m_position;
  }

  void CameraSys::UpdatePosition(CameraData& _cam, const Vector3 position)
  {
    _cam.m_view_info.m_position = position + _cam._offset;

    _cam.m_view_info.changed = true;
  }

  void CameraSys::UpdateOffset(_In_ CameraData& _camera_component, _In_  Vector3 _offset)
  {
    _camera_component.m_view_info.m_position = GetPosition(_camera_component) + _offset;
  }

  bool CameraSys::SetFOV(flecs::id _id, float _fov)
  {
    m_camera = GetCamera(_id);
    m_camera->m_projection_info.m_fovY = _fov;
    m_camera->m_projection_info.changed = true;

    return true;
  }

  std::shared_ptr<CameraData> CameraSys::GetCamera(flecs::id _id)
  {
    const flecs::world& _local_world = IEngine::Get().GetWorld();
    CameraData cam = *_local_world.get_alive(_id).get_mut<CameraData>();
    return std::make_shared<CameraData>(cam);
  }

  static CameraData& GetCamera(_In_ int _id)
  {
    flecs::world& _local_world = IEngine::Get().GetWorld();
    CameraData _cam = *_local_world.get_alive(_id).get_mut<CameraData>();
    return _cam;
  }

  void  CameraSys::UpdateView(_In_ CameraData& _data)
  {

    Vector3 globalUp = { 0, 1, 0 };
    if (_data.m_view_info.m_forward != Vector3{ 0, 1, 0 }&& _data.m_view_info.m_forward != Vector3{ 0, -1, 0 })
    {
      _data.m_view_info.m_right = globalUp.Cross(_data.m_view_info.m_forward);
      _data.m_view_info.m_right.Normalize();

      _data.m_view_info.m_up = _data.m_view_info.m_forward.Cross(_data.m_view_info.m_right);
      _data.m_view_info.m_up.Normalize();
    }
    else
    {
      _data.m_view_info.m_up = _data.m_view_info.m_forward.Cross({ 1, 0, 0 });
      _data.m_view_info.m_up.Normalize();

      _data.m_view_info.m_right = _data.m_view_info.m_up.Cross(_data.m_view_info.m_forward);
      _data.m_view_info.m_right.Normalize();
    }

    _data.m_view_matrix = XMMatrixLookToRH(_data.m_view_info.m_position, _data.m_view_info.m_forward, _data.m_view_info.m_up);
  }


  void CameraSys::UpdateProjection(CameraData& _camera_data)
  {
    _camera_data.m_projection_matrix = XMMatrixPerspectiveFovRH(_camera_data.m_projection_info.m_fovY, _camera_data.m_projection_info.m_aspectRatio, _camera_data.m_projection_info.m_near, _camera_data.m_projection_info.m_far);
  }
  void CameraSys::SetCameraPosition(uint64_t _id, Vector3 _position)
  {
    auto& world = IEngine::Get().GetWorld();
    auto _cam = world.get_alive(_id).get_mut<CameraData>();
    _cam->m_view_info.m_position = _position;
    _cam->m_view_info.changed = true;
  }

  void CameraSys::UpdateFOV(uint32_t _new_fov)
  {

  }

}