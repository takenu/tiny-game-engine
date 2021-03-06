/*
Copyright 2012-2015, Bas Fagginger Auer and Matthijs van Dorp.

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
#include <algorithm>

#include <tiny/hash/md5.h>
#include <tiny/draw/renderer.h>

using namespace tiny::draw;

detail::BoundProgram::BoundProgram(const std::string &a_vertexShaderCode, const std::string &a_geometryShaderCode, const std::string &a_fragmentShaderCode) :
    hash(tiny::hash::md5hash(a_vertexShaderCode + a_geometryShaderCode + a_fragmentShaderCode)),
    vertexShader(0),
    geometryShader(0),
    fragmentShader(0),
    program(0),
    vertexShaderCode(a_vertexShaderCode),
    geometryShaderCode(a_geometryShaderCode),
    fragmentShaderCode(a_fragmentShaderCode),
    renderableIndices()
{

}

detail::BoundProgram::~BoundProgram()
{
    if (program) delete program;
    if (vertexShader) delete vertexShader;
    if (geometryShader) delete geometryShader;
    if (fragmentShader) delete fragmentShader;
}

void detail::BoundProgram::bind() const
{
    assert(program);
    program->bind();
}

void detail::BoundProgram::unbind() const
{
    assert(program);
    program->unbind();
}

void detail::BoundProgram::bindRenderTarget(const unsigned int &index, const std::string &name) const
{
    assert(program);
    GL_CHECK(glBindFragDataLocation(program->getIndex(), index, name.c_str()));
}

void detail::BoundProgram::setUniforms(const UniformMap &map) const
{
    assert(program);
    map.setUniformsInProgram(*program);
}

void detail::BoundProgram::setUniformsAndTextures(const UniformMap &map, const int &textureOffset) const
{
    assert(program);
    map.setUniformsAndTexturesInProgram(*program, textureOffset);
}

const ShaderProgram &detail::BoundProgram::getProgram() const
{
    assert(program);
    return *program;
}

void detail::BoundProgram::compile()
{
    //Create shaders.
    if (program) delete program;
    if (vertexShader) delete vertexShader;
    if (geometryShader) delete geometryShader;
    if (fragmentShader) delete fragmentShader;
    
    vertexShader = new VertexShader();
    geometryShader = new GeometryShader();
    fragmentShader = new FragmentShader();
    program = new ShaderProgram();
    
    //Compile shaders.
    vertexShader->compile(vertexShaderCode);
    
    if (geometryShaderCode.empty())
    {
        //If the renderable has no geometry shader, remove it.
        delete geometryShader;
        
        geometryShader = 0;
    }
    else
    {
        geometryShader->compile(geometryShaderCode);
    }
    
    fragmentShader->compile(fragmentShaderCode);
    
    //Attach shaders.
    program->attach(*vertexShader);
    if (geometryShader) program->attach(*geometryShader);
    program->attach(*fragmentShader);
}

void detail::BoundProgram::link()
{
    assert(program);
    program->link();
}

bool detail::BoundProgram::validate() const
{
    if (program) return program->validate();
    
    return false;
}

void detail::BoundProgram::addRenderableIndex(const unsigned int &renderableIndex)
{
    assert(renderableIndices.count(renderableIndex) == 0);
    
    renderableIndices.insert(renderableIndex);
}

void detail::BoundProgram::freeRenderableIndex(const unsigned int &renderableIndex)
{
    renderableIndices.erase(renderableIndex);
}

unsigned int detail::BoundProgram::numRenderables() const
{
    return renderableIndices.size();
}

Renderer::Renderer() :
    frameBufferIndex(0),
    renderTargetNames(),
    renderTargetTextures(),
    depthTargetTexture(0),
    viewportSize(0, 0)
{
    
}

Renderer::Renderer(const Renderer &) :
    frameBufferIndex(0),
    renderTargetNames(),
    renderTargetTextures(),
    depthTargetTexture(0)
{
    
}

Renderer::~Renderer()
{
    //Free allocated bound renderables.
    for (std::map<unsigned int, detail::BoundRenderable *>::iterator i = renderables.begin(); i != renderables.end(); ++i)
    {
        delete i->second;
    }
    
    //Free allocated renderable programs.
    for (std::map<unsigned int, detail::BoundProgram *>::iterator i = shaderPrograms.begin(); i != shaderPrograms.end(); ++i)
    {
        delete i->second;
    }
    
    destroyFrameBuffer();
}

void Renderer::createFrameBuffer()
{
    //Do not create the frame buffer if it already exists.
    if (frameBufferIndex != 0) return;
    
    GL_CHECK(glGenFramebuffers(1, &frameBufferIndex));
    
    if (frameBufferIndex == 0)
        throw std::bad_alloc();
}

void Renderer::destroyFrameBuffer()
{
    renderTargetNames.clear();
    renderTargetTextures.clear();
    
    if (frameBufferIndex != 0)
        GL_CHECK(glDeleteFramebuffers(1, &frameBufferIndex));
    
    frameBufferIndex = 0;
}

void Renderer::addRenderable(const unsigned int &renderableIndex, Renderable *renderable, const bool &readFromDepthTexture, const bool &writeToDepthTexture, const BlendMode &blendMode, const CullMode &cullMode)
{
    assert(renderable);
    
    //Verify that this index is not yet in use.
    std::map<unsigned int, detail::BoundRenderable *>::iterator j = renderables.find(renderableIndex);
    
    if (j != renderables.end())
    {
        std::cerr << "A renderable with index " << renderableIndex << " has already been added to the renderer!" << std::endl;
        throw std::exception();
    }
    
    detail::BoundProgram *shaderProgram = new detail::BoundProgram(renderable->getVertexShaderCode(), renderable->getGeometryShaderCode(), renderable->getFragmentShaderCode());
    std::map<unsigned int, detail::BoundProgram *>::iterator k = shaderPrograms.find(shaderProgram->hash);

    //Lock texture uniforms to prevent the bindings from changing.
    uniformMap.lockTextures();
    renderable->uniformMap.lockTextures();
        
    //Has this program already been compiled earlier?
    if (k == shaderPrograms.end())
    {
        //If not, then add it to the list of programs.
        shaderProgram->compile();
        
        //Set program outputs.
        for (size_t i = 0; i < renderTargetNames.size(); ++i)
        {
            std::cerr << "Bound '" << renderTargetNames[i].c_str() << "' to colour number " << i << " for program " << shaderProgram->getProgram().getIndex() << "." << std::endl;
            shaderProgram->bindRenderTarget(i, renderTargetNames[i]);
        }
        
        shaderProgram->link();
        
        //Bind uniforms and textures to program.
        shaderProgram->bind();
        shaderProgram->setUniformsAndTextures(uniformMap);
        shaderProgram->setUniformsAndTextures(renderable->uniformMap, uniformMap.getNrTextures());
        shaderProgram->unbind();
    
#ifndef NDEBUG
        if (!shaderProgram->validate())
        {
            std::cerr << "Unable to validate program!" << std::endl;
            throw std::exception();
        }
#endif
        
        k = shaderPrograms.insert(std::make_pair(shaderProgram->hash, shaderProgram)).first;
    }
    else
    {
        delete shaderProgram;
    }
    
    k->second->addRenderableIndex(renderableIndex);
    renderables.insert(std::make_pair(renderableIndex, new detail::BoundRenderable(renderable, k->first, readFromDepthTexture, writeToDepthTexture, blendMode, cullMode)));
}

bool Renderer::freeRenderable(const unsigned int &renderableIndex)
{
    std::map<unsigned int, detail::BoundRenderable *>::iterator j = renderables.find(renderableIndex);
    
    if (j == renderables.end())
    {
        std::cerr << "Unable to find renderable with index " << renderableIndex << "!" << std::endl;
        return false;
    }
    
    detail::BoundRenderable *renderable = j->second;
    std::map<unsigned int, detail::BoundProgram *>::iterator k = shaderPrograms.find(renderable->shaderProgramHash);
    
    assert(k != shaderPrograms.end());
    k->second->freeRenderableIndex(renderableIndex);
    
    //Remove renderable.
    delete renderable;
    renderables.erase(j);
    
    //Remove shader program if it is no longer referenced by any renderable.
    if (k->second->numRenderables() == 0)
    {
        delete k->second;
        shaderPrograms.erase(k);
    }

    return true;
}

void Renderer::addRenderTarget(const std::string &name)
{
    if (std::find(renderTargetNames.begin(), renderTargetNames.end(), name) != renderTargetNames.end())
    {
        std::cerr << "Warning: render target '" << name << "' already exists!" << std::endl;
        return;
    }
    
    renderTargetNames.push_back(name);
    renderTargetTextures.push_back(0);
    
    if (renderTargetNames.size() >= GL_MAX_DRAW_BUFFERS)
    {
        std::cerr << "Warning: binding more than the maximum number (" << GL_MAX_DRAW_BUFFERS << ") of draw buffers!" << std::endl;
    }
}

void Renderer::updateRenderTargets()
{
    if (frameBufferIndex == 0)
        createFrameBuffer();
    
    std::vector<GLenum> drawBuffers;
    
    drawBuffers.reserve(renderTargetTextures.size() + 1);
    
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferIndex));
    
    for (size_t i = 0; i < renderTargetTextures.size(); ++i)
    {
        if (renderTargetTextures[i] != 0)
        {
            GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, renderTargetTextures[i], 0));
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        }
    }
    
    GL_CHECK(glDrawBuffers(drawBuffers.size(), &drawBuffers[0]));
    
    if (depthTargetTexture != 0)
    {
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTargetTexture, 0));
    }
    
#ifndef NDEBUG    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Warning: frame buffer " << frameBufferIndex << " is incomplete: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << " (incomplete = " << GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT << ", wrong dimensions = " << GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT << ", missing attachment = " << GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT << ", unsupported = " << GL_FRAMEBUFFER_UNSUPPORTED << ")." << std::endl;
        assert(false);
    }
#endif
    
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Renderer::clearTargets() const
{
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferIndex));
    GL_CHECK(glDepthMask(GL_TRUE));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Renderer::render() const
{
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferIndex));
    //GL_CHECK(glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT));
    
    if (viewportSize.x > 0 && viewportSize.y > 0)
    {
        GL_CHECK(glViewport(0, 0, viewportSize.x, viewportSize.y));
    }
    
    //Try to minimize shader program switching.
    unsigned int lastShaderProgramHash = 0;
    const detail::BoundProgram *shaderProgram = 0;
    unsigned int nrShaderProgramSwitches = 0;
    
    uniformMap.bindTextures();
    
    for (std::map<unsigned int, detail::BoundRenderable *>::const_iterator i = renderables.begin(); i != renderables.end(); ++i)
    {
        const detail::BoundRenderable *renderable = i->second;
        
        if (renderable->readFromDepthTexture) GL_CHECK(glEnable(GL_DEPTH_TEST));
        else GL_CHECK(glDisable(GL_DEPTH_TEST));
        
        GL_CHECK(glDepthMask(renderable->writeToDepthTexture ? GL_TRUE : GL_FALSE));

        if (renderable->blendMode == BlendReplace)
        {
            GL_CHECK(glDisable(GL_BLEND));
        }
        else
        {
            GL_CHECK(glEnable(GL_BLEND));
            
                 if (renderable->blendMode == BlendAdd) GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE));
            else if (renderable->blendMode == BlendMix) GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        }
        
        if (renderable->cullMode == CullBack)
        {
            GL_CHECK(glEnable(GL_CULL_FACE));
            GL_CHECK(glCullFace(GL_BACK));
        }
        else if (renderable->cullMode == CullFront)
        {
            GL_CHECK(glEnable(GL_CULL_FACE));
            GL_CHECK(glCullFace(GL_FRONT));
        }
        else if (renderable->cullMode == CullNothing)
        {
            GL_CHECK(glDisable(GL_CULL_FACE));
        }
        
        //TODO: Is this very inefficient? Should we let the rendererable decide whether or not to update the uniforms every frame?
        if (renderable->shaderProgramHash != lastShaderProgramHash)
        {
            //If we have a different shader program, bind it.
            lastShaderProgramHash = renderable->shaderProgramHash;
            ++nrShaderProgramSwitches;
            
            //We should never have invalid hashes.
            assert(shaderPrograms.find(lastShaderProgramHash) != shaderPrograms.end());
            shaderProgram = shaderPrograms.find(lastShaderProgramHash)->second;
            shaderProgram->bind();
            shaderProgram->setUniforms(uniformMap);
        }
        
        assert(shaderProgram);
        
        shaderProgram->setUniforms(renderable->renderable->uniformMap);
        
        renderable->renderable->uniformMap.bindTextures(uniformMap.getNrTextures());
        renderable->renderable->bind();
        renderable->renderable->render(shaderProgram->getProgram());
        renderable->renderable->unbind();
        renderable->renderable->uniformMap.unbindTextures(uniformMap.getNrTextures());
    }
    
    GL_CHECK(glUseProgram(0));
    
    uniformMap.unbindTextures();
    
    //GL_CHECK(glPopAttrib());
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
#ifndef NDEBUG
    //std::cerr << "Switched shaders " << nrShaderProgramSwitches << " times for " << renderables.size() << " renderables." << std::endl;
#endif
}

