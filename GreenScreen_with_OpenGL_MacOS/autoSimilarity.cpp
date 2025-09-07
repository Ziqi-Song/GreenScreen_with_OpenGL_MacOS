//
//  autoSimilarity.cpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by 宋子奇 on 2025/9/5.
//

#include "autoSimilarity.hpp"
#include <numeric>
#include <algorithm>


void AutoSimilarity::update(cv::Mat mask)
{
    cv::threshold(mask, mask, 127, 1, cv::THRESH_BINARY);
    ratio.push_back((float)cv::sum(mask)[0] / (mask.rows * mask.cols));
}


void AutoSimilarity::update(int width, int height, unsigned char *data)
{
    
}


void AutoSimilarity::process(int &similarity, int &smoothness, int &spill)
{
    int p1 = 0, p2 = 0, p3 = 0;
    
    // Return when ratio is not long enough
    if(ratio.size() < 2) {
        similarity = 0;
        smoothness = 0;
        spill = 0;
        return;
    } else {
        // Generate ratio_d1
        for(int idx = 0; idx < ratio.size(); idx ++)
        {
            ratio_d1.push_back(std::abs(ratio[idx+1] - ratio[idx]));
        }
        
        // Generate ratio_d1_smooth
        int smooth_window = 10;
        for(int idx = 0; idx < ratio_d1.size(); idx++)
        {
            std::vector<float> tmp(ratio_d1.begin() + idx, ratio_d1.begin() + idx + smooth_window);
            double tmp_sum = std::accumulate(std::begin(tmp), std::end(tmp), 0.0);
            double tmp_mean = tmp_sum / tmp.size();
            ratio_d1_smooth.push_back(tmp_mean);
        }
        
        if(ratio.size() < 1) {
            similarity = 0;
            smoothness = 0;
            spill = 0;
            return;
        } else {
            // Find p1
            std::vector<float> tmp1(ratio_d1_smooth.begin(), ratio_d1_smooth.end());
            for(int idx = 0; idx < tmp1.size(); idx++)
            {
                if(tmp1[idx] >= 0.001) {
                    p1 = idx;
                    break;
                }
            }
            // Find p2
            if(p1 >= ratio_d1_smooth.size()) {
                similarity = 0;
                smoothness = 0;
                spill = 0;
                return;
            } else {
                std::vector<float> tmp2(ratio_d1_smooth.begin() + p1, ratio_d1_smooth.end());
                int filter_window = 20;
                for(int idx = 0; idx < tmp2.size() - filter_window; idx++)
                {
                    std::vector<float> tmp(tmp2.begin() + idx, tmp2.begin() + idx + filter_window);
                    if(*max_element(tmp.begin(), tmp.end()) < 0.001){
                        p2 = idx + p1;
                        break;
                    }
                }
                
                // Find p3
                if(p2 + filter_window >= ratio_d1_smooth.size())
                {
                    similarity = 0;
                    smoothness = 0;
                    spill = 0;
                    return;
                } else {
                    std::vector<float> tmp3(ratio_d1_smooth.begin() + p2 + filter_window, ratio_d1_smooth.end());
                    float tmp3_min = tmp3[0];
                    float tmp3_max = *max_element(tmp3.begin(), tmp3.end());
                    for(int idx = 0; idx < tmp3.size(); idx++) {
                        if(tmp3[idx] >= (tmp3_min + (tmp3_max - tmp3_min) * 0.8)) {
                            p3 = idx + p2 + filter_window;
                            break;
                        }
                    }
                    similarity = p2 + (p3 - p2) * 0.8;
                    smoothness = 0;
                    spill = 0;
                }
            }
        }
    }
}
