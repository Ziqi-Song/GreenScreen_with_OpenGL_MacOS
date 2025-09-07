//
//  similarity.hpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by 宋子奇 on 2025/9/6.
//

#ifndef similarity_hpp
#define similarity_hpp

#include "GreenScreen_Common.hpp"

greenscreen_param_finder_result greenscreen_param_finder_create(greenscreen_param_finder_handle *handle);
greenscreen_param_finder_result greenscreen_param_finder_update(greenscreen_param_finder_handle handle,
                                                                greenscreen_mask mask);
greenscreen_params greenscreen_param_finder_process(greenscreen_param_finder_handle handle);

#endif /* similarity_hpp */
