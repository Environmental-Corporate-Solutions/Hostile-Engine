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

        public Vector3 Right
        {
            get
            {
                InternalCalls.Camera_GetRight(Entity.ID, out Vector3 right);
                return right;
            }
        }

        public Vector3 Up
        {
            get
            {
                InternalCalls.Camera_GetUp(Entity.ID, out Vector3 up);
                return up;
            }
        }

        public Vector3 Forward
        {
            get
            {
                InternalCalls.Camera_GetForward(Entity.ID, out Vector3 forward);
                return forward;
            }
        }

        public Vector2 FarNear
        {
            get
            {
                InternalCalls.Camera_GetFarNear(Entity.ID, out Vector2 farNear);
                return farNear;
            }
        }

        public void Pitch(float _degree)
        {
            InternalCalls.Camera_Pitch(Entity.ID, _degree);
        }

        public void Yaw(float _degree)
        {
            InternalCalls.Camera_Yaw(Entity.ID, _degree);
        }

        public void MoveForward(float _speed)
        {
            InternalCalls.Camera_MoveForward(Entity.ID, _speed);
        }

        public void MoveRight(float _speed)
        {
            InternalCalls.Camera_MoveRight(Entity.ID, _speed);
        }

        public void MoveUp(float _speed)
        {
            InternalCalls.Camera_MoveUp(Entity.ID, _speed);
        }

        public void LookAt(Vector3 _eyePos, Vector3 _focusPos, Vector3 _globalUp)
        {
            InternalCalls.Camera_LookAt(Entity.ID, _eyePos, _focusPos, _globalUp);
        }

        public void LookTo(Vector3 _eyePos, Vector3 _lookDirection, Vector3 _relativeUp)
        {
            InternalCalls.Camera_LookTo(Entity.ID, _eyePos, _lookDirection, _relativeUp);
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
