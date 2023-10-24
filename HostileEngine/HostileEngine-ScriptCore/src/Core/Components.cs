namespace HostileEngine
{
    /// <summary>
    /// Since the Script Engine will set the Entity ID for you
    /// All Component Classes Should Inherit This
    /// </summary>
    public abstract class Component
    {
        public Entity Entity { get; internal set; }

    }
    public class Transform : Component
    {
        public Vector3 Position
        {
            get
            {
                InternalCalls.TransformComponent_GetPosition(Entity.ID, out Vector3 position);
                return position;
            }
            set
            {
                InternalCalls.TransformComponent_SetPosition(Entity.ID, in value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.TransformComponent_SetScale(Entity.ID, in value);
            }
        }
    }
}
