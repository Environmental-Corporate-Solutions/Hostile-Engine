
namespace HostileEngine
{
    public class Input
    {
        public static bool IsPressed(KeyCode key)
        {
            return InternalCalls.Input_IsPressed_Key(key);
        }
        public static bool IsPressed(MouseCode mouse)
        {
            return InternalCalls.Input_IsPressed_Mouse(mouse);
        }

        public static bool IsTriggered(KeyCode key)
        {
            return InternalCalls.Input_IsTriggered_Key(key);
        }
        public static bool IsTriggered(MouseCode mouse)
        {
            return InternalCalls.Input_IsTriggered_Mouse(mouse);
        }

        public static bool IsRepeating(KeyCode key)
        {
            return InternalCalls.Input_IsRepeating_Key(key);
        }

        public static bool IsReleased(KeyCode key)
        {
            return InternalCalls.Input_IsReleased_Key(key);
        }
        public static bool IsReleased(MouseCode mouse)
        {
            return InternalCalls.Input_IsReleased_Mouse(mouse);
        }
    }
}
