using System;
using HostileEngine;

namespace JJEngine
{
    static class Graphics
    {
        public static void SetClearColor(float r, float g, float b)
        {
            Console.Write("SetClearColor");
            //InternalCalls.Graphics_SetClearColor(r,g,b);
        }
    }


    static class Debug
    {
        public static void Log(string str)
        {
            InternalCalls.Debug_Log(str);
        }
    }
    public class main
    {
        public main()
        {
            Debug.Log("Test log from c#");
            test();
        }

        public void test()
        {
            Console.WriteLine("color change");
            //Graphics.SetClearColor(1.0f, 0.8f, 0.3f);
        }

        public void printStr(string str)
        {
            Console.WriteLine(str);
        }
    }

    public class test
    {
        public string iamstr = "yes i am";
        public string test_str
        {
            get { return "test_str returned"; }
        }

        public void run(float val)
        {
            Console.WriteLine("test class run! {0}", val);
        }
    }

}
