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
#include <cstdlib>

#include <tiny/snd/alcheck.h>

void tiny::snd::CheckOpenALError(const char *statement, const char *fileName, const int line)
{
    const ALCenum error = alGetError();
    
    if (error != AL_NO_ERROR)
    {
        std::cerr << "OpenAL error " << error << " (" << alGetString(error) << ") at " << fileName << ":" << line << " for '" << statement << "'!" << std::endl;
        abort();
    }
    else
    {
        //std::cerr << statement << " OK." << std::endl;
    }
}
