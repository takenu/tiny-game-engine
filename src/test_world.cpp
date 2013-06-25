/*
Copyright 2012, Bas Fagginger Auer.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <vector>
#include <string>
#include <exception>

#include <config.h>

#include <tiny/os/application.h>
#include <tiny/os/sdlapplication.h>

#include <tiny/img/io/image.h>
#include <tiny/mesh/io/staticmesh.h>

#include <tiny/draw/worldrenderer.h>
#include <tiny/draw/screensquare.h>
#include <tiny/draw/staticmesh.h>
#include <tiny/draw/staticmeshhorde.h>
#include <tiny/draw/texture2d.h>
#include <tiny/draw/icontexture2d.h>
#include <tiny/draw/iconhorde.h>
#include <tiny/draw/lighthorde.h>
#include <tiny/draw/terrain.h>

#include <tiny/draw/effects/diffuse.h>

#include <tiny/draw/heightmap/normalmap.h>
#include <tiny/draw/heightmap/scale.h>
#include <tiny/draw/heightmap/resize.h>
#include <tiny/draw/heightmap/diamondsquare.h>
#include <tiny/draw/heightmap/heighttocolour.h>

using namespace std;
using namespace tiny;

os::Application *application = 0;

draw::WorldRenderer *worldRenderer = 0;

draw::StaticMesh *testMesh = 0;
draw::StaticMeshHorde *testMeshes = 0;
std::vector<draw::StaticMeshInstance> testMeshesInstances;
draw::RGBATexture2D *testDiffuseTexture = 0;

draw::StaticMesh *skyBox = 0;
draw::RGBATexture2D *skyTexture = 0;

const vec2 terrainScale = vec2(1.0f, 1.0f);
const float terrainHeightScale = 512.0f;
const ivec2 terrainFarScale = ivec2(16, 16);
const vec2 terrainFarOffset = vec2(0.5f, 0.5f);
draw::FloatTexture2D *terrainHeightTexture = 0;
draw::FloatTexture2D *terrainFarHeightTexture = 0;
draw::RGBTexture2D *terrainNormalTexture = 0;
draw::RGBTexture2D *terrainFarNormalTexture = 0;
draw::RGBTexture2D *terrainDiffuseTexture = 0;
draw::Terrain *terrain = 0;

std::vector<draw::PointLightInstance> pointLightInstances;
draw::PointLightHorde *pointLights = 0;

draw::ScreenIconHorde *font = 0;
draw::IconTexture2D *fontTexture = 0;
draw::WorldIconHorde *worldFont = 0;

vec3 cameraPosition = vec3(0.0f, 0.0f, 0.0f);
vec4 cameraOrientation = vec4(0.0f, 0.0f, 0.0f, 1.0f);

double globalTime = 0.0;

class SimpleFogEffect : public tiny::draw::ScreenFillingSquare
{
    public:
        SimpleFogEffect();
        ~SimpleFogEffect();
        
        template <typename TextureType>
        void setSkyTexture(const TextureType &texture)
        {
            const size_t width = texture.getWidth();
                
            if (width > 0)
            {
                skyColours.resize(width);
                
                for (size_t i = 0; i < width; ++i)
                {
                    skyColours[i] = texture(i, 0);
                }
            }
        }
        
        std::string getFragmentShaderCode() const;
        
        void setSun(const vec3 &);
        void setFog(const float &);
        
    private:
        vec3 sun;
        vector<vec3> skyColours;
};

SimpleFogEffect::SimpleFogEffect() :
    ScreenFillingSquare()
{
    skyColours.push_back(vec3(1.0f, 1.0f, 1.0f));
    setSun(vec3(0.7f, 0.7f, 0.0f));
    setFog(8.0f);
}

SimpleFogEffect::~SimpleFogEffect()
{

}

std::string SimpleFogEffect::getFragmentShaderCode() const
{
    return std::string(
"#version 150\n"
"\n"
"precision highp float;\n"
"\n"
"uniform sampler2D diffuseTexture;\n"
"uniform sampler2D worldNormalTexture;\n"
"uniform sampler2D worldPositionTexture;\n"
"\n"
"uniform vec3 cameraPosition;\n"
"uniform vec3 sunDirection;\n"
"uniform vec3 fogFalloff;\n"
"uniform vec2 inverseScreenSize;\n"
"uniform vec3 skyColour;\n"
"\n"
"out vec4 colour;\n"
"\n"
"void main(void)\n"
"{\n"
"   vec2 tex = gl_FragCoord.xy*inverseScreenSize;\n"
"   vec4 diffuse = texture(diffuseTexture, tex);\n"
"   vec4 worldNormal = texture(worldNormalTexture, tex);\n"
"   vec4 worldPosition = texture(worldPositionTexture, tex);\n"
"   \n"
"   vec3 relativePosition = normalize(worldPosition.xyz - cameraPosition);\n"
"   vec3 sunContribution = max(0.0f, sunDirection.y)*vec3(800.0f, 500.0f, 0.0f)*max(0.0f, dot(relativePosition, sunDirection) - 0.999f);\n"
"   \n"
"   float depth = worldPosition.w;\n"
"   float directLight = 0.25f + 0.75f*max(dot(worldNormal.xyz, sunDirection), 0.0f);\n"
"   vec3 decay = vec3(exp(depth*fogFalloff));\n"
"   \n"
"   colour = vec4(diffuse.xyz*directLight*decay + (vec3(1.0f) - decay)*skyColour + sunContribution, 1.0f);\n"
"   //colour = vec4(diffuse.xyz, 1.0f);\n"
"   //colour = vec4(1.0f, 0.0f, 0.0f, 1.0f);\n"
"}\n");
}

void SimpleFogEffect::setSun(const vec3 &sunDirection)
{
    sun = normalize(sunDirection);
    uniformMap.setVec3Uniform(sun, "sunDirection");
    
    uniformMap.setVec3Uniform(skyColours[static_cast<size_t>(floor((0.5f - 0.49f*sun.y)*static_cast<float>(skyColours.size())))], "skyColour");
}

void SimpleFogEffect::setFog(const float &fogIntensity)
{
    uniformMap.setVec3Uniform(-fogIntensity*vec3(5.8e-6 + 2.0e-5, 13.5e-6 + 2.0e-5, 33.1e-6 + 2.0e-5), "fogFalloff");
}

SimpleFogEffect *fogEffect = 0;
draw::Renderable *screenEffect = 0;

void setup()
{
    const float aspectRatio = static_cast<float>(application->getScreenWidth())/static_cast<float>(application->getScreenHeight());
    
    //testMesh = new draw::StaticMesh(mesh::io::readStaticMeshOBJ(DATA_DIRECTORY + "mesh/sponza/sponza_triangles.obj"));
    //testMesh = new draw::StaticMesh(mesh::io::readStaticMeshOBJ(DATA_DIRECTORY + "mesh/sibenik/sibenik_triangles.obj"));
    testMesh = new draw::StaticMesh(mesh::StaticMesh::createCubeMesh(0.5f));
    testDiffuseTexture = new draw::RGBATexture2D(img::Image::createTestImage());
    testMesh->setDiffuseTexture(*testDiffuseTexture);
    
    testMeshes = new draw::StaticMeshHorde(mesh::StaticMesh::createCubeMesh(0.25f), 1024);
    testMeshes->setDiffuseTexture(*testDiffuseTexture);
    
    const float meshSpacing = 1.0f;
    
    for (int i = -4; i <= 4; ++i)
    {
        for (int j = -4; j <= 4; ++j)
        {
            for (int k = -4; k <= 4; ++k)
            {
                testMeshesInstances.push_back(draw::StaticMeshInstance(vec4(meshSpacing*i, meshSpacing*j, meshSpacing*k, 1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f)));
            }
        }
    }
    
    testMeshes->setMeshes(testMeshesInstances.begin(), testMeshesInstances.end());
    
    //Create sky.
    skyBox = new draw::StaticMesh(mesh::StaticMesh::createCubeMesh(-1.0e5));
    skyTexture = new draw::RGBATexture2D(img::io::readImage(DATA_DIRECTORY + "img/sky.png"));
    
    //Create terrain.
    terrainHeightTexture = new draw::FloatTexture2D(img::io::readImage(DATA_DIRECTORY + "img/tasmania.png"));
    terrainFarHeightTexture = new draw::FloatTexture2D(terrainHeightTexture->getWidth(), terrainHeightTexture->getHeight());
    terrainNormalTexture = new draw::RGBTexture2D(terrainHeightTexture->getWidth(), terrainHeightTexture->getHeight());
    terrainFarNormalTexture = new draw::RGBTexture2D(terrainHeightTexture->getWidth(), terrainHeightTexture->getHeight());
    //terrainDiffuseTexture = new draw::RGBTexture2D(img::io::readImage(DATA_DIRECTORY + "img/default.png"));
    terrainDiffuseTexture = new draw::RGBTexture2D(terrainHeightTexture->getWidth(), terrainHeightTexture->getHeight());
    
    draw::computeScaledTexture(*terrainHeightTexture, *terrainFarHeightTexture, vec4(terrainHeightScale/255.0f), vec4(0.0f));
    draw::computeNormalMap(*terrainFarHeightTexture, *terrainFarNormalTexture, terrainScale.x*terrainFarScale.x);
    draw::computeColourFromHeight(*terrainFarHeightTexture, *terrainDiffuseTexture, terrainScale.x*terrainFarScale.x);
    
    terrain = new draw::Terrain(6, 8);
    
    if (terrainFarScale.x > 1 || terrainFarScale.y > 1)
    {
        //Create far-away terrain.
        draw::computeResizedTexture(*terrainFarHeightTexture, *terrainHeightTexture, vec2(1.0f/static_cast<float>(terrainFarScale.x), 1.0f/static_cast<float>(terrainFarScale.y)), terrainFarOffset);
        draw::computeDiamondSquareRefinement(*terrainHeightTexture, *terrainHeightTexture, terrainFarScale.x);
        draw::computeNormalMap(*terrainHeightTexture, *terrainNormalTexture, terrainScale.x);
        terrain->setFarTextures(*terrainHeightTexture, *terrainFarHeightTexture,
                                *terrainNormalTexture, *terrainFarNormalTexture,
                                *terrainDiffuseTexture,
                                terrainScale, terrainFarScale, terrainFarOffset);
    }
    else
    {
        terrain->setTextures(*terrainFarHeightTexture, *terrainFarNormalTexture, *terrainDiffuseTexture, terrainScale);
    }
    
    const float lightSpacing = 4.0f;
    
    for (int i = -4; i <= 4; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            pointLightInstances.push_back(draw::PointLightInstance(vec4(lightSpacing*i, 0.0f, lightSpacing*j, 1.0f), vec4(1.0f, 0.9f, 0.3f, lightSpacing)));
        }
    }
    
    pointLights = new draw::PointLightHorde(pointLightInstances.size());
    
    fontTexture = new draw::IconTexture2D(512, 512);
    fontTexture->packIcons(img::io::readFont(DATA_DIRECTORY + "font/OpenBaskerville-0.0.75.ttf", 48));
    
    font = new draw::ScreenIconHorde(1024);
    font->setIconTexture(*fontTexture);
    font->setText(-1.0, -1.0, 0.1, aspectRatio, "The \\rtiny\\w-\\ggame\\w-\\bengine\\w.\nA world rendering example.", *fontTexture);
    
    worldFont = new draw::WorldIconHorde(1024);
    worldFont->setIconTexture(*fontTexture);
    worldFont->setText(0.0, 0.0, 1.0, "The \\rtiny\\w-\\ggame\\w-\\bengine\\w.\nA world rendering example.", *fontTexture);
    
    fogEffect = new SimpleFogEffect();
    fogEffect->setSkyTexture(*skyTexture);
    
    screenEffect = new draw::effects::Diffuse();
    
    worldRenderer = new draw::WorldRenderer(application->getScreenWidth(), application->getScreenHeight());
    
    worldRenderer->addWorldRenderable(skyBox);
    worldRenderer->addWorldRenderable(testMesh);
    worldRenderer->addWorldRenderable(testMeshes);
    worldRenderer->addWorldRenderable(terrain);
    worldRenderer->addWorldRenderable(worldFont);
    
    //worldRenderer->addScreenRenderable(fogEffect, false, false);
    worldRenderer->addScreenRenderable(screenEffect, false, false);
    worldRenderer->addScreenRenderable(pointLights, false, false, draw::BlendAdd);
    worldRenderer->addScreenRenderable(font, false, false, draw::BlendMix);
}

void cleanup()
{
    delete worldRenderer;
    
    delete screenEffect;
    delete fogEffect;
    
    delete worldFont;
    delete font;
    delete fontTexture;
    
    delete pointLights;
    
    delete terrain;
    delete terrainDiffuseTexture;
    delete terrainNormalTexture;
    delete terrainFarNormalTexture;
    delete terrainHeightTexture;
    delete terrainFarHeightTexture;
    
    delete skyTexture;
    delete skyBox;
    
    delete testMeshes;
    delete testMesh;
    delete testDiffuseTexture;
}

void update(const double &dt)
{
    application->updateSimpleCamera(dt, cameraPosition, cameraOrientation);
    
    terrain->setCameraPosition(cameraPosition);
    worldRenderer->setCamera(cameraPosition, cameraOrientation);
    
    fogEffect->setSun(vec3(cos(0.5f*globalTime), 1.0f, sin(0.5f*globalTime)));
    
    for (std::vector<draw::PointLightInstance>::iterator i = pointLightInstances.begin(); i != pointLightInstances.end(); ++i)
    {
        i->position += randomVec4(2.0f*dt);
    }
    
    pointLights->setLights(pointLightInstances.begin(), pointLightInstances.end());
    
    globalTime += dt;
}

void render()
{
    worldRenderer->clearTargets();
    worldRenderer->render();
}

int main(int, char **)
{
    try
    {
        application = new os::SDLApplication(SCREEN_WIDTH, SCREEN_HEIGHT);
        setup();
    }
    catch (std::exception &e)
    {
        cerr << "Unable to start application!" << endl;
        return -1;
    }
    
    while (application->isRunning())
    {
        update(application->pollEvents());
        render();
        application->paint();
    }
    
    cleanup();
    delete application;
    
    cerr << "Goodbye." << endl;
    
    return 0;
}

