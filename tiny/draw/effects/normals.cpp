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
#include <tiny/draw/effects/normals.h>

using namespace tiny::draw::effects;

Normals::Normals()
{

}

Normals::~Normals()
{

}

std::string Normals::getFragmentShaderCode() const
{
    return std::string(
"#version 150\n"
"\n"
"precision highp float;\n"
"\n"
"uniform sampler2D worldNormalTexture;\n"
"uniform vec2 inverseScreenSize;\n"
"\n"
"out vec4 colour;\n"
"\n"
"void main(void)\n"
"{\n"
"   vec2 tex = gl_FragCoord.xy*inverseScreenSize;\n"
"   vec3 normal = texture(worldNormalTexture, tex).xyz;\n"
"   \n"
"   colour = vec4(0.5f*(vec3(1.0f) + normalize(normal)), 1.0f);\n"
"}\n");
}

