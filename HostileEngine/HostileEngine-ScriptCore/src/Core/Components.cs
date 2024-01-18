using System.Linq;

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


    public class Camera : Component
    {
	    public Vector3 Position
	    {
		    get
		    {
                InternalCalls.Camera_GetPosition(Entity.ID, out Vector3 position);
                return position;
		    }
		    set
		    {
				InternalCalls.Camera_SetPosition(Entity.ID, in value);
			}
		}
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

    public class CollisionEventData : Component
    {
        // Properties for individual event data
        public ulong Entity1ID { get; private set; }
        public ulong Entity2ID { get; private set; }
        public int Category { get; private set; }
        public int EventType { get; private set; }

        // Array to hold multiple events
        public CollisionEventData[] CollisionEvents { get; private set; }

        public void LoadCollisionEventData()
        {
            InternalCalls.CollisionEventDataComponent_GetCollisionEventData(Entity.ID, out CollisionEventData[] fetchedEvents, out int eventCount);
            CollisionEvents = fetchedEvents.Take(eventCount).ToArray();

            // Optionally, update properties for the first event or handle events differently
            if (CollisionEvents.Length > 0)
            {
                var firstEvent = CollisionEvents[0];
                this.Entity1ID = firstEvent.Entity1ID;
                this.Entity2ID = firstEvent.Entity2ID;
                this.Category = firstEvent.Category;
                this.EventType = firstEvent.EventType;
            }
        }
    }


    public class Rigidbody : Component
    {
        public void AddForce(in Vector3 force)
        {
            InternalCalls.RigidbodyComponent_AddForce(Entity.ID, force);
        }

        public void AddTorque(in Vector3 angularForce)
        {
            InternalCalls.RigidbodyComponent_AddTorque(Entity.ID, angularForce);
        }
    }


    public class Material : Component
    {
        public Vector3 GetColor(in string name)
        {
            Vector3 color = new Vector3();
            InternalCalls.MaterialComponent_GetColor(Entity.ID, out color, name);
            return color;
        }

        public void SetColor(in Vector3 color, in string name)
        {
            InternalCalls.MaterialComponent_SetColor(Entity.ID, color, name);
        }
    }
}
