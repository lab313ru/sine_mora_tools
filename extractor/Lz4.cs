using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Lz4Net
{
    public static unsafe class Lz4
    {
        #region ** 32 Bit DllImports **

        [DllImport("lz4_r10.dll", EntryPoint = "LZ4_compress", CallingConvention = CallingConvention.Cdecl)]
        public static extern Int32 LZ4_compress(byte* source, byte* dest, Int32 isize);
        [DllImport("lz4_r10.dll", EntryPoint = "LZ4_decode", CallingConvention = CallingConvention.Cdecl)]
        public static extern Int32 LZ4_decode(byte* source, byte* dest, Int32 isize);

        #endregion
    }

}