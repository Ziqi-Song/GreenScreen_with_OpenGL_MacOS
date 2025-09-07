//
//  shaderLoader.hpp
//  GreenScreen_with_OpenGL_MacOS
//
//  Created by 宋子奇 on 2025/9/6.
//

#ifndef shaderLoader_hpp
#define shaderLoader_hpp

#pragma once

#include <stdio.h>
#include <GL/glcorearb.h>

namespace shaw {
    class ShaderLoader
    {
    private:
        
    public:
        // Return the compiled shader object. 0 for error.
        static GLuint loadFromFile(const char* file_path, const GLenum shader_type);
        static GLuint linkShaders(GLuint shader, ...);
    };
}

#endif /* shaderLoader_hpp */
