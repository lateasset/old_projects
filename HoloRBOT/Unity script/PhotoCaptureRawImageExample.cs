using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime;
using UnityEngine.Windows.WebCam;

public class PhotoCaptureRawImageExample : MonoBehaviour
{
    PhotoCapture photoCaptureObject = null;
    Texture2D targetTexture = null;
    Renderer quadRenderer = null;
    bool m_sending = false;
    private TcpClient socketConnection;
    private Thread m_NetworkThread;
    Resolution cameraResolution

    // Use this for initialization
    void Start()
    {
        cameraResolution = PhotoCapture.SupportedResolutions.OrderByDescending((res) => res.width * res.height).First();

        ConnectToTcpServer();

        targetTexture = new Texture2D(cameraResolution.width, cameraResolution.height, TextureFormat.RGBA32, false);

        PhotoCapture.CreateAsync(false, delegate (PhotoCapture captureObject) {
            photoCaptureObject = captureObject;

            CameraParameters c = new CameraParameters();
            c.cameraResolutionWidth = targetTexture.width;
            c.cameraResolutionHeight = targetTexture.height;
            c.pixelFormat = CapturePixelFormat.BGRA32;

            captureObject.StartPhotoModeAsync(c, delegate (PhotoCapture.PhotoCaptureResult result) {
                photoCaptureObject.TakePhotoAsync(OnCapturedPhotoToMemory);
            });
        });
    }

    private void ConnectToTcpServer()
    {
        try
        {
            socketConnection = new TcpClient("192.168.1.193", 27015);

            sayHello();
            //hearHello();
            
            //m_NetworkThread = new Thread(new ThreadStart(NetworkThread));
            //m_NetworkThread.IsBackground = true;
            //m_NetworkThread.Start();
        }
        catch (Exception e)
        {
            Debug.Log("On client connect exception: " + e);
        }
    }



    List<byte> imageBufferList = new List<byte>();
    void OnCapturedPhotoToMemory(PhotoCapture.PhotoCaptureResult result, PhotoCaptureFrame photoCaptureFrame)
    {
        // Copy the raw IMFMediaBuffer data into our empty byte list.
        photoCaptureFrame.CopyRawImageDataIntoBuffer(imageBufferList);

        // In this example, we captured the image using the BGRA32 format.
        // So our stride will be 4 since we have a byte for each rgba channel.
        // The raw image data will also be flipped so we access our pixel data
        // in the reverse order.
        int stride = 4;
        float denominator = 1.0f / 255.0f;
        List<Color> colorArray = new List<Color>();
        for (int i = imageBufferList.Count - 1; i >= 0; i -= stride)
        {
            float a = (int)(imageBufferList[i - 0]) * denominator;
            float r = (int)(imageBufferList[i - 1]) * denominator;
            float g = (int)(imageBufferList[i - 2]) * denominator;
            float b = (int)(imageBufferList[i - 3]) * denominator;

            colorArray.Add(new Color(r, g, b, a));
        }

        m_sending = true;

        targetTexture.SetPixels(colorArray.ToArray());
        targetTexture.Apply();
        /*
        if (quadRenderer == null)
        {
            GameObject p = GameObject.CreatePrimitive(PrimitiveType.Quad);
            quadRenderer = p.GetComponent<Renderer>() as Renderer;
            quadRenderer.material = new Material(Shader.Find("Unlit/Texture")); //Custom/Unlit/UnlitTexture

            p.transform.parent = this.transform;
            p.transform.localPosition = new Vector3(0.0f, 0.0f, 1.0f);
        }

        quadRenderer.material.SetTexture("_MainTex", targetTexture);
        */
        // Take another photo
        photoCaptureObject.TakePhotoAsync(OnCapturedPhotoToMemory);
        SendMessage();
    }

    private void SendMessage()
    {
        m_Sending = false;
        bool success = false;
        try
        {
            NetworkStream stream = socketConnection.GetStream();
            if (stream.CanWrite)
            {
                stream.Write(imageBufferList.ToArray(), 0, imageBufferList.Count);
                Debug.Log("Client sent his message - should be received by server");
            }
        }
        catch
        {
            success = false;
            //client.client.Close();
        }
        m_Sending = !success;
    }

    private void sayHello()
    {
        NetworkStream stream = socketConnection.GetStream();
        if (stream.CanWrite)
        {
            //string clientMessage = "This is a message from one of your clients.";
            byte[] dataLength = BitConverter.GetBytes(cameraResolution.width * cameraResolution.height);
            stream.Write(dataLength, 0, sizeof(int));
            Debug.Log("Client sent his message - should be received by server");
        }
    }

    private void hearHello()
    {
        try
        {
            // Get a stream object for reading 				
            using (stream = socketConnection.GetStream())
            {
                int length;
                // Read incomming stream into byte arrary.
                var incommingData = new byte[length];
                Array.Copy(bytes, 0, incommingData, 0, length);
            }
        catch
        {

        }
    }

    private void NetworkThread()
    {
        //while (true)
        //{
            // Get a stream object for reading 				
            using ( stream = socketConnection.GetStream())
            {
                int length;
                // Read incomming stream into byte arrary. 					
                while ((length = stream.Read(bytes, 0, bytes.Length)) != 0)
                {
                    var incommingData = new byte[length];
                    Array.Copy(bytes, 0, incommingData, 0, length);
                    for (int i = 0; i < 16; i++)
                        array[i] = System.BitConverter.toSingle(incommingData + i * 4);

                    // ADJUST POSE!!!!!!
                    Matrix4x4 transfPose;
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            transfPose[j][i] = array[i*4 + j];
                    var matrix = transform.localToWorldMatrix;
                    matrix = matrix * transfPose;
                    transform.localPosition = matrix.GetColumn(3);
                    transform.localScale = new Vector3(matrix.GetColumn(0).magnitude, matrix.GetColumn(1).magnitude, matrix.GetColumn(2).magnitude);
                    float w = Mathf.Sqrt(1.0f + matrix[0,0] + matrix[1,1] + matrix[2,2]) / 2.0f;
                    transform.localRotation = new Quaternion((matrix[2,1]-matrix[1,2])/(4.0f*w), (matrix[0,2] - matrix[2,0]) / (4.0f * w), (matrix[1,0] - matrix[0,1]) / (4.0f * w), w);

                    // Convert byte array to string message. 						
                    //string serverMessage = Encoding.ASCII.GetString(incommingData);
                    //Debug.Log("server message received as: " + serverMessage);
                }
            }
        //}
    }
    
    private void update()
    {
        NetworkStream();
    }
}
