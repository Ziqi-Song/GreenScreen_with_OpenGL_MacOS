//
//  similarity.cpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by 宋子奇 on 2025/9/6.
//

#include "similarity.hpp"
#include "autoSimilarity.hpp"


greenscreen_param_finder_result greenscreen_param_finder_create(greenscreen_param_finder_handle *handle)
{
    AutoSimilarity* pAutoSimilarity = new AutoSimilarity();
    if(pAutoSimilarity)
    {
        *handle = pAutoSimilarity;
    } else {
        delete pAutoSimilarity;
        *handle = NULL;
        return PIXELAI_ERROR_HANDLE;
    }
    return PIXELAI_OK;
}


greenscreen_param_finder_result greenscreen_param_finder_update(greenscreen_param_finder_handle handle,
                                                                greenscreen_mask mask)
{
    if(handle)
    {
        cv::Mat input(mask.height, mask.width, CV_8UC1, mask.data);
        ((AutoSimilarity*) handle)->update(input);
    } else {
        return PIXELAI_ERROR_HANDLE;
    }
    return PIXELAI_OK;
}


greenscreen_params greenscreen_param_finder_process(greenscreen_param_finder_handle handle)
{
    int similarity = 0, smoothness = 0, spill = 0;
    ((AutoSimilarity*) handle)->process(similarity, smoothness, spill);
    
    greenscreen_params result;
    result.smoothness = smoothness;
    result.similarity = similarity;
    result.spill = spill;
    
    return result;
}
