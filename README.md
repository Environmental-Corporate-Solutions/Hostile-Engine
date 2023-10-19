# Hostile-Engine
### Build Status
Main:
| OS           | Architecture       | Status                       |
|--------------|--------------------|------------------------------|
| Windows      | x64                |[![Build-Windows-x64](https://github.com/Environmental-Corporate-Solutions/Hostile-Engine/actions/workflows/build_win_64.yml/badge.svg?branch=main)](https://github.com/Environmental-Corporate-Solutions/Hostile-Engine/actions/workflows/build_win_64.yml)|


### Scripting Example
```cs
using HostileEngine;
public class Player:Entity
{
    private Transform m_transform;

    void OnCreate()
    {
        Debug.Log($"Player create");
        m_transform = GetComponent<Transform>();
    }

    void OnUpdate()
    {
        float speed=1.0f;
        Vector3 pos=m_transform.Position;
        
        if (Input.IsPressed(KeyCode.Up))
        {
            pos.Y += speed;
        }
        else if (Input.IsPressed(KeyCode.Down))
        {
            pos.Y -= speed;
        }

        if (Input.IsPressed(KeyCode.Right))
        {
            pos.X += speed;
        }
        else if (Input.IsPressed(KeyCode.Left))
        {
            pos.X -= speed;
        }
        m_transform.Position=pos;
    }
}
```
