using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using UnityEngine;

public class SendUDP : MonoBehaviour {

    // Camera
    public string cameraName = "Main Camera";
    GameObject camera;

    public int remotePort = 55556;

    Socket server;
    IPEndPoint ep;

    // Use this for initialization
    void Start () {
        // Find Game Object
        camera = GameObject.Find(cameraName);

        server = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
        ep = new IPEndPoint(IPAddress.Loopback, remotePort);
    }
	
	// Update is called once per frame
	void Update () {
        double cameraX = camera.transform.position.x;
        double cameraY = camera.transform.position.z;
        double cameraZ = camera.transform.position.y;
        double cameraRoll = camera.transform.eulerAngles.z;
        double cameraPitch = camera.transform.eulerAngles.x;
        double cameraYaw = camera.transform.eulerAngles.y;


        byte[] dataPackage = new byte[6 * 8];
        appendTo(dataPackage, BitConverter.GetBytes(cameraX), 0);
        appendTo(dataPackage, BitConverter.GetBytes(cameraY), 8);
        appendTo(dataPackage, BitConverter.GetBytes(cameraZ), 16);
        appendTo(dataPackage, BitConverter.GetBytes(cameraRoll), 24);
        appendTo(dataPackage, BitConverter.GetBytes(cameraPitch), 32);
        appendTo(dataPackage, BitConverter.GetBytes(cameraYaw), 40);
        server.SendTo(dataPackage, ep);
    }

    private void appendTo(byte[] dest, byte[] src, int pos)
    {
        dest[pos + 0] = src[0];
        dest[pos + 1] = src[1];
        dest[pos + 2] = src[2];
        dest[pos + 3] = src[3];
        dest[pos + 4] = src[4];
        dest[pos + 5] = src[5];
        dest[pos + 6] = src[6];
        dest[pos + 7] = src[7];
    }
}
