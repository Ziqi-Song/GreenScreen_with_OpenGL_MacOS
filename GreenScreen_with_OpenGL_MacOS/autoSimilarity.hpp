//
//  autoSimilarity.hpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by 宋子奇 on 2025/9/5.
//

#ifndef autoSimilarity_h
#define autoSimilarity_h

#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

class AutoSimilarity
{
public:
    AutoSimilarity(){
        
    };
    ~AutoSimilarity(){
        
    };
    
    void update(cv::Mat mask);
    void update(int width, int height, unsigned char *data);
    void process(int& similarity, int& smoothness, int& spill);

private:
    std::vector<float> ratio;
    std::vector<float> ratio_d1;
    std::vector<float> ratio_d1_smooth;
};


#endif /* autoSimilarity_h */
