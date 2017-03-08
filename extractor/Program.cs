//using LZ4Sharp;
using Lz4Net;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace extractor
{
    class Program
    {
        const string good_paths = "paths.txt";
        const string hash_nf = "hash_nf.txt";
        const string hash1_log = "hash1_log.txt";
        const string hash2_log = "hash2_log.txt";
        const string source = "source.bin";
        const string source_strings = "source.txt";
        const string direcs = "dirs.txt";
        const string gen_ = "gen.txt";
        const string alpha = "abcdefghijklmnopqrstuvwxyz_";
        static byte alpha_len = (byte)alpha.Length;
        
        static List<string> DedupCollection (List<string> input )
        {
            HashSet<string> passedValues = new HashSet<string>();
            
            //relatively simple dupe check alg used as example
            int len = input.Count;
            for (int i = 0; i < len; i++)
            {
                string item = input[i];
                if (passedValues.Contains(item))
                    continue;
                else
                    passedValues.Add(item);
            }

            List<string> retn = new List<string>();
            retn.AddRange(passedValues);
            return retn;
        }
        
        struct block
        {
            public uint dwPath;
            public uint dwFName;
            public uint dwExt;
            public uint dwDSize;
            public uint dwCSize;
        }

        public static T ByteToType<T>(BinaryReader reader)
        {
            byte[] bytes = reader.ReadBytes(Marshal.SizeOf(typeof(T)));

            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
            T theStructure = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();

            return theStructure;
        }

        static uint sdbm(string str)
        {
            uint hash = 0;
            for (int i = 0; i < str.Length; i++)
            {
                hash = 0x1003F * (hash + str[i]);
            }
            return hash;
        }

        static uint sdbm(byte[] ptr, int size)
        {
            uint hash = 0;
            for (int i = 0; i < size; i++)
            {
                hash = 0x1003F * (hash + ptr[i]);
            }
            return hash;
        }

        static string getExt(uint hash)
        {
            switch (hash)
            {
                case 0x5E5AB54D: return "dds";
                case 0x1E1EA7D5: return "sph";
                case 0x06C0B650: return "mp3";
                case 0x5CE18709: return "dat";
                case 0x93FF62E0: return "psc";
                case 0x17B1DCCE: return "scb";
                case 0x1E22F830: return "bson";
                case 0xD7EBE1E0: return "loc";
                case 0x208EF3E0: return "sub";
                case 0x5ED5C411: return "dep";
                case 0x71F3BA83: return "hlsl";
                case 0xF10C041F: return "rsb";
                case 0x1F92D4DE: return "ssb";
                case 0x08F2025B: return "mtl";
                case 0x35B99DDD: return "cpp";
                default: return "";
            }
        }

        static string findPath(List<string> founded, uint hash){
            int len = founded.Count;
            for (int i = 0; i < len; i++)
            {
                uint h = sdbm(founded[i]);

                if (h == hash)
                    return founded[i];
            }

            List<string> hashes = new List<string>();
            if (File.Exists(hash_nf))
                hashes.AddRange(File.ReadAllLines(hash_nf));
            string hsh = String.Format("{0:X8}", hash);
            if (hashes.Contains(hsh))
                return hsh;

            string[] paths = File.ReadAllLines(good_paths);

            len = paths.Length;
            for (int i = 0; i < len; i++)
            {
                uint h = sdbm(paths[i]);

                if (h == hash)
                    return paths[i];
            }
            File.AppendAllText(hash_nf, hsh + Environment.NewLine);
            return String.Format("{0:X8}", hash);
        }

        static void parse(int min, string src)
        {            
            const string alpha = "abcdefghijklmnopqrstuvwxyz0123456789-+_=/.";
            const string not_start = "-+_/=. ";

            BinaryReader fl = new BinaryReader(File.OpenRead(src));
            List<string> res = new List<string>();

            long len = fl.BaseStream.Length;
            while (fl.BaseStream.Position < len)
            {
                char c;
                string line = "";

                while (alpha.IndexOf(c = (char)fl.ReadByte()) != -1)
                    line += c;

                if (line.Length < min) continue;

                while (line.Length != 0 && not_start.Contains(line[0].ToString())) line = line.Substring(1);

                if (line.Length < min) continue;

                res.Add(line.Trim());

                //if (i % 1000000 == 0)
                //   Console.WriteLine((i * 100 / len) + "%");
            }

            res = DedupCollection(res);
            File.AppendAllLines(gen_, res);
        }

        static void parse2(int min)
        {
            string[] lines = File.ReadAllLines(gen_);
            List<string> res = new List<string>();
            int len = lines.Length;

            for (int i = 0; i < len; i++)
            {
                string s = lines[i];
                int sl = s.Length;

                if (sl < min) continue;
                    res.Add(s);

                if (s.IndexOf('/') > 0)
                {
                    string[] ss = s.Split('/');

                    for (int j = 0; j < ss.Length; j++)
                    {
                        if (ss[j].Length < min) continue;
                            res.Add(ss[j]);
                    }
                }

                if (s.IndexOf('_') > 0)
                {
                    string[] ss = s.Split('_');

                    int l = ss.Length;
                    for (int j = 0; j < l; j++)
                    {
                        if (ss[j].Length < min) continue;
                            res.Add(ss[j]);
                    }
                }

                if (s.IndexOf('=') > 0)
                {
                    string[] ss = s.Split('=');

                    int l = ss.Length;
                    for (int j = 0; j < l; j++)
                    {
                        if (ss[j].Length < min) continue;
                        res.Add(ss[j]);
                    }
                }

                if (s.IndexOf('-') > 0)
                {
                    string[] ss = s.Split('-');

                    int l = ss.Length;
                    for (int j = 0; j < l; j++)
                    {
                        if (ss[j].Length < min) continue;
                        res.Add(ss[j]);
                    }
                }

                if (s.IndexOf('+') > 0)
                {
                    string[] ss = s.Split('+');

                    int l = ss.Length;
                    for (int j = 0; j < l; j++)
                    {
                        if (ss[j].Length < min) continue;
                        res.Add(ss[j]);
                    }
                }

                if (s.IndexOf('.') > 0)
                {
                    string[] ss = s.Split('.');

                    int l = ss.Length;
                    for (int j = 0; j < l; j++)
                    {
                        if (ss[j].Length < min) continue;
                        res.Add(ss[j]);
                    }
                }

                for (int j = 1; j < sl; j++)
                {
                    string add = s.Substring(j);

                    if (add.Length < min) break;
                        res.Add(add);
                }

                if (i % 10000 == 0)
                {
                    res = DedupCollection(res);

                    Console.WriteLine(i + " " + len);
                }
            }

            //res.AddRange(File.ReadAllLines(good_paths));
            res = DedupCollection(res);
            File.WriteAllLines(gen_, res);
        }

        static void getDirs()
        {
            const string not_start = ". ";
            
            List<string> res = new List<string>();
            res.AddRange(File.ReadAllLines(good_paths));

            List<string> dirs = new List<string>();

            int len = res.Count;
            for (int i = 0; i < len; i++)
            {
                string dir = res[i];

                if (!dir.Contains("/")) continue;
                if (dir.Contains(not_start[0].ToString()) || dir.Contains(not_start[1].ToString())) continue;

                while ((dir = Path.GetDirectoryName(dir)) != "" && dir != null)
                    dirs.Add(dir.Replace('\\', '/') + '/');
            }
            dirs = DedupCollection(dirs);
            File.WriteAllLines(direcs, dirs);
        }

        static void getNames()
        {            
            List<string> res = new List<string>();
            res.AddRange(File.ReadAllLines(gen_));

            List<string> names = new List<string>();

            int len = res.Count;

            for (int i = 0; i < len; i++)
            {
                string name = Path.GetFileNameWithoutExtension(res[i]);

                if (name != "")
                    names.Add(name);
            }

            names = DedupCollection(names);
            File.WriteAllLines(gen_, names);
        }

        static void genPaths()
        {
            List<string> names = new List<string>();
            names.AddRange(File.ReadAllLines(gen_));
            List<string> dirs = new List<string>();
            dirs.AddRange(File.ReadAllLines(direcs));

            List<string> paths = new List<string>();

            int dc = dirs.Count;
            int nc = names.Count;

            for (int j = 0; j < dc; j++)
                for (int i = 0; i < nc; i++)
                    paths.Add(dirs[j] + names[i]);

            paths = DedupCollection(paths);
            File.WriteAllLines(gen_, paths);
        }

        static void getIncludes()
        {
            string[] ext = new string[] { ".dds", ".sph", ".mp3", ".dat", ".psc", ".scb", ".bson", ".loc", ".sub", ".dep", ".hlsl", ".rsb", ".ssb", ".mtl", ".cpp" };
            List<string> exts = new List<string>();
            exts.AddRange(ext);

            const string alpha = "abcdefghijklmnopqrstuvwxyz0123456789-+_=.";
            BinaryReader fl = new BinaryReader(File.OpenRead(source));
            List<string> res = new List<string>();
            
            long len = fl.BaseStream.Length;
            while (fl.BaseStream.Position < len)
            {
                char c;
                string line = "";

                while (alpha.IndexOf(c = (char)fl.ReadByte()) != -1)
                    line += c;

                if (line.Length < 4 || !line.Contains(".") || line[0] == '.' || res.Contains(line)) continue;
                if (!exts.Contains(Path.GetExtension(line))) continue;

                res.Add(Path.GetFileNameWithoutExtension(line));

                //if (i % 1000000 == 0)
                //   Console.WriteLine((i * 100 / len) + "%");
            }

            res = DedupCollection(res);
            File.WriteAllLines(gen_, res);
        }

        static void genForHash2()
        {
            string[] ext = new string[] { ".dds", ".sph", ".mp3", ".psc", ".scb", ".rsb", ".ssb", ".mtl" };
            
            List<string> lines = new List<string>();
            lines.AddRange(File.ReadAllLines(gen_));

            List<string> gen = new List<string>();

            for (int i = 0; i < lines.Count; i++)
            {
                for (int j = 0; j < ext.Length; j++)
                {
                    for (int k = 0; k < 10; k++ )
                        gen.Add(lines[i] + k.ToString() + ext[j]);

                    for (int k = 0; k < 10; k++)
                        gen.Add(lines[i] + k.ToString("D2") + ext[j]);

                    for (int k = 10; k < 16; k++)
                        gen.Add(lines[i] + "_" + k.ToString("x") + ext[j]);
                }
            }

            gen = DedupCollection(gen);

            lines.Clear();
            lines.AddRange(File.ReadAllLines(hash2_log));

            for (int i = 0; i < gen.Count; i++)
            {
                string hash = String.Format("{0:X8}", sdbm(gen[i]));

                if (lines.Contains(hash))
                {
                    Console.WriteLine(gen[i]);
                    File.AppendAllText("hash2_good.txt", gen[i] + Environment.NewLine);
                }
            }
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

        static unsafe void Main(string[] args)
        {
            //Console.WriteLine("step 1");
            //parse(3);
            //Console.WriteLine("step 2");
            //parse2(3);
            /*Console.WriteLine("step 3");
            getDirs();
            Console.WriteLine("step 4");
            getNames();
            Console.WriteLine("step 5");
            genPaths();*/
            //return;

            //getDirs();
            //getNames();
            //getIncludes();
            //genPaths();
            //return;

            /*string[] bsons = File.ReadAllLines("bsons.txt");

            for (int i = 0; i < bsons.Length; i++)
            {
                parse(5, bsons[i]);
            }
            return;*/

            //genForHash2();
            //return;

            File.Delete(hash1_log);
            File.Delete(hash2_log);

            List<string> good = new List<string>();
            if (File.Exists("good.txt")) good.AddRange(File.ReadAllLines("good.txt"));
            string[] list = File.ReadAllLines(args[0]);

            int len = list.Length;
            for (int i = 0; i < len; i++)
            {
                BinaryReader file = new BinaryReader(File.Open(list[i], FileMode.Open, FileAccess.Read));

                long pos;
                long size = file.BaseStream.Length;
                while ((pos = file.BaseStream.Position) < size)
                {
                    block b = ByteToType<block>(file);

                    byte[] input = file.ReadBytes((int)b.dwCSize);
                    byte[] output = new byte[b.dwDSize];

                    fixed (byte* inp = &input[0])
                        fixed (byte* outp = &output[0])
                            Lz4.LZ4_decode(inp, outp, input.Length);

                    string name = findPath(good, (uint)b.dwPath);
                    string path = Path.GetDirectoryName(list[i]) + '\\' + Path.GetFileNameWithoutExtension(list[i]) + '\\' + name;
                    path = path.Replace('/', '\\');
                    path = Path.GetDirectoryName(path);
                    

                    Directory.CreateDirectory(path);

                    string ext = getExt((uint)b.dwExt);
                    string fname = String.Format("{0}\\{1}.{2}", path, Path.GetFileNameWithoutExtension(name), ext);
                    string h1 = String.Format("{0:X8}", b.dwPath);
                    string h2 = String.Format("{0:X8}", b.dwFName);

                    if (!File.Exists(fname))
                    {
                        File.WriteAllBytes(fname, output);
                        File.Delete(path + '\\' + h1 + ext);
                    }

                    if (h1 != name)
                    {
                        if (good.IndexOf(name) == -1)
                        {
                            good.Add(name);
                            File.WriteAllLines("good.txt", good);                            
                        }
                    }
                    else
                    {
                        File.AppendAllText(hash2_log, h2 + '=' + name + '.' + ext + Environment.NewLine);
                    }

                    File.AppendAllText(hash1_log, name + Environment.NewLine);

                    Console.WriteLine(String.Format("{0}/{1}: {2} {3}%", i, len, name, (pos * 100) / size));
                }
            }
            File.WriteAllLines("good.txt", good);
        }
    }
}
