using System;

namespace HostileEngine
{
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

    public class Compiler
    {
        public static void Compile(string str)
        {
            Console.WriteLine(str);
        } 
    }

}
