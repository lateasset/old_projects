/**
 *   #, #,         CCCCCC  VV    VV MM      MM RRRRRRR
 *  %  %(  #%%#   CC    CC VV    VV MMM    MMM RR    RR
 *  %    %## #    CC        V    V  MM M  M MM RR    RR
 *   ,%      %    CC        VV  VV  MM  MM  MM RRRRRR
 *   (%      %,   CC    CC   VVVV   MM      MM RR   RR
 *     #%    %*    CCCCCC     VV    MM      MM RR    RR
 *    .%    %/
 *       (%.      Computer Vision & Mixed Reality Group
 *                For more information see <http://cvmr.info>
 *
 * This file is part of RBOT.
 *
 *  @copyright:   RheinMain University of Applied Sciences
 *                Wiesbaden Rüsselsheim
 *                Germany
 *     @author:   Henning Tjaden
 *                <henning dot tjaden at gmail dot com>
 *    @version:   1.0
 *       @date:   30.08.2018
 *
 * RBOT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RBOT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RBOT. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>/*
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#define MAX 80
#define PORT 8080
#define SA struct sockaddr*/

#include "boost/asio.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

#include "glad/glad.h"

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "camera_calibration.hpp"
#include "shader.h"
#include "object3d.h"
#include "pose_estimator6d.h"
//#include "server.h"

using namespace std;
using namespace cv;

cv::Mat drawResultOverlay(const vector<Object3D*>& objects, const cv::Mat& frame)
{
    // render the models with phong shading
    RenderingEngine::Instance()->setLevel(0);
    
    vector<Point3f> colors;
    colors.push_back(Point3f(1.0, 0.5, 0.0));
    //colors.push_back(Point3f(0.2, 0.3, 1.0));
//    RenderingEngine::Instance()->renderShaded(vector<Model*>(objects.begin(), objects.end()), GL_FILL, colors, true);
    RenderingEngine::Instance()->renderSilhouette(vector<Model*>(objects.begin(), objects.end()), true, GL_FILL, colors, true);
    
    // download the rendering to the CPU
    Mat rendering = RenderingEngine::Instance()->downloadFrame(RenderingEngine::RGB);
    
    // download the depth buffer to the CPU
    Mat depth = RenderingEngine::Instance()->downloadFrame(RenderingEngine::DEPTH);
    
    // compose the rendering with the current camera image for demo purposes (can be done more efficiently directly in OpenGL)
    Mat result = frame.clone();
    for(int y = 0; y < frame.rows; y++)
    {
        for(int x = 0; x < frame.cols; x++)
        {
            Vec3b color = rendering.at<Vec3b>(y,x);
            if(depth.at<float>(y,x) != 0.0f)
            {
                result.at<Vec3b>(y,x)[0] = color[2];
                result.at<Vec3b>(y,x)[1] = color[1];
                result.at<Vec3b>(y,x)[2] = color[0];
            }
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    // camera image size
    int width = 1408;
    int height = 792;
    
    // near and far plane of the OpenGL view frustum
    float zNear = 10.0;
    float zFar = 10000.0;
    
    bool found = false;
    Matx33f K;
    Matx14f distCoeffs;
    
    if (found) {
        if (!calib_mat_coeff("in_VID5.xml", width, 0.f)) {
            FileStorage fs("out_camera_data.xml", FileStorage::READ); // Read the settings
            if (!fs.isOpened())
            {
                cout << "Could not open the calibration file: \" out_camera_data.xml \"" << endl;
        //        parser.printMessage();
                return -1;
            }
            fs["camera_matrix"] >> K;
            fs["distortion_coefficients"] >> distCoeffs;
            fs.release();
        } else {
            cout << "No calib no file";
            return -1;
        }
    } else {
        // camera instrinsics
        K = Matx33f(1031.328, 0, 679.696, 0, 1034.549, 394.479, 0, 0, 1);
        distCoeffs =  Matx14f(0.231, -0.367, -0.0016, -0.0013);
    }
    
    // distances for the pose detection template generation
    vector<float> distances = {200.0f, 400.0f, 600.0f}; // 2 4 6 for templates sake
    
    RenderingEngine::Instance();
    // load 3D objects
    vector<Object3D*> objects;
    //objects.push_back(new Object3D("data/tetrahedron.obj", 15, -35, 515, 55, -20, 205, 1.0, 0.55f, distances));
    objects.push_back(new Object3D("data/eggbox.obj", 15, -0, 500, 195, -10, -20, 1.0, 0.55f, distances));
    //objects.push_back(new Object3D("data/a_second_model.obj", -50, 0, 600, 30, 0, 180, 1.0, 0.55f, distances2));
    
    // create the pose estimator
    PoseEstimator6D* poseEstimator = new PoseEstimator6D(width, height, zNear, zFar, K, distCoeffs, objects);
    
    // active the OpenGL context for the offscreen rendering engine during pose estimation
    RenderingEngine::Instance()->makeCurrent();
    
    int timeout = 0;
    
    bool showHelp = true;
    
    // NETWORKING
    boost::system::error_code error;
    boost::asio::streambuf receive_buffer;
    int port = 27015;
    
    boost::asio::io_context io_cont;
    boost::asio::ip::tcp::acceptor acceptor(io_cont, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
        
    std::cout << "Accept connection at " << port << std::endl;
    const string msg = "Hello From Server!\n";
    
    Matx44f oldPose = objects[0]->getPose(); // to normalize
    boost::asio::ip::tcp::socket sock(io_cont);
    try {
        acceptor.accept(sock);
        
        // RECEIVE frameBytesLength
        boost::asio::read(sock, receive_buffer, boost::asio::transfer_all(), error);
        const int* dataLen = boost::asio::buffer_cast<const int*>(receive_buffer.data());
        
        // SEND 64 bytes (transform mat)
//        std::vector<float> data(oldPose.val);
//        std::vector<float> data(16 * sizeof(float));
//        data = oldPose.val;
//        //boost::asio::buffer buf(data);
//        const size_t bytes = boost::asio::write(sock, boost::asio::buffer(data));
//        boost::asio::write(sock, boost::asio::buffer(msg), error);
//
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    Mat frame;
    bool sel = false;
    int i = 3;
    while(!glfwWindowShouldClose(RenderingEngine::Instance()->getContext()))
    {
        //boost::asio::read(sock, receive_buffer, boost::asio::transfer_exactly(4), error);
        // int?
        if (i++==3) {
            i = 1;
            boost::asio::read(sock, receive_buffer, boost::asio::transfer_all(), error);
            if( error && error != boost::asio::error::eof ) {
                cout << "receive failed: " << error.message() << endl;
    //            frame = stableFrame.clone();
            } else {
                const uchar* mid = boost::asio::buffer_cast<const uchar*>(receive_buffer.data());
                uchar* data = const_cast<uchar*>(mid);
                //delete frame;
                frame = Mat(1408, 792, CV_8UC4, data);
                flip(frame, frame, 1);
                cout << "Check inbox" << endl;
            }
        }
        
        // obtain an input image
        //frame = imread("data/frame_egg.jpg");
        //Mat framefull = imread("data/frame.jpg");
        //resize(framefull, frame, Size(), scale_f, scale_f, INTER_LINEAR);
        
        if (sel) {
            // the main pose uodate call
            poseEstimator->estimatePoses(frame, false, true);
        }
        
        Matx44f deltaPose = objects[0]->getPose()*oldPose.inv();
        
        std::vector<float> poseVec(deltaPose.val, deltaPose.val +16*sizeof(float));
        //boost::asio::streambuf buf(poseVec);
        boost::asio::write( sock, boost::asio::buffer(poseVec) );
        cout << "Server sent Hello message to Client!" << endl;
        
        cout << "Pose:\n" << objects[0]->getPose() << endl;
        
        // render the models with the resulting pose estimates ontop of the input image
        Mat result = drawResultOverlay(objects, frame);
        imshow("result", result);
        
        //stableFrame = frame.clone();
        
        int key = waitKey(timeout);
        
        // start/stop tracking the first object
        if(key == (int)'1')
        {
            poseEstimator->toggleTracking(frame, 0, false);
            poseEstimator->estimatePoses(frame, false, false);
            timeout = 1;
            showHelp = !showHelp;
        }
        if(key == (int)'2') // the same for a second object
        {
            //poseEstimator->toggleTracking(frame, 1, false);
            //poseEstimator->estimatePoses(frame, false, false);
        }
        // reset the system to the initial state
        if(key == (int)'r')
            poseEstimator->reset();
        // stop the demo
        if(key == (int)'c')
        {
            glfwSetWindowShouldClose(RenderingEngine::Instance()->getContext(), true);
//            break;
        }
        glfwSwapBuffers(RenderingEngine::Instance()->getContext());
        glfwPollEvents();
    }
    
    // After chatting close the socket
    sock.close();
    
    // deactivate the offscreen rendering OpenGL context
    RenderingEngine::Instance()->doneCurrent();
    
    // clean up
    RenderingEngine::Instance()->destroy();
    
    for(int i = 0; i < objects.size(); i++)
    {
        delete objects[i];
    }
    objects.clear();
    
    glfwTerminate();
    delete poseEstimator;
}
