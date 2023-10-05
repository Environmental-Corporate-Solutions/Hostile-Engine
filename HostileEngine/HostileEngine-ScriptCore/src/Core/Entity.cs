using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HostileEngine
{
    public class Entity
    {
        public readonly UInt64 ID;
        protected Entity()
        {
            ID = 0;
        }

        protected Entity(UInt64 _ID)
        {
            ID = _ID;
        }

        protected virtual void OnCreate()
        {
            
        }
        protected virtual void OnUpdate()
        {

        }
    }
}
