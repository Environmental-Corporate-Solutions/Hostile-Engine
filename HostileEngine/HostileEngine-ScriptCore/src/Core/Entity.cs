using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HostileEngine
{
    public class Entity
    {
        public readonly UInt64 ID;
        protected Entity()
        {
            ID = 0;
        }

        protected Entity(UInt64 _ID)
        {
            ID = _ID;
        }
        public T AddComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
                return GetComponent<T>();

            Type componentType = typeof(T);
            InternalCalls.Entity_AddComponent(ID, componentType);
            T component = new T() { Entity = this };
            return component;
        }
        public bool HasComponent<T>() where T : Component, new()
        {
            Type componentType = typeof(T);
            return InternalCalls.Entity_HasComponent(ID, componentType);
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;

            T component = new T() { Entity = this };
            return component;
        }

        public Vector3 Position
        {
            get
            {
                InternalCalls.TransformComponent_GetPosition(ID, out Vector3 position);
                return position;
            }
            set
            {
                InternalCalls.TransformComponent_SetPosition(ID, in value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.TransformComponent_GetScale(ID, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.TransformComponent_SetScale(ID, in value);
            }
        }

        protected virtual void OnCreate()
        {
            
        }
        protected virtual void OnUpdate()
        {

        }
    }
}
