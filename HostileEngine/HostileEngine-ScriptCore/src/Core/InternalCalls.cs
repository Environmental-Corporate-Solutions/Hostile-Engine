using System;
using System.Runtime.CompilerServices;

namespace HostileEngine
{
    internal static class InternalCalls
    {
        #region Debug

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Debug_Log(string str);
        #endregion


        #region CameraComponent

        [MethodImpl(MethodImplOptions.InternalCall)]
	    internal static extern void Camera_GetPosition(UInt64 id, out Vector3 returnParam);
        
	    [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Camera_SetPosition(UInt64 id, in Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Camera_ChangeCamera(UInt64 id);
        #endregion

        #region Entity

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Entity_AddComponent(UInt64 id, Type componentType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Entity_HasComponent(UInt64 id, Type componentType);

        #endregion

        #region TransformComponent

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformComponent_GetPosition(UInt64 id, out Vector3 returnParam);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformComponent_SetPosition(UInt64 id, in Vector3 toSet);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformComponent_GetScale(UInt64 id, out Vector3 returnParam);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void TransformComponent_SetScale(UInt64 id, in Vector3 toSet);

        #endregion

        #region CollisionContactDataComponent

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool ContactDataComponent_HasCollisionData(UInt64 id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ContactDataComponent_GetCollisionData(UInt64 id, out CollisionContactData returnParam);
        #endregion

        #region CollisionEventDataComponent

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void CollisionEventDataComponent_GetNumCollidingEntities(UInt64 id, out int numEntities);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong CollisionEventDataComponent_GetCollidingEntityID(UInt64 id, int index, out UInt64 collidingId);
        #endregion

        #region Rigidbody
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidbodyComponent_AddForce(UInt64 id, in Vector3 force);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidbodyComponent_AddTorque(UInt64 id, in Vector3 angularForce);
        #endregion

        #region Input

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsPressed_Key(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsPressed_Mouse(MouseCode mouseCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsTriggered_Key(KeyCode key);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsTriggered_Mouse(MouseCode mouse);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsRepeating_Key(KeyCode key);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsReleased_Key(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsReleased_Mouse(MouseCode mouseCode);

        #endregion

        #region MaterialComponent
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void MaterialComponent_GetColor(UInt64 _id,  out Vector3 _color, string _name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void MaterialComponent_SetColor(UInt64 _id, Vector3 _color, string _name);
        #endregion
    }
}
