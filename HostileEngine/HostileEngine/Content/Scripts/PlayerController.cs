using HostileEngine;
public class PlayerController:Entity
{
    private Transform m_transform;
    public float speed = 0.05f;

    void OnCreate()
    {
        m_transform = GetComponent<Transform>();
    }

    void OnUpdate()
    {  
        Vector3 pos = m_transform.Position;
        if(Input.IsPressed(KeyCode.W))
        {
            pos.Z += speed;
        }
		if (Input.IsPressed(KeyCode.S))
		{
			pos.Z -= speed;
		}
		if (Input.IsPressed(KeyCode.D))
		{
			pos.X += speed;
		}
		if (Input.IsPressed(KeyCode.A))
		{
			pos.X -= speed;
		}
		if (Input.IsPressed(KeyCode.Space))
		{
			pos.Y += 5;
		}

		m_transform.Position = pos;
    
    }
}
