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
#pragma once

#include <exception>
#include <vector>
#include <string>
#include <map>

#include <cassert>

#include <draw/texture.h>
#include <draw/vertexbuffer.h>
#include <draw/vertexbufferinterpreter.h>
#include <draw/renderable.h>
#include <draw/renderer.h>

namespace tiny
{

namespace draw
{

class SquareVertexBuffer : public VertexBuffer<float>
{
    public:
        SquareVertexBuffer();
        ~SquareVertexBuffer();
};

class SquareVertexBufferInterpreter : public VertexBufferInterpreter
{
    public:
        SquareVertexBufferInterpreter();
        ~SquareVertexBufferInterpreter();
        
    private:
        SquareVertexBuffer vertices;
};

class ComputeTextureInput : public Renderable
{
    public:
        ComputeTextureInput(const std::vector<std::string> &inputNames, const std::string &a_fragmentShadercode);
        ~ComputeTextureInput();
        
        std::string getVertexShaderCode() const;
        std::string getFragmentShaderCode() const;
        
        template <typename TextureType>
        void setInput(const TextureType &texture, const std::string &name)
        {
            if (inputSizes.find(name) == inputSizes.end())
            {
                std::cerr << "Warning: input texture '" << name << "' cannot be found!" << std::endl;
                return;
            }
            
            inputSizes[name] = std::pair<size_t, size_t>(texture.width, texture.height);
            setTexture(texture, name);
        }
    
    protected:
        void render(const ShaderProgram &) const;
        
    private:
        std::map<std::string, std::pair<size_t, size_t> > inputSizes;
        const std::string fragmentShaderCode;
        SquareVertexBufferInterpreter square;
};

class ComputeTextureOutput : public Renderer
{
    public:
        ComputeTextureOutput(const std::vector<std::string> &outputNames);
        ~ComputeTextureOutput();
};

class ComputeTexture
{
    public:
        ComputeTexture(const std::vector<std::string> &inputNames, const std::vector<std::string> &outputNames, const std::string &fragmentShaderCode);
        ~ComputeTexture();
        
        template <typename TextureType>
        void setInput(const TextureType &texture, const std::string &name)
        {
            input.setInput(texture, name);
        }

        template <typename TextureType>
        void setOutput(const TextureType &texture, const std::string &name)
        {
            output.setTextureTarget(texture, name);
        }
        
        void compute() const;

    private:
        ComputeTextureInput input;
        ComputeTextureOutput output;
};

}

}
