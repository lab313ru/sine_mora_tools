using System;
using System.Windows.Forms;

namespace dehasher
{
    public partial class frmMain : Form
    {
        static string sdbm(string str)
        {
            uint hash = 0;
            for (int i = 0; i < str.Length; i++)
            {
                hash = 0x1003F * (hash + str[i]);
            }
            return String.Format("{0:X8}", hash);
        }

        static UInt32 reverse(UInt32 value)
        {
            return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
                   (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
        }

        static string sdbm_rev(string str)
        {
            uint hash = 0;
            for (int i = 0; i < str.Length; i++)
            {
                hash = 0x1003F * (hash + str[i]);
            }


            return String.Format("{0:X8}", reverse(hash));
        }
        
        public frmMain()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            textBox2.Text = sdbm(textBox1.Text);
            textBox3.Text = sdbm_rev(textBox1.Text);
        }
    }
}
