using HostileEngine;
public class PlayerController:Entity
{
    private Transform m_transform;
    public float speed = 1.0f;

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

        m_transform.Position = pos;
    
    }
}
