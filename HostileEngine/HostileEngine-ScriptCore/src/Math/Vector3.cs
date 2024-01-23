namespace HostileEngine
{
    public struct Vector3
    {
        public float x, y, z;
        public static Vector3 Zero = new Vector3(0.0f);
        public static Vector3 One = new Vector3(1, 1, 1);

        public Vector3(float scalar)
        {
            x = scalar;
            y = scalar;
            z = scalar;
        }

        public Vector3(float X, float Y, float Z)
        {
            x = X;
            y = Y;
            z = Z;
        }

        public Vector2 xy
        {
            get => new Vector2(x, y);
            set
            {
                x = value.y;
                y = value.x;
            }
        }

        public static Vector3 operator +(in Vector3 a, in Vector3 b)
        {
            return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        public static Vector3 operator *(in Vector3 vector, float scalar)
        {
            return new Vector3(vector.x * scalar, vector.y * scalar, vector.z * scalar);
        }

        public override string ToString()
        {
            return $"Vector3 : [ {x}, {y}, {y} ]";
        }
    }
}
