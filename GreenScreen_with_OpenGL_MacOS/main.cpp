//
//  main.cpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by Ziqi Song on 2025/9/5.
//

#include <vector>
#include <time.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <unistd.h>

#include <sb7.h>
#include <opencv2/opencv.hpp>

#include "similarity.hpp"
#include "shaderLoader.hpp"
#include "GreenScreen_Common.hpp"

using namespace cv;
using namespace std;


#define VERTEX_SHADER "/Users/songziqi/Documents/Projects/GreenScreen_with_OpenGL_MacOS/GreenScreen_with_OpenGL_MacOS/VertexShader.glsl"
#define FRAGMENT_SHADER "/Users/songziqi/Documents/Projects/GreenScreen_with_OpenGL_MacOS/GreenScreen_with_OpenGL_MacOS/FragmentShader.glsl"
#define TEST_VIDEO "/Users/songziqi/Documents/Projects/GreenScreen_with_OpenGL_MacOS/TestVideos/WithShadow/GS_00005.mp4"
#define BASEIMG "/Users/songziqi/Documents/Projects/GreenScreen_with_OpenGL_MacOS/TestVideos/WithShadow/GS_00005.png"
#define BACKGROUND_VIDEO ""
#define BACKGROUND_IMG ""
#define VIDEO_SAVE_PATH "/Users/songziqi/Documents/Projects/GreenScreen_with_OpenGL_MacOS/TestVideos/output.mp4"
#define IMAGE_SAVE_PATH ""

#define WHITE_BACKGROUND
//#define IMAGE_BACKGROUND
//#define VIDEO_BACKGROUND


/**
Note: You should adjust info.windowWidth and info.windowHeight in sb7.h to the resolution of your input video frames.
 IMAGEWIDTH and IMAGEHEIGHT in FragmentShader.glsl should be set the same as info.windowWidth and info.windowHeight.
*/



struct ImgAndName {
    cv::Mat img;
    string img_name;
};


class my_application : public sb7::application
{
public:
    /**
     Load vertex shader and fragment shader, return glprogram
     */
    GLuint compile_shaders(void)
    {
        // Load vertex shader and fragment shader
        GLuint vertex_shader = shaw::ShaderLoader::loadFromFile(VERTEX_SHADER, GL_VERTEX_SHADER);
        assert(vertex_shader != 0);
        GLuint fragment_shader = shaw::ShaderLoader::loadFromFile(FRAGMENT_SHADER, GL_FRAGMENT_SHADER);
        assert(fragment_shader != 0);
        
        // Compile the shaders to get glprogram
        GLuint program = shaw::ShaderLoader::linkShaders(vertex_shader,
                                                         fragment_shader,
                                                         0);
        assert(program != 0);
        
        // Clean buffer and return
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        
        return program;
    }
    
    
    /**
     
     */
    void startup()
    {
        // Initialize VideoCapture and VideoWriter
        if(!camera_capture.open(TEST_VIDEO)) {
            cout << "Error: <startup> Failed to open camera." << endl;
            return;
        } else {
            camera_frame_width  = camera_capture.get(CAP_PROP_FRAME_WIDTH);
            camera_frame_height = camera_capture.get(CAP_PROP_FRAME_HEIGHT);
            camera_fps          = camera_capture.get(CAP_PROP_FPS);
            cout << "<startup> camera info: " << endl
                 << "frame width = " << camera_frame_width << endl
                 << "frame height = " << camera_frame_height << endl
                 << "fps = " << camera_fps << endl;
        }
        // Open background video
#ifdef VIDEO_BACKGROUND
        if(!background_video_capture.open(BACKGROUND_VIDEO)) {
            cout << "Error: <startup> Failed to open background video. " << BACKGROUND_VIDEO << endl;
            return;
        } else {
            background_video_frame_count = background_video_capture.get(CAP_PROP_FRAME_COUNT);
            background_video_fps         = background_video_capture.get(CAP_PROP_FPS);
            background_video_frame_idx   = 0;
        }
#endif
        // Init video writer
        if(!video_writer.open(VIDEO_SAVE_PATH,
                              cv::VideoWriter::fourcc('m','p','4','v'),
                              camera_fps,
                              cv::Size(camera_frame_width, camera_frame_height),
                              true)) {
            cout << "Error: <startup> Failed to initial video writer." << endl;
            return;
        }
        
        /*-------------- OpenGL Content Init --------------*/
        this->glprogram = compile_shaders();
        
        // Init texture id
        glGenTextures(3, texture_ids);
        // Create vertex array object
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        // Create a vertex buffer object and copy the vertex data to it
        glGenBuffers(1, &quad_vbo);
        
        static const GLfloat quad_data[] =
        {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
            
             0.0f, 1.0f,
             1.0f, 1.0f,
             0.0f, 0.0f,
             1.0f, 0.0f,
        };
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)(8 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        /*-------------- Init parameters --------------*/
        _bIfRecordVideo = true;
        similarity = 385.0;  // [1, 1000], default: 35.0
        smoothness = 80.0;   // [1, 1000], default: 0.001
        spill      = 200.0;  // [1, 1000], default: 0.001
        cout << "Algo init parameters: " << endl
             << "similarity = " << similarity << endl
             << "smoothness = " << smoothness << endl
             << "spill = " << spill << endl;
        
        // Autosearch similarity
        greenscreen_param_finder_create(&handle);
        input_mask.data = new unsigned char[camera_frame_height * camera_frame_width];
        input_mask.height = camera_frame_height;
        input_mask.width = camera_frame_width;
        search_param = false;
        needSaveFrame = false;
        show_camera = true;
        saveBaseImg = false;
        
        // Dev
        base_image = cv::imread(BASEIMG);
//        base_image = cv::Mat(camera_frame_height, camera_frame_width, CV_8UC3, cv::Scalar(0, 0, 0));
    }
    
    
    /**
     Clean buffers and OpenGL context
     */
    void shutdown()
    {
        // Clean videoCapture and videoWriter
        video_writer.release();
        background_video_capture.release();
        camera_capture.release();
        delete[] input_mask.data;
        
        // Clean OpenGL context
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(glprogram);
        glDeleteVertexArrays(1, &vao);
        
        cout << "Quit." << endl;
    
    }
    
    
    /**
     Convert frames from camera and background video to OpenGL textures, then send them into OpenGL context together with algo parameters for rendering.
     */
    void render(double currentTime)
    {
        // Clear window with specified color
        const GLfloat color[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glClearBufferfv(GL_COLOR, 0, color);
        
        // Specify glprogram
        glUseProgram(glprogram);
        
        // Get current frame
        cv::Mat camera_frame;
        if(search_param) {
            if(needSaveFrame) {
                camera_capture >> camera_frame;
                camera_frame.copyTo(param_search_frame);
                needSaveFrame = false;
            } else {
                camera_frame = param_search_frame;
            }
        } else {
            camera_capture >> camera_frame;
            if((camera_frame.cols == 0) ||
               (camera_frame.rows == 0)) {
                video_writer.release();
                return;
            }
            if(saveBaseImg) {
                camera_frame.copyTo(base_image);
                saveBaseImg = false;
            }
        }
        
        // Get background frame
        cv::Mat background_image;
#ifdef WHITE_BACKGROUND
        background_image = cv::Mat(camera_frame_height, camera_frame_width, CV_8UC3, cv::Scalar(255, 255, 255));
#endif
        
#ifdef IMAGE_BACKGROUND
        background_image = cv::imread(BACKGROUND_IMG);
#endif
        
#ifdef VIDEO_BACKGROUND
        background_video_capture >> background_image;
        background_video_frame_idx += 1;
        if(background_video_frame_idx >= background_video_frame_count)
        {
            background_video_capture.set(CAP_PROP_POS_FRAMES, 0);
            background_video_frame_idx = 0;
        }
#endif
        
        if(background_image.size() != cv::Size(camera_frame_width, camera_frame_height)) {
            cv::resize(background_image,
                       background_image,
                       cv::Size(camera_frame_width, camera_frame_height));
        }
        
        // Convert camera frame to GL texture and send into shader
        glActiveTexture(GL_TEXTURE0 + texture_ids[0]);
        texture_camera = texture_ids[0];
        convert_cvMat_to_glTexture(camera_frame, pointer_camera_frame_data, texture_camera);
        glUniform1i(glGetUniformLocation(glprogram, "tex_camera"), texture_camera);
        
        // Convert background video frame to GL texture and send into shader
        glActiveTexture(GL_TEXTURE0 + texture_ids[1]);
        texture_background_image = texture_ids[1];
        convert_cvMat_to_glTexture(background_image, pointer_backgroundImg_data, texture_background_image);
        glUniform1i(glGetUniformLocation(glprogram, "tex_bgimg"), texture_background_image);
        
        // Convert base image to GL texture and send into shader
        glActiveTexture(GL_TEXTURE0 + texture_ids[2]);
        texture_base_image = texture_ids[2];
        convert_cvMat_to_glTexture(base_image, pointer_baseImg_data, texture_base_image);
        glUniform1i(glGetUniformLocation(glprogram, "tex_baseimg"), texture_base_image);
        
        // Send algo parameters to GL context
        if(search_param) {
            glUniform1f(glGetUniformLocation(glprogram, "similarity"), similarity / 1000.0);
            similarity += 1;
        } else {
            glUniform1f(glGetUniformLocation(glprogram, "similarity"), similarity / 1000.0);
        }
        glUniform1i(glGetUniformLocation(glprogram, "search_param"), search_param);
        glUniform1f(glGetUniformLocation(glprogram, "smoothness"), smoothness / 1000.0);
        glUniform1f(glGetUniformLocation(glprogram, "spill"), spill / 1000.0);
        glUniform1f(glGetUniformLocation(glprogram, "show_camera"), show_camera);
        
        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        // Save output video. Must after glDrawArrays.
        if(_bIfRecordVideo) {
            cv::Mat opengl_image = getOpenGLBufferAsCVMat();
            if(opengl_image.size() != cv::Size(camera_frame_height, camera_frame_width)) {
                cv::resize(opengl_image, opengl_image, cv::Size(camera_frame_height, camera_frame_width));
            }
            video_writer << opengl_image;
        }
        
        if(search_param) {
            if(similarity < 500) {
                cv::Mat mask = getOpenGLBufferAsCVMat();
                if(mask.size() != cv::Size(camera_frame_height, camera_frame_width)) {
                    cv::resize(mask, mask, cv::Size(camera_frame_height, camera_frame_width));
                }
                cv::cvtColor(mask, mask, cv::COLOR_BGR2GRAY);
                memcpy(input_mask.data, mask.data, mask.rows * mask.cols);
                greenscreen_param_finder_update(handle, input_mask);
            } else {
                greenscreen_params result = greenscreen_param_finder_process(handle);
                similarity = result.similarity;
                cout << "Parameter search result" << endl
                     << "similarity = " << similarity << endl;
                search_param = false;
            }
        }
    }
    
    
    
    /**
     Convert cv::Mat to GL texture
     */
    void convert_cvMat_to_glTexture(cv::Mat srcImg, GLubyte* pointer_image_data, GLuint texture_id)
    {
        // Get image pointer and copy image data
        int srcImg_width = srcImg.cols;
        int srcImg_height = srcImg.rows;
        int srcImg_channels = srcImg.channels();
        
        long pixellength = srcImg_width * srcImg_height * srcImg_channels;
        pointer_image_data = new GLubyte[pixellength];
        
        memcpy(pointer_image_data, srcImg.data, pixellength * sizeof(char));
        
        // Set texture as 2D
        glBindTexture(GL_TEXTURE_2D, texture_id);
        // Use linear interpolation for texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Use repeat for vertical expand
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Use clamp to edge for vertical expand
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Use image data in memory as texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, srcImg_width, srcImg_height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, pointer_image_data);
        free(pointer_image_data);
    }
    
    
    /**
     Extract OpenGL texture to cv::Mat
     */
    cv::Mat getOpenGLBufferAsCVMat()
    {
        int vPort[4];
        glGetIntegerv(GL_VIEWPORT, vPort);
        
        cv::Mat_<cv::Vec3b> opengl_image(vPort[3], vPort[2]);
        {
            cv::Mat_<cv::Vec4b> opengl_image_4b(vPort[3], vPort[2]);
            glReadPixels(0, 0, vPort[2], vPort[3], GL_BGRA, GL_UNSIGNED_BYTE, opengl_image_4b.data);
            cv::flip(opengl_image_4b, opengl_image_4b, 0);
            cv::mixChannels(&opengl_image_4b, 1, &opengl_image, 1, &(cv::Vec6i(0,0,1,1,2,2)[0]), 3);
        }
        return opengl_image;
    }
    
    
    /**
     Keyboard
     */
    void onKey(int key, int action)
    {
        // Adjust similarity
        if(key == 265 && action == GLFW_PRESS) // press up arrow
        {
            similarity = similarity + 1;
            if(similarity > 1000.0){
                similarity = 1000.0;
            }
            cout << "similarity = " << similarity << endl;
        }
        if(key == 264 && action == GLFW_PRESS) // press down arrow
        {
            similarity = similarity - 1;
            if(similarity < 1.0){
                similarity = 1.0;
            }
            cout << "similarity = " << similarity << endl;
        }
        
        // Save image
        if(key == 73 && action == GLFW_PRESS) // press 'i'
        {
            cv::Mat opengl_image = getOpenGLBufferAsCVMat();
            cv::imwrite(IMAGE_SAVE_PATH, opengl_image);
            saveBaseImg = true;
        }
        
        // Save video
        if(key == 86 && action == GLFW_PRESS) // press 'v', press once to start saving, press again to pause saving
        {
            if(_bIfRecordVideo == true) {
                _bIfRecordVideo = false;
                cout << "Video recording stop." << endl;
            } else if(_bIfRecordVideo == false) {
                _bIfRecordVideo = true;
                cout << "Video recording start." << endl;
            }
        }
        
        // Adjust smoothness
        if(key == 79 && action == GLFW_PRESS) // press 'o'
        {
            smoothness = smoothness + 1;
            if(smoothness > 1000.0){
                smoothness = 1000.0;
            }
            cout << "smoothness = " << smoothness << endl;
        }
        if(key == 80 && action == GLFW_PRESS) // press 'p'
        {
            smoothness = smoothness - 1;
            if(smoothness <= 1.0){
                smoothness = 1.0;
            }
            cout << "smoothness = " << smoothness << endl;
        }
        
        // Adjust spill
        if(key == 75 && action == GLFW_PRESS) // press 'k'
        {
            spill = spill + 1;
            if(spill > 1000.0){
                spill = 1000.0;
            }
            cout << "spill = " << spill << endl;
        }
        if(key == 76 && action == GLFW_PRESS) // press 'l'
        {
            spill = spill - 1;
            if(spill <= 1.0){
                spill = 1.0;
            }
            cout << "spill = " << spill << endl;
        }
        
        // Trigger parameters search
        if(key == 83 && action == GLFW_PRESS) // press 's'
        {
            search_param = true;
            needSaveFrame = true;
            cout << "Search start." << endl;
        }
        
        // Switch view
        if(key == 67 && action == GLFW_PRESS) // press 'c'
        {
            show_camera = !show_camera;
            cout << "View switched." << endl;
        }
    }
    
private:
    GLuint glprogram;
    GLuint vao;
    GLuint quad_vbo;
    
    // OpenGL textures
    GLuint texture_camera;
    GLuint texture_background_image;
    GLuint texture_base_image;
    GLuint texture_ids[3];
    // Algorithm parameters
    float similarity;
    float smoothness;
    float spill;
    // Image pointer
    GLubyte* pointer_camera_frame_data;
    GLubyte* pointer_backgroundImg_data;
    GLubyte* pointer_baseImg_data;
    // Camera
    cv::VideoCapture camera_capture;
    cv::VideoCapture background_video_capture;
    cv::VideoWriter video_writer;
    
    int camera_frame_width;
    int camera_frame_height;
    double camera_fps;
    
    double background_video_fps;
    double background_video_frame_count;
    int background_video_frame_idx;
    
    bool _bIfRecordVideo;
    
    vector<string> person_filenames;
    vector<string> greenscreen_filenames;
    
    greenscreen_param_finder_handle handle;
    greenscreen_mask input_mask;
    bool search_param;
    bool needSaveFrame;
    bool show_camera;
    bool saveBaseImg;
    cv::Mat param_search_frame;
    cv::Mat base_image;
};

DECLARE_MAIN(my_application);

//int main(int argc, const char * argv[]) {
//    Mat img = imread("/Users/songziqi/Documents/20140913_163906.jpg");
//    imshow("Img", img);
//    cv::waitKey(0);
//    std::cout << "Hello, World!\n";
//    return 0;
//}
