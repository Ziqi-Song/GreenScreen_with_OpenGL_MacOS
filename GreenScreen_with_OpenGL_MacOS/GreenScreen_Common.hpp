//
//  GreenScreen_Common.hpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by 宋子奇 on 2025/9/5.
//

#ifndef GreenScreen_Common_h
#define GreenScreen_Common_h

typedef int greenscreen_param_finder_result;
#define PIXELAI_OK                              0  // Run as expected
#define PIXELAI_ERROR_HANDLE                    -1 // Wrong in handle
#define PIXELAI_ERROR_INVALID_HANDLE_PARAM      -2 // Wrong in handle parameters
#define PIXELAI_ERROR_OUT_OF_MEMORY             -3 // Out of memory

typedef void *greenscreen_param_finder_handle;

typedef struct greenscreen_mask {
    unsigned char *data;
    int width;
    int height;
} greenscreen_mask;

typedef struct greenscreen_params {
    int similarity;
    int smoothness;
    int spill;
} greenscreen_params;


#endif /* GreenScreen_Common_h */
