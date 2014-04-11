// render.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#define DEBUG_COALESCE               0
#define DEBUG_SECONDARY_ILLUMINATION 0
#define DEBUG_ORBIT_CACHE            0

//#define DEBUG_HDR
#ifdef DEBUG_HDR
//#define DEBUG_HDR_FILE
//#define DEBUG_HDR_ADAPT
//#define DEBUG_HDR_TONEMAP
#endif
#ifdef DEBUG_HDR_FILE
#include <fstream>
std::ofstream hdrlog;
#define HDR_LOG     hdrlog
#else
#define HDR_LOG     cout
#endif

#ifdef USE_HDR
#define BLUR_PASS_COUNT     2
#define BLUR_SIZE           128
#define DEFAULT_EXPOSURE    -23.35f
#define EXPOSURE_HALFLIFE   0.4f
//#define USE_BLOOM_LISTS
#endif

// #define ENABLE_SELF_SHADOW

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif
#endif /* _WIN32 */

#include <OVR.h>
#undef new
#undef min
#undef max
#include "render.h"
#define  GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/norm.hpp>
#include <GlDebug.h>
#include <GlMethods.h>
#include <GlBuffers.h>
#include <GlFrameBuffer.h>
#include <GlStacks.h>
#include <GlQuery.h>
#include <GlShaders.h>
#include <GlGeometry.h>
#include <GlLighting.h>

extern OVR::Util::Render::StereoConfig ovrStereoConfig;
extern OVR::HMDInfo ovrHmdInfo;

enum StereoEye {
  LEFT_EYE,
  RIGHT_EYE
};

typedef gl::Texture<GL_TEXTURE_2D, GL_RG16F> RiftLookupTexture;
typedef RiftLookupTexture::Ptr RiftLookupTexturePtr;

class RiftDistortionHelper {
public:
  glm::dvec4 K{ 1, 0, 0, 0 };
  glm::dvec4 chromaK{ 1, 0, 1, 0 };
  double lensOffset{ 0 };
  double eyeAspect{ 0.06 };

  double getLensOffset(StereoEye eye) const;
  static glm::dvec2 screenToTexture(const glm::dvec2 & v);
  static glm::dvec2 textureToScreen(const glm::dvec2 & v);
  glm::dvec2 screenToRift(const glm::dvec2 & v, StereoEye eye) const;
  glm::dvec2 riftToScreen(const glm::dvec2 & v, StereoEye eye) const;
  glm::dvec2 textureToRift(const glm::dvec2 & v, StereoEye eye) const;
  glm::dvec2 riftToTexture(const glm::dvec2 & v, StereoEye eye) const;
  double getUndistortionScaleForRadiusSquared(double rSq) const;
  double getUndistortionScale(const glm::dvec2 & v) const;
  double getUndistortionScaleForRadius(double r) const;
  glm::dvec2 getUndistortedPosition(const glm::dvec2 & v) const;
  glm::dvec2 getTextureLookupValue(const glm::dvec2 & texCoord, StereoEye eye) const;
  double getDistortionScaleForRadius(double rTarget) const;
  glm::dvec2 findDistortedVertexPosition(const glm::dvec2 & source, StereoEye eye) const;

public:
  RiftDistortionHelper(const OVR::HMDInfo & hmdInfo);
  RiftLookupTexturePtr createLookupTexture(const glm::uvec2 & lookupTextureSize, StereoEye eye) const;
  gl::GeometryPtr createDistortionMesh(const glm::uvec2 & distortionMeshResolution, StereoEye eye) const;
};


double RiftDistortionHelper::getLensOffset(StereoEye eye) const{
  return (eye == LEFT_EYE) ? -lensOffset : lensOffset;
}

glm::dvec2 RiftDistortionHelper::screenToTexture(const glm::dvec2 & v) {
  return ((v + 1.0) / 2.0);
}

glm::dvec2 RiftDistortionHelper::textureToScreen(const glm::dvec2 & v) {
  return ((v * 2.0) - 1.0);
}

glm::dvec2 RiftDistortionHelper::screenToRift(const glm::dvec2 & v, StereoEye eye) const{
  return glm::dvec2(v.x + getLensOffset(eye), v.y / eyeAspect);
}

glm::dvec2 RiftDistortionHelper::riftToScreen(const glm::dvec2 & v, StereoEye eye) const{
  return glm::dvec2(v.x - getLensOffset(eye), v.y * eyeAspect);
}

glm::dvec2 RiftDistortionHelper::textureToRift(const glm::dvec2 & v, StereoEye eye) const {
  return screenToRift(textureToScreen(v), eye);
}

glm::dvec2 RiftDistortionHelper::riftToTexture(const glm::dvec2 & v, StereoEye eye) const{
  return screenToTexture(riftToScreen(v, eye));
}

double RiftDistortionHelper::getUndistortionScaleForRadiusSquared(double rSq) const {
  double distortionScale = K[0]
    + rSq * (K[1] + rSq * (K[2] + rSq * K[3]));
  return distortionScale;
}

double RiftDistortionHelper::getUndistortionScale(const glm::dvec2 & v) const {
  double rSq = glm::length2(v);
  return getUndistortionScaleForRadiusSquared(rSq);
}

double RiftDistortionHelper::getUndistortionScaleForRadius(double r) const{
  return getUndistortionScaleForRadiusSquared(r * r);
}

glm::dvec2 RiftDistortionHelper::getUndistortedPosition(const glm::dvec2 & v) const {
  return v * getUndistortionScale(v);
}

glm::dvec2 RiftDistortionHelper::getTextureLookupValue(const glm::dvec2 & texCoord, StereoEye eye) const {
  glm::dvec2 riftPos = textureToRift(texCoord, eye);
  glm::dvec2 distorted = getUndistortedPosition(riftPos);
  glm::dvec2 result = riftToTexture(distorted, eye);
  return result;
}

double RiftDistortionHelper::getDistortionScaleForRadius(double rTarget) const {
  double max = rTarget * 2;
  double min = 0;
  double distortionScale;
  while (true) {
    double rSource = ((max - min) / 2.0) + min;
    distortionScale = getUndistortionScaleForRadiusSquared(
      rSource * rSource);
    double rResult = distortionScale * rSource;
    if (glm::epsilonEqual(rResult, rTarget, 1e-6)) {
      break;
    }
    if (rResult < rTarget) {
      min = rSource;
    }
    else {
      max = rSource;
    }
  }
  return 1.0 / distortionScale;
}

glm::dvec2 RiftDistortionHelper::findDistortedVertexPosition(const glm::dvec2 & source,
  StereoEye eye) const {
  const glm::dvec2 rift = screenToRift(source, eye);
  double rTarget = glm::length(rift);
  double distortionScale = getDistortionScaleForRadius(rTarget);
  glm::dvec2 result = rift * distortionScale;
  glm::dvec2 resultScreen = riftToScreen(result, eye);
  return resultScreen;
}

RiftDistortionHelper::RiftDistortionHelper(const OVR::HMDInfo & hmdInfo) {
  OVR::Util::Render::DistortionConfig distortion;
  OVR::Util::Render::StereoConfig stereoConfig;
  stereoConfig.SetHMDInfo(hmdInfo);
  distortion = stereoConfig.GetDistortionConfig();

  // The Rift examples use a post-distortion scale to resize the
  // image upward after distorting it because their K values have
  // been chosen such that they always result in a scale > 1.0, and
  // thus shrink the image.  However, we can correct for that by
  // finding the distortion scale the same way the OVR examples do,
  // and then pre-multiplying the constants by it.
  double postDistortionScale = 1.0  / stereoConfig.GetDistortionScale();
  for (int i = 0; i < 4; ++i) {
    K[i] = distortion.K[i] * postDistortionScale;
  }
  lensOffset = distortion.XCenterOffset;
  eyeAspect = hmdInfo.HScreenSize / 2.0f / hmdInfo.VScreenSize;
}

RiftLookupTexturePtr RiftDistortionHelper::createLookupTexture(const glm::uvec2 & lookupTextureSize, StereoEye eye) const {
  size_t lookupDataSize = lookupTextureSize.x * lookupTextureSize.y * 2;
  float * lookupData = new float[lookupDataSize];
  // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
  glm::dvec2 texCenterOffset = glm::dvec2(0.5f) / glm::dvec2(lookupTextureSize);
  size_t rowSize = lookupTextureSize.x * 2;
  for (size_t y = 0; y < lookupTextureSize.y; ++y) {
    for (size_t x = 0; x < lookupTextureSize.x; ++x) {
      size_t offset = (y * rowSize) + (x * 2);
      glm::dvec2 texCoord = (glm::dvec2(x, y) / glm::dvec2(lookupTextureSize)) + texCenterOffset;
      glm::dvec2 riftCoord = textureToRift(texCoord, eye);
      glm::dvec2 undistortedRiftCoord = getUndistortedPosition(riftCoord);
      glm::dvec2 undistortedTexCoord = riftToTexture(undistortedRiftCoord, eye);
      lookupData[offset] = (float)undistortedTexCoord.x;
      lookupData[offset + 1] = (float)undistortedTexCoord.y;
    }
  }

  RiftLookupTexturePtr outTexture(new RiftLookupTexture());
  outTexture->bind();
  outTexture->image2d(lookupTextureSize, lookupData, 0, GL_RG, GL_FLOAT);
  outTexture->parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  outTexture->parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  outTexture->parameter(GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  outTexture->parameter(GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  delete[] lookupData;
  return outTexture;
}

gl::GeometryPtr RiftDistortionHelper::createDistortionMesh(
  const glm::uvec2 & distortionMeshResolution, StereoEye eye) const{
  std::vector<glm::vec4> vertexData;
  vertexData.reserve(
    distortionMeshResolution.x * distortionMeshResolution.y * 2);
  // The texture coordinates are actually from the center of the pixel, so thats what we need to use for the calculation.
  for (size_t y = 0; y < distortionMeshResolution.y; ++y) {
    for (size_t x = 0; x < distortionMeshResolution.x; ++x) {
      // Create a texture coordinate that goes from [0, 1]
      glm::dvec2 texCoord = (glm::dvec2(x, y)
        / glm::dvec2(distortionMeshResolution - glm::uvec2(1)));
      // Create the vertex coordinate in the range [-1, 1]
      glm::dvec2 vertexPos = (texCoord * 2.0) - 1.0;

      // now find the distorted vertex position from the original
      // scene position
      vertexPos = findDistortedVertexPosition(vertexPos, eye);
      vertexData.push_back(glm::vec4(vertexPos, 0, 1));
      vertexData.push_back(glm::vec4(texCoord, 0, 1));
    }
  }

  std::vector<GLuint> indexData;
  for (size_t y = 0; y < distortionMeshResolution.y - 1; ++y) {
    size_t rowStart = y * distortionMeshResolution.x;
    size_t nextRowStart = rowStart + distortionMeshResolution.x;
    for (size_t x = 0; x < distortionMeshResolution.x; ++x) {
      indexData.push_back(nextRowStart + x);
      indexData.push_back(rowStart + x);
    }
    indexData.push_back(UINT_MAX);

  }
  return gl::GeometryPtr(
    new gl::Geometry(vertexData, indexData, indexData.size(),
    gl::Geometry::Flag::HAS_TEXTURE,
    GL_TRIANGLE_STRIP));
}




static GLuint sceneTextures[2];
static RiftLookupTexturePtr distortionTextures[2];
static StereoEye currentEye;
static gl::ShaderPtr warpVs;
static gl::ShaderPtr warpFs;
static gl::ProgramPtr warp;
static gl::GeometryPtr quadGeometry;

float getProjectionOffset() {
  return ovrStereoConfig.GetProjectionCenterOffset();
}

template <class T>
void for_each_eye(T t) {
  for (int i = 0; i < 2; ++i) {
    currentEye = static_cast<StereoEye>(i);
    t(currentEye);
  }
}

template <class P, class M, class B>
void with_push_matrices(P projection, M modelview, B body) {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  projection();
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix();
  modelview();
  body();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

template <class T>
void with_push_matrices(T body) {
    with_push_matrices([]{}, []{}, body);
};

template <class T>
void with_push_attrib(GLbitfield bits, T body) {
  glPushAttrib(bits);
  body();
  glPopAttrib();
}

double getAspect() {
  return (1 + getProjectionOffset()) * 640.0 / 800.0;
//  return 320.0 / 480.0;
}

using namespace cmod;
using namespace Eigen;
using namespace std;

#define FOV           45.0f
#define NEAR_DIST      0.5f
#define FAR_DIST       1.0e9f

// This should be in the GL headers, but where?
#ifndef GL_COLOR_SUM_EXT
#define GL_COLOR_SUM_EXT 0x8458
#endif

const string WARP_FS = " \
#version 130 \n\
uniform sampler2D Scene; \
uniform sampler2D OffsetMap; \
uniform float DistortionOffset; \
uniform bool LeftEye; \
in vec2 vTexCoord; \
out vec4 vFragColor; \
const vec2 ZERO = vec2(0); \
const vec2 ONE = vec2(1); \
void main() { \
  vec2 undistorted = texture(OffsetMap, vTexCoord).rg; \
  undistorted.x -= undistorted.x * DistortionOffset;\
  if (!LeftEye) { \
    undistorted.x += DistortionOffset;\
  } \
  if (!all(equal(undistorted, \
    clamp(undistorted, ZERO, ONE)))) { \
    discard; \
  } \
  vFragColor = texture(Scene, undistorted); \
}";


const string WARP_VS = "\
#version 130 \n\
uniform mat4 Projection = mat4(1); \
uniform mat4 ModelView = mat4(1); \
in vec4 Position; \
in vec2 TexCoord0; \
out vec2 vTexCoord; \
void main() { \
  gl_Position = Projection * ModelView * Position; \
  vTexCoord = TexCoord0; \
}";

extern OVR::SensorFusion * ovrSensorFusion;

void Renderer::render(const Observer& observer,
  const Universe& universe,
  float faintestMagNight,
  const Selection& sel)
{

  Quaterniond riftOrientation; {
    OVR::Quatf q = ovrSensorFusion->GetPredictedOrientation();
    riftOrientation = Quaterniond(q.w, q.x, q.y, q.z);
  }
  for_each_eye([&](const StereoEye & eye){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    Observer riftObserver(observer);
    riftObserver.setFOV(ovrStereoConfig.GetYFOVRadians());
    Quaterniond orientation = riftObserver.getOrientation();
    riftObserver.setOrientation(riftOrientation.inverse() * orientation);
    //UniversalCoord pos = observer.getPosition();
    //UniversalCoord trackedPos = observer.getTrackedObject().getPosition(observer.getTime());
    //double distance = pos.distanceTo(trackedPos);
    //Vector3d offset(distance / 10.0, 0, 0);
    //offset = observer.getOrientation() * offset;
    //if (eye == LEFT_EYE) {
    //  offset = -1 * offset;
    //}
    //pos = pos + Point3d(offset.x(), offset.y(), offset.z());
    //riftObserver.setPosition(pos);
    renderToTexture(riftObserver, universe, faintestMagNight, sel);
  });


  glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  with_push_attrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT, [&]{
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    with_push_matrices([]{
      glLoadIdentity();
    }, []{
      glLoadIdentity();
    }, [&]{
      for_each_eye([&](const StereoEye & eye){
        glViewport(LEFT_EYE == eye ? 0 : windowWidth / 2, 0, windowWidth / 2, windowHeight);
        warp->use();
        warp->setUniform1i("LeftEye", LEFT_EYE == eye ? 1 : 0);
        glActiveTexture(GL_TEXTURE1);
        distortionTextures[eye]->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTextures[eye]);
        quadGeometry->bindVertexArray();
        quadGeometry->draw();
        glBindTexture(GL_TEXTURE_2D, 0);
        gl::VertexArray::unbind();
        gl::Program::clear();
      });
    });
  });
}


void Renderer::genSceneTexture()
{
  if (!distortionTextures[0]) {
    RiftDistortionHelper helper(ovrHmdInfo);
    for_each_eye([&](StereoEye eye) {
      distortionTextures[eye] = helper.createLookupTexture(glm::uvec2(512, 512), eye);
    });
  }

  if (!warp) {
    warpVs = gl::ShaderPtr(new gl::Shader(GL_VERTEX_SHADER, WARP_VS));
    warpFs = gl::ShaderPtr(new gl::Shader(GL_FRAGMENT_SHADER, WARP_FS));
    warp = gl::ProgramPtr(new gl::Program(warpVs, warpFs));
    // Create the rendering shaders
    warp->use();
    warp->setUniform1i("OffsetMap", 1);
    warp->setUniform1i("Scene", 0);
    warp->setUniform1f("DistortionOffset", getProjectionOffset());
    gl::Program::clear();

    const glm::vec2 min(-1), max(1), texMin(0), texMax(1);
    {
      std::vector<glm::vec4> v;
      v.push_back(glm::vec4(min.x, min.y, 0, 1));
      v.push_back(glm::vec4(texMin.x, texMin.y, 0, 0));
      v.push_back(glm::vec4(max.x, min.y, 0, 1));
      v.push_back(glm::vec4(texMax.x, texMin.y, 0, 1));
      v.push_back(glm::vec4(min.x, max.y, 0, 1));
      v.push_back(glm::vec4(texMin.x, texMax.y, 0, 1));
      v.push_back(glm::vec4(max.x, max.y, 0, 1));
      v.push_back(glm::vec4(texMax.x, texMax.y, 0, 1));

      std::vector<GLuint> i;
      i.push_back(0);
      i.push_back(1);
      i.push_back(2);
      i.push_back(3);
      i.push_back(2);
      i.push_back(1);
      i.push_back(3);
      quadGeometry = gl::GeometryPtr(new gl::Geometry(
        v, i, 2,
        // Buffer has texture coordinates
        gl::Geometry::Flag::HAS_TEXTURE,
        // Indices are for a triangle strip
        GL_TRIANGLES));
    }

  }

  {
    int newSceneTexWidth = windowWidth;
    int newSceneTexHeight = windowHeight;
    if (newSceneTexWidth == sceneTexWidth && newSceneTexHeight == sceneTexHeight) {
      return;
    }
    sceneTexWidth = newSceneTexWidth;
    sceneTexHeight = newSceneTexHeight;
  }

  unsigned int *data;
  data = (unsigned int*)malloc(sceneTexWidth*sceneTexHeight * 4 * sizeof(unsigned int));
  memset(data, 0, sceneTexWidth*sceneTexHeight * 4 * sizeof(unsigned int));

  for_each_eye([&](StereoEye eyeIndex){
    GLuint & sceneTexture = sceneTextures[eyeIndex];
    if (sceneTexture != 0)
      glDeleteTextures(1, &sceneTexture);
    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sceneTexWidth, sceneTexHeight, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, data);
    //      fb.color = gl::Texture2dPtr(new gl::Texture2d(sceneTexture));
    //      fb.init(glm::ivec2(sceneTexWidth, sceneTexHeight));
  });
  free(data);
}


void Renderer::renderToTexture(const Observer& observer,
  const Universe& universe,
  float faintestMagNight,
  const Selection& sel)
{
  GLuint & sceneTexture = sceneTextures[currentEye];
  if (sceneTexture == 0)
    return;
  glPushAttrib(GL_COLOR_BUFFER_BIT);

  draw(observer, universe, faintestMagNight, sel);

  glBindTexture(GL_TEXTURE_2D, sceneTexture);
  glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0,
    sceneTexWidth, sceneTexHeight, 0);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPopAttrib();
}

