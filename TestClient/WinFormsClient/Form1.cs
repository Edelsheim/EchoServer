using System;
using System.Text;
using System.Windows.Forms;
using System.Net.Sockets;

namespace WinFormsClient
{
    public partial class Form1 : Form
    {
        private TcpClient client = null;
        public Form1()
        {
            InitializeComponent();
        }

        private void connect_button_Click(object sender, EventArgs e)
        {
            if (this.client != null)
            {
                this.client.Close();
                this.client?.Dispose();
                this.client = null;
            }

            if (this.client == null)
            {
                this.client = new TcpClient(ip_textbox.Text, int.Parse(port_textbox.Text));
            }
        }

        private void send_button_Click(object sender, EventArgs e)
        {
            if (this.client != null)
            {
                if (this.client.Connected)
                {
                    using (NetworkStream networkStream = this.client.GetStream())
                    {
                        string send = this.input_textbox.Text;
                        byte[] send_bytes = Encoding.Default.GetBytes(send);
                        networkStream.Write(send_bytes, 0, send_bytes.Length);
                    }
                }
            }
        }

        private void close_button_Click(object sender, EventArgs e)
        {
            if (this.client != null) {
                this.client.Close();
                this.client?.Dispose();
            }
            this.client = null;
        }
    }
}
