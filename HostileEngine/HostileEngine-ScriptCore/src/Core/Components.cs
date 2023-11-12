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

    public class CollisionContactData : Component
    {
        public ulong Entity1ID { get; set; }//uint64_t
        public ulong Entity2ID { get; set; }//uint64_t

        public Vector3 CollisionNormal { get; set; }
        public Vector3 ContactPoint1 { get; set; }
        public Vector3 ContactPoint2 { get; set; }

        public bool HasCollisionData
        {
            get
            {
                return InternalCalls.ContactDataComponent_HasCollisionData(Entity.ID);
            }
        }

        public void LoadCollisionData()
        {
            InternalCalls.ContactDataComponent_GetCollisionData(Entity.ID, out CollisionContactData fetchedData);
            // fetch & update
            this.Entity1ID = fetchedData.Entity1ID;
            this.Entity2ID = fetchedData.Entity2ID;
            this.CollisionNormal = fetchedData.CollisionNormal;
            this.ContactPoint1 = fetchedData.ContactPoint1;
            this.ContactPoint2 = fetchedData.ContactPoint2;
        }
    }
}
