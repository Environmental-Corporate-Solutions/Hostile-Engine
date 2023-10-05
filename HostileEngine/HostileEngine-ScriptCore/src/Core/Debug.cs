using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HostileEngine
{
    public class Debug
    {
        public static void Log(string str)
        {
            InternalCalls.Debug_Log(str);
        }
    }
}
