// The DAGFrameScheduler is a Multi-Threaded lock free and wait free scheduling library.
// © Copyright 2010 - 2014 BlackTopp Studios Inc.
/* This file is part of The DAGFrameScheduler.

    The DAGFrameScheduler is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The DAGFrameScheduler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The DAGFrameScheduler.  If not, see <http://www.gnu.org/licenses/>.
*/
/* The original authors have included a copy of the license specified above in the
   'doc' folder. See 'gpl.txt'
*/
/* We welcome the use of the DAGFrameScheduler to anyone, including companies who wish to
   Build professional software and charge for their product.

   However there are some practical restrictions, so if your project involves
   any of the following you should contact us and we will try to work something
   out:
    - DRM or Copy Protection of any kind(except Copyrights)
    - Software Patents You Do Not Wish to Freely License
    - Any Kind of Linking to Non-GPL licensed Works
    - Are Currently In Violation of Another Copyright Holder's GPL License
    - If You want to change our code and not add a few hundred MB of stuff to
        your distribution

   These and other limitations could cause serious legal problems if you ignore
   them, so it is best to simply contact us or the Free Software Foundation, if
   you have any questions.

   Joseph Toppi - toppij@gmail.com
   John Blackwood - makoenergy02@gmail.com
*/
#ifndef _lockguard_h
#define _lockguard_h

// Parts of this library use the TinyThread++ libary and those parts are also covered by the following license
/*
Copyright (c) 2010-2012 Marcus Geelnard

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/


/// @file
/// @brief Declares a tool for automatically unlocking a mutex in an exception safe way.

namespace Mezzanine
{
    namespace Threading
    {
        /// @brief Lock guard class.
        /// @details The constructor locks the mutex, and the destructor unlocks the mutex, so
        /// the mutex will automatically be unlocked when the lock guard goes out of
        /// scope. Example usage:
        /// @code
        /// mutex m;
        /// int counter;
        ///
        /// void increment()
        /// {
        ///   lock_guard<mutex> guard(m);
        ///   ++counter;
        /// }
        /// @endcode
        template <class T>
        class lock_guard
        {
            public:
                /// @brief This allows other code to use the type of this mutex in a more safe way.
                typedef T mutex_type;

                /// @brief The constructor locks the mutex.
                /// @param aMutex Any mutex which implements lock() and unlock().
                explicit lock_guard(mutex_type& aMutex)
                {
                    mMutex = &aMutex;
                    mMutex->lock();
                }

                /// @brief The destructor unlocks the mutex.
                ~lock_guard()
                {
                    if(mMutex)
                        mMutex->unlock();
                }

            private:
                /// @internal
                /// @brief A non-owning pointer to the mutex.
                mutex_type* mMutex;
        };//lock_guard


    }//Threading
}//Mezzanine

#endif
