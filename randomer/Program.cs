using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace randomer
{
    class Program
    {
        static string prefix = "";
        static string postfix = "";//"_button_mouse";
        static string alpha = "abcdefghijklmnopqrstuvwxyz";
        static byte alpha_len = (byte)alpha.Length;
        static uint hash_1 = 0;
        static uint hash_2 = 0;
        static uint init_ = 0;
        static int cores = 1;
        static ulong passed = 0;
        static string ext = "";

        static bool finished = false;

        static uint sdbm(string str, uint init)
        {
            uint hash = init;
            int len = str.Length;
            for (int i = 0; i < len; i++)
            {
                hash = 0x1003F * (hash + str[i]);
            }
            return hash;
        }

        static UInt64 sdbm_(string str)
        {
            UInt64 hash = 0;
            int len = str.Length;
            for (int i = 0; i < len; i++)
            {
                hash = 0x1003F * (hash + str[i]);
            }
            return hash;
        }

        static string ColIdx2ColName(ulong idx)
        {
            string col = alpha[(int)(idx % alpha_len)].ToString();
            while (idx >= alpha_len)
            {
                idx = (idx / alpha_len) - 1;
                col = alpha[(int)(idx % alpha_len)] + col;
            }
            return col;
        }

        static ulong ColName2ColIdx(string col)
        {            
            ulong sum = 0;

            for (int i = 0; i < col.Length; i++)
            {
                sum *= alpha_len;
                sum += (ulong)(alpha.IndexOf(col[i]) + 1);
            }
            
            return (sum == 0 ? 0 : sum - 1);
        }

        static bool testPath(string path)
        {
            return (sdbm(path + postfix, init_) == hash_1);
        }

        static void brutePath(int thread, ref ulong passed)
        {
            string path = "";
            ulong idx = (ulong)thread + passed;

            while (true)
            {
                do
                {
                    path = ColIdx2ColName(idx);
                    idx += (ulong)cores;
                    passed++;
                }
                while (!testPath(path));

                string s = Path.GetFileNameWithoutExtension(prefix + path + postfix) + ext;
                finished = (sdbm(s, 0) == hash_2);

                if (finished)
                {
                    Console.WriteLine(path);
                    File.AppendAllText(thread.ToString() + "_result.txt", path + Environment.NewLine);
                }
            }
        }

        static void printPassed(ulong passed)
        {
            while (!finished)
            {
                //if ((int)((DateTime.Now - curr).TotalSeconds) % 5 == 0)
                //{
                    Console.Clear();
                    Console.WriteLine(prefix + ColIdx2ColName(passed) + postfix);
                //}
            }
        }

        protected static void closeConsole(object sender, ConsoleCancelEventArgs args)
        {
            string last = ColIdx2ColName(passed);
            File.WriteAllText(String.Format("last_{0:X8}.txt", hash_1), last);
            args.Cancel = true;
            
            // Announce the new value of the Cancel property.
            Console.WriteLine("Last bruted text: {0}", last);
        }

        static int idx2len(ulong idx, byte alpha_len)
        {
            int len = 1;

            while (idx >= alpha_len)
            {
                idx = (idx / alpha_len) - 1;
                len++;
            }

            return len;
        }
        
        static void Main(string[] args)
        {
            //List<string> strings = GenerateStrings().Take(0x10000000).ToList();
            //File.WriteAllLines("genned.txt", strings);
            //return;

            ulong idx = ColName2ColIdx("zabcc");
            Console.WriteLine(idx);
            Console.WriteLine(idx2len(idx, 26));
            
            return;

            Console.CancelKeyPress += new ConsoleCancelEventHandler(closeConsole);
            
            foreach (string s in args)
            {
                if (s.ToLower().IndexOf("-pre=") != -1)
                {
                    prefix = s.Substring(5);
                    init_ = sdbm(prefix, 0);
                }
                else if (s.ToLower().IndexOf("-post=") != -1)
                {
                    postfix = s.Substring(6);
                }
                else if (s.ToLower().IndexOf("-alpha=") != -1)
                {
                    alpha = s.Substring(7);
                    alpha_len = (byte)alpha.Length;
                }
                else if (s.ToLower().IndexOf("-hash1=") != -1)
                {
                    hash_1 = Convert.ToUInt32(s.Substring(7), 16);
                }
                else if (s.ToLower().IndexOf("-hash2=") != -1)
                {
                    hash_2 = Convert.ToUInt32(s.Substring(7), 16);
                }
                else if (s.ToLower().IndexOf("-last=") != -1)
                {
                    passed = ColName2ColIdx(s.Substring(7));
                }
                else if (s.ToLower().IndexOf("-ext=") != -1)
                {
                    ext = '.' + s.Substring(5);
                }
            }

            if (passed == 0 && File.Exists(String.Format("last_{0:X8}.txt", hash_1)))
                passed = ColName2ColIdx(File.ReadAllText(String.Format("last_{0:X8}.txt", hash_1)));

            cores = Environment.ProcessorCount;

            Console.WriteLine(String.Format("String = {0}XXX{1}\nHash = {2:X8}\nAlphabet = {3}\nCores = {4}\n\n", prefix, postfix, hash_1, alpha, cores));

            Parallel.Invoke(() =>
            {
                brutePath(0, ref passed);
            },
            () =>
            {
                brutePath(1, ref passed);
            },
            () =>
            {
                brutePath(2, ref passed);
            },
            () =>
            {
                brutePath(3, ref passed);
            });

            Console.WriteLine("finished");
        }
    }
}
