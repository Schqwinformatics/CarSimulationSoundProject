using System;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.Audio;

public class ReceiveUDP : MonoBehaviour
{

    const double cycleTime = 500;
    bool cycleState = false;

    // read Thread
    Thread readThread;

    // udpclient object
    UdpClient client;

    // port number
    public int port = 55555;

    // Game Object
    public string cubeModelName = "Cube";
    GameObject cube;

    double x;
    double y;
    double z;
    double rpm;
    double pitch;
    double yaw;
  
    double xOff = 0;
    double yOff = 0;
    double zOff = 0;

    void Start()
    {
        // Find Game Object
        cube = GameObject.Find(cubeModelName);

        // create thread for reading UDP messages
        readThread = new Thread(new ThreadStart(ReceiveData));
        readThread.IsBackground = true;
        readThread.Start();
    }

    // Update is called once per frame
    void Update()
    {
        cube.transform.position = new Vector3((float)(x+xOff), (float)(z+zOff), (float)(y+yOff));
        cube.transform.rotation = Quaternion.Euler((float) 0, (float)(yaw), (float)pitch);

        FindObjectOfType<AudioManager>().SetPitch("NormalRpm", MapRpmToPitch(rpm));
    }

    // Unity Application Quit Function
    void OnApplicationQuit()
    {
        StopThread();
    }

    // Stop reading UDP messages
    private void StopThread()
    {
        if (readThread.IsAlive)
        {
            readThread.Abort();
        }
        client.Close();
    }

    private double MapRpmToPitch(double rpm)
    {
        return (double)(rpm / 2500);
    }

    // receive thread function
    private void ReceiveData()
    {
        client = new UdpClient(port);
        while (true)
        {
            try
            {
                // receive bytes
                IPEndPoint anyIP = new IPEndPoint(IPAddress.Any, 0);
                byte[] data = client.Receive(ref anyIP);

                x = BitConverter.ToDouble(data, 0);
                y = BitConverter.ToDouble(data, 8);
                z = BitConverter.ToDouble(data, 16);
                rpm = BitConverter.ToDouble(data, 24);
                pitch = BitConverter.ToDouble(data, 32);
                yaw = BitConverter.ToDouble(data, 40);
               
            }
            catch (Exception err)
            {
                print(err.ToString());
            }
        }
    }
}
