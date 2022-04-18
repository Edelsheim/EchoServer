using System;
using System.Text;
using System.Windows.Forms;
using System.Net.Sockets;
using System.Threading;

namespace WinFormsClient
{
    public partial class Form1 : Form
    {
        private TcpClient client = null;
        private NetworkStream ns = null;
        private Thread readThread = null;
        private bool readThreadRun = false;
        private object lockObj = new object();
        public Form1()
        {
            InitializeComponent();
        }

        private void connect_button_Click(object sender, EventArgs e)
        {
            if (this.ns != null)
            {
                this.ns.Close();
                this.ns = null;
            }
            if (this.client != null)
            {
                this.client?.Close();
                this.client?.Dispose();
                this.client = null;
            }

            if (this.client == null)
            {
                this.client = new TcpClient(ip_textbox.Text, int.Parse(port_textbox.Text));
                if (this.client.Connected)
                    this.ns = this.client.GetStream();
            }

            this.readThread = new Thread(new ThreadStart(ThreadRunner));
            this.readThread.Start();
        }

        private void send_button_Click(object sender, EventArgs e)
        {
            if (this.ns != null && this.client != null)
            {
                if (this.client.Connected || this.ns.CanWrite)
                {
                    lock(this.lockObj)
                    {
                        string data = input_textbox.Text;
                        byte[] send_data = Encoding.Default.GetBytes(data);
                        this.ns.Write(send_data, 0, send_data.Length);
                    }
                }
            }
        }

        private void close_button_Click(object sender, EventArgs e)
        {

            if (this.readThread != null)
            {
                this.readThread.Join();
            }
            if (this.ns != null)
            {
                this.ns.Close();
            }
            this.ns = null;

            if (this.client != null) {
                this.client?.Close();
                this.client?.Dispose();
            }
            this.client = null;
        }

        private void ThreadRunner()
        {
            this.readThreadRun = true;
            while(true)
            {
                if (this.readThreadRun == false)
                    break;

                if (this.ns == null)
                    continue;
                if (this.ns.CanRead)
                {
                    byte[] recv = new byte[2048];
                    this.ns.Read(recv, 0, 2048);
                    string recv_str = Encoding.Default.GetString(recv);
                    Invoke(new Action(() =>
                    {
                        this.log_list.Items.Add(recv_str);
                    }));
                }
            }
        }
    }
}
