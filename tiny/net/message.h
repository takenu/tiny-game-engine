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

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

#include <SDL_net.h>

#include <tiny/math/vec.h>

namespace tiny
{

namespace net
{

namespace vt
{

enum vt_enum
{
    Integer,
    IVec2,
    IVec3,
    IVec4,
    Float,
    Vec2,
    Vec3,
    Vec4
};

}

typedef unsigned short id_t;

struct VariableType
{
    VariableType(const std::string &, const vt::vt_enum &);
    
    std::string name;
    vt::vt_enum type;
};

struct VariableData
{
    VariableData();
    VariableData(const int &);
    VariableData(const unsigned int &);
    VariableData(const ivec2 &);
    VariableData(const ivec3 &);
    VariableData(const ivec4 &);
    VariableData(const float &);
    VariableData(const vec2 &);
    VariableData(const vec3 &);
    VariableData(const vec4 &);
    
    int iv1;
    ivec2 iv2;
    ivec3 iv3;
    ivec4 iv4;
    float v1;
    vec2 v2;
    vec3 v3;
    vec4 v4;
};

struct Message
{
    Message() :
        id(0),
        data()
    {

    }
    
    Message(const id_t &a_id) :
        id(a_id),
        data()
    {

    }
    
    template<typename T>
    Message & operator << (const T &a)
    {
        data.push_back(VariableData(a));
        return *this;
    }
    
    id_t id;
    std::vector<VariableData> data;
};

class MessageType
{
    public:
        MessageType(const id_t &, const std::string &, const std::string & = "");
        virtual ~MessageType();
        
        size_t getNrVariables() const;
        std::string getDescription() const;
        size_t getSizeInBytes() const;
        
        bool textToMessage(const std::string &, Message &) const;
        bool messageToText(const Message &, std::string &) const;
        
        size_t dataToMessage(const unsigned char *, Message &) const;
        size_t messageToData(const Message &, unsigned char *) const;
        
    protected:
        void addVariableType(const std::string &, const vt::vt_enum &);
        
    public:
        const id_t id;
        const std::string name;
        const std::string usage;
        
    private:
        std::vector<VariableType> variableTypes;
};

class MessageTranslator
{
    public:
        MessageTranslator();
        virtual ~MessageTranslator();
        
        bool textToMessage(const std::string &, Message &) const;
        bool messageToText(const Message &, std::string &) const;
        bool sendMessageTCP(const Message &, TCPsocket);
        bool receiveMessageTCP(TCPsocket, Message &);
        std::string getMessageTypeNames(const std::string & = ", ") const;
        std::string getMessageTypeDescriptions() const;
        
    protected:
        bool addMessageType(const MessageType *);
        
    private:
        std::vector<unsigned char> messageBuffer;
        std::map<id_t, const MessageType *> messageTypes;
};

}

}

