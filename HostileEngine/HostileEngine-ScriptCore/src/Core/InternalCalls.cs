using System;
using System.Runtime.CompilerServices;

namespace HostileEngine
{
    internal static class InternalCalls
    {
        #region Debug

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Debug_Log(string str);
        #endregion
    }
}
