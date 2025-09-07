#version 410 core

precision highp float;

in vec2 tex_coord;
layout (location = 0) out vec4 color;

uniform sampler2D tex_camera;  // camera data, BGRA
uniform sampler2D tex_bgimg;   // background image, to replace the green screen, BGRA
uniform sampler2D tex_baseimg; // base image, BGRA
// Algo params
uniform float similarity;
uniform float smoothness;
uniform float spill;
uniform bool search_param;
uniform bool show_camera;
const float gamma = 0.0;
const float contrast = 0.0;
const float brightness = 0.0;
const float opacity = 1.0;
// Key Color
// Note: keycolor should be in the same format with camera data
// E.g. the following blue, magenta are RGBA, same as camera data
vec4 green = vec4(0.0, 255.0, 0.0, 255.0) / 255.0;
vec4 blue = vec4(0.0, 153.0, 255.0, 255.0) / 255.0;
vec4 magenta = vec4(255.0, 0.0, 255.0, 255.0) / 255.0;
// Resolution of camera
#define IMAGEHEIGHT 720
#define IMAGEWIDTH  1280


/* input: [0.0, 1.0], output: [0.0, 1.0] */
vec4 rgba2yuvx(in vec4 rgba)
{
	const mat4 transfer_matrix = mat4(
	    65.52255,    -37.79398,    111.98732,    0.00000,
	    128.62729,   -74.19334,    -93.81088,    0.00000,
	    24.92233,    111.98732,    -18.17644,    0.00000,
	    16.00000,    128.00000,    128.00000,    1.00000
	);
	return transfer_matrix * rgba / 255.0;
}

/* input: [0.0, 1.0], output: [0.0, 1.0] */
vec4 yuvx2rgba(in vec4 yuvx)
{
	yuvx *= 255.0;
	const mat4 transfer_matrix = mat4(
	    0.00456,     0.00456,      0.00456,      0.00000,
	    0.00000,     -0.00153,     0.00791,      0.00000,
	    0.00626,     -0.00319,     0.00000,      0.00000,
	    -0.87416,    0.53133,      -1.08599,     1.00000
	);
	return transfer_matrix * yuvx;
}

// All components are in range [0, 1], including hue.
vec3 rgb2hsv(in vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
	vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// All components are in  range [0, 1], including hue
vec3 hsv2rgb(in vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
	
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float disH(float h)
{
	if(h <= 0.23) 
		return -18.9 * pow(h, 2) + 1.0;
	else if(h > 0.47)
		return -3.56 * pow(h, 2) + 7.12 * h - 2.56;
	else
		return 0.0;
}

float disS(float s)
{
	if(s <= 0.20)
		return -25.0 * pow(s, 2) + 1.0;
	else
		return 0.0;
}

float disV(float v)
{
	if(v <= 0.30)
		return -11.1 * pow(v, 2) + 1.0;
	else
		return 0.0;
}

//vec4 green_yuvx = rgba2yuvx(green);
//vec2 chroma_key = green_yuvx.yz;

/* Calculate the distance between camera color and chromakey color */
float GetChromaDist(in vec2 chroma_key, in vec4 rgba) 
{
	vec4 yuvx = rgba2yuvx(vec4(rgba.rgb, 1.0));
	return distance(chroma_key, yuvx.yz);   // Note: only UV of YUV are used.
}

/* Calculate the boxfiltered distance */
float GetBoxfilteredChromaDist(in vec2 chroma_key, in vec4 input_rgba, vec2 tex_coord)
{
	float dist = GetChromaDist(chroma_key, input_rgba); // center
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(-1)/IMAGEWIDTH, float(-1) / IMAGEHEIGHT))); // top_left
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(0)/IMAGEWIDTH, float(-1) / IMAGEHEIGHT))); // top
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(1)/IMAGEWIDTH, float(-1) / IMAGEHEIGHT))); // top_right
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(-1)/IMAGEWIDTH, float(0) / IMAGEHEIGHT))); // left
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(1)/IMAGEWIDTH, float(0) / IMAGEHEIGHT))); // right
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(-1)/IMAGEWIDTH, float(1) / IMAGEHEIGHT))); // bottom_left
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(0)/IMAGEWIDTH, float(1) / IMAGEHEIGHT))); // bottom
	dist += GetChromaDist(chroma_key, texture(tex_camera, tex_coord + vec2(float(1)/IMAGEWIDTH, float(1) / IMAGEHEIGHT))); // bottom_right
	return dist / 9.0;
}

/* Inhancement: gamma, contrast, brightness */
vec4 CalcColor(in vec4 rgba, in float contrast, in float brightness, in float gamma)
{
	contrast = (contrast < 0.0) ? (1.0 / (-contrast + 1.0)) : (contrast + 1.0);
	brightness *= 0.5;
	gamma = (gamma < 0.0) ? (-gamma + 1.0) : (gamma + 1.0);
	return vec4(pow(rgba.rgb, vec3(gamma, gamma, gamma)) * contrast + brightness, rgba.a);
}

/* Despill by avg */
vec3 despillByAvg(in vec3 rgb)
{
    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;
    // If g > (r + b) / 2 and (g > r) and (g > b), decrease g
    if((g > ((r + b) / 2.0)) && (g - r > 10.0/255.0) && (g - b > 10.0/255.0))
    {
        // Method 1
        // rgb.g = (r + b) / 2.0;
        
        // Method 2
        float desat = (rgb.r * 0.2126 + rgb.g * 0.7152 + rgb.b * 0.0722);
        rgb = vec3(desat, desat, desat);
        return rgb;
    }
    else {
        return rgb;
    }
}

/* ChromaKey pipeline */
vec4 ProcessChromaKey(in vec2 chroma_key, in vec4 rgba, in vec4 bgimg, in vec2 tex_coord)
{
	float chromaDist = GetBoxfilteredChromaDist(chroma_key, rgba, tex_coord);
	float baskMask = chromaDist - similarity;
	float fullMask = pow(clamp((baskMask / smoothness), 0.0, 1.0), 1.5);
	float spillVal = pow(clamp((baskMask / spill), 0.0, 1.0), 1.5);

	rgba.rgba *= opacity;
	fullMask *= opacity;

	float desat = (rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.0722);

	vec3 hsv = rgb2hsv(rgba.rgb);
	float h = hsv.r;

	// Get hue distance
	float dis_h = disH(h);
	// Adjust spill
	spillVal *= dis_h;
	rgba.rgb = clamp(vec3(desat, desat, desat), 0.0, 1.0) * (1.0 - spillVal) + rgba.rgb * spillVal;
	// Enhancement
	vec4 sharp_rgba = CalcColor(rgba, contrast, brightness, gamma);

	if(search_param)
		return vec4(fullMask, fullMask, fullMask, 1.0);
	else
		return sharp_rgba * fullMask + bgimg * (1.0 - fullMask);
}


vec4 ProcessChromaKey(in vec2 chroma_key, in vec2 tex_coord)
{
    // Get camera frame
    vec4 rgba = texture(tex_camera, tex_coord).rgba;
    // Get baseimg
    vec4 baseimg = texture(tex_baseimg, tex_coord).rgba;
    // Get bgimg
    vec4 bgimg = texture(tex_bgimg, tex_coord).rgba;
    
    // Generate soft mask for shadow
    vec4 rgb_diff = abs(rgba - baseimg);
    float softMask = clamp(rgb_diff.r + rgb_diff.g + rgb_diff.b, 0.0, 1.0);
    
    // Generate core mask for foreground subjects
    float chromaDist = GetChromaDist(chroma_key, rgba);
    float baseMask = chromaDist - similarity;
    if((rgba.r > 1.5 * rgba.b) &&
       (rgba.g > 1.5 * rgba.b) &&
       (rgba.r > 100.0 / 255.0) &&
       (rgba.g > 100.0 / 255.0))    // base mask should be adjusted for yellow pixels
    {
        baseMask = 1.0;
    }
    float coreMask = pow(clamp((baseMask / smoothness), 0.0, 1.0), 1.5);
    float spillVal = pow(clamp((baseMask / spill), 0.0, 1.0), 1.5);
    
    // Apply opacity
    rgba *= opacity;
    coreMask *= opacity;
    
    // Adjust spill
    // Method 1
//    float gray = (rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.0722);
//    vec3 hsv = rgb2hsv(rgba.rgb);
//    float h = hsv.r;
//    float dis_h = disH(h);
//    spillVal *= dis_h;
//    rgba.rgb = clamp(vec3(gray, gray, gray), 0.0, 1.0) * (1.0 - spillVal) + rgba.rgb * spillVal;
    // Method 2
    rgba.rgb = despillByAvg(rgba.rgb);
    
    // Inhancement, if needed
    vec4 sharp_rgba = CalcColor(rgba, contrast, brightness, gamma);
    
    // Blend with background image
    float fullMask = clamp(coreMask + softMask, 0.0, 1.0);

    // Keep shadow, use fullMask; Do not keep shadow, use coreMask
    return sharp_rgba * fullMask + bgimg * (1.0 - fullMask);
//    return sharp_rgba * coreMask + bgimg * (1.0 - coreMask);
}

void main(void)
{
    vec2 chroma_key = rgba2yuvx(green).yz;
    color = ProcessChromaKey(chroma_key, tex_coord);
}









