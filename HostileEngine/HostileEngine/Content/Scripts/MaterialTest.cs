using HostileEngine;
using System.Diagnostics;

public class MaterialTest : Entity
{
    private Transform m_transform;

    void OnCreate()
    {
        Debug.Log($"Player create");
        m_transform = GetComponent<Transform>();
        Material material = GetComponent<Material>();
    }

    void OnUpdate()
    {
       
    }
}