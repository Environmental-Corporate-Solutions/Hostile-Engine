namespace HostileEngine
{
    public struct Vector2
    {
        public float x, y;

        public static Vector2 Zero = new Vector2(0.0f);
        public static Vector2 One = new Vector2(1, 1);
        public Vector2(float scalar)
        {
            x = scalar;
            y = scalar;
        }
        public Vector2(float X, float Y)
        {
            x = X;
            y = Y;
        }

        public static Vector2 operator +(in Vector2 a, in Vector2 b)
        {
            return new Vector2(a.x + b.x, a.y + b.y);
        }

        public static Vector2 operator *(Vector2 vector, float scalar)
        {
            return new Vector2(vector.x * scalar, vector.y * scalar);
        }

        public override string ToString()
        {
            return $"Vector2 : [ {x}, {y} ]";
        }
    }
}
