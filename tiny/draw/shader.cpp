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
#include <tiny/draw/shader.h>

using namespace tiny::draw;

VertexShader::VertexShader() :
    Shader<GL_VERTEX_SHADER>()
{

}

VertexShader::~VertexShader()
{

}

GeometryShader::GeometryShader() :
    Shader<GL_GEOMETRY_SHADER>()
{

}

GeometryShader::~GeometryShader()
{

}

FragmentShader::FragmentShader() :
    Shader<GL_FRAGMENT_SHADER>()
{

}

FragmentShader::~FragmentShader()
{

}

