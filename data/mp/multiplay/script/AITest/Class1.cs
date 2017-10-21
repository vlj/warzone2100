using System;
using System.Runtime.InteropServices;

namespace AITest
{
    public class Class1
    {
        [DllImport("warzone2100.exe", EntryPoint = "check_sanity")]
        public static extern void check_sanity();

        static void Main()
        {
            check_sanity();
            Console.WriteLine("test");
            //System.IO.File.Create("test.tst");
        }
    }
}
