using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;


public class EventProcessor : MonoBehaviour
{
    //public Text TextDebug;
    public TextMeshPro TextDebug; // Replaced with TextMeshPro object as the old Unity text mesh is deprecated
    //public Transform CubeOrientation;
    //private Quaternion CubeOrientation = this.transform.rotation;

    private System.Object _queueLock = new System.Object();
    
    List<byte[]> _queuedData = new List<byte[]>();
    List<byte[]> _processingData = new List<byte[]>();

    public void QueueData(byte[] data)
    {
        lock (_queueLock)
        {
            _queuedData.Add(data);
        }
    }


    void Update()
    {
        MoveQueuedEventsToExecuting();
        while (_processingData.Count > 0)
        {
            var byteData = _processingData[0];
            _processingData.RemoveAt(0);
            try
            {
                var IMUData = IMU_DataPacket.ParseDataPacket(byteData);
                TextDebug.text = IMUData.ToString();// + "\r\n" + IMUData.rX.ToString();
                //UPDATE CUBE ORIENTATION HERE
                Quaternion CubeOrientation = new Quaternion();

                // Convert IMU right-handed z-up [i,j,k,real] to Unity's left-handed y-up [x,y,z,w] 
                // Will require object to be adjusted in Unity by x = -180, y = -90, z = 0; starting adjust will be x = 180
                CubeOrientation.Set(IMUData.rX*-1, IMUData.rZ*-1, IMUData.rY*-1, IMUData.rW);
                this.transform.localRotation = CubeOrientation;
            }
            catch (Exception e)
            {
                TextDebug.text = "Error: " + e.Message;
            }
        }
    }


    private void MoveQueuedEventsToExecuting()
    {
        lock (_queueLock)
        {
            while (_queuedData.Count > 0)
            {
                byte[] data = _queuedData[0];
                _processingData.Add(data);
                _queuedData.RemoveAt(0);
            }
        }
    }

    public void PrintFlags(bool devicefound, bool deviceconnected)
    {
        if (devicefound && deviceconnected)
        {
            TextDebug.text = "Device Found and Connected!";
        }
        if(devicefound && !deviceconnected)
        {
            TextDebug.text = "Device Found and not yet connected!";
        }
        else
        {
        
        }
    }

    public void DebugMessages(string text)
    {
        TextDebug.text = text;
    }
}