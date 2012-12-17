// The DAGFrameScheduler is a Multi-Threaded lock free and wait free scheduling library.
// © Copyright 2010 - 2012 BlackTopp Studios Inc.
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
#ifndef _DAGFrameScheduler_h
#define _DAGFrameScheduler_h

/// @file
/// @brief This is the file that code using this library should include. It includes all the required components.

#include "asynchronousfileloadingworkunit.h"
#include "asynchronousworkunit.h"
#include "atomicoperations.h"
#include "barrier.h"
#include "compilerthreadcompat.h"
#include "datatypes.h"
#include "doublebufferedresource.h"
#include "framescheduler.h"
#include "frameschedulerworkunits.h"
#include "monopoly.h"
#include "mutex.h"
#include "rollingaverage.h"
#include "systemcalls.h"
#include "thread.h"
#include "threadingenumerations.h"
#include "workunit.h"
#include "workunitkey.h"

/// @mainpage Directed Acyclic Graph Frame Scheduler.
/// @section goal_sec Goals
/// This library tries to make writing multithreaded software easier by changing the kinds of primitives that
/// multithreaded software is built upon. Several libraries before this have attempted this already.
/// This library is different becuse it focuses on a specific kind of workload and provides the kinds of
/// guarantees that workload needs while sacrificing other guarantees that workload does need.
/// @n @n
/// This attempts to provide a multithreading solution for workloads that must be run in many iterations in
/// a given amount of realtime. Games are an ideal example. Every frame a video game, must update physics
/// simulations, make AI decisions, accept/interpret user input, enforce game rules, perform dynamic I/O
/// and render it to the screen all while maintaining a smooth FrameRate and do that while minimizing drain
/// batteries on portable devices (sometimes without even knowing if the device is portable).
/// @n @n
/// This library accomplishes those goals by removing the conventional mutlithreading primitives that so many
/// developers have come to fear, loathe or misunderstand. Mutexes, threads, memory fences, thread_local
/// storage, atomic variables, and all the pitfalls that come with them are replaced by a small set of
/// of primitives that provide all the required sophistication a typical multi-threaded application
/// requires. It does this using a new kind of @ref Mezzanine::Threading::iWorkUnit "iWorkUnit",
/// @ref Mezzanine::Threading::DoubleBufferedResource "Double Buffering", A strong concept of
/// Dependencies and a @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" that uses heuristics
/// to decide how to run it all without exposing needless complexity to the application developer.
///
/// @section overview_sec Overview
/// The DAGFrameScheduler is a variation on a common multithreaded work queue. It seeks to avoid its pitfalls,
/// such as non-determinism, thread contention and lackluster scalability, while keeping its advantages
/// including simplicity, understandiblity and low overhead.
/// @n @n
/// With this algorithm very few if any
/// calls will need to be made to the underlying system for synchronization in the actual work to be performed.
/// Instead, this library will provide limited
/// deterministic ordering of @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" execution through a dependency
/// feature. Having the knowledge that one @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" will complete after
/// another allows for resources to be used without using expensive and complex synchronization mechansisms
/// like @ref Mezzanine::Threading::Mutex "mutexes", semaphores, or even an
/// @ref Mezzanine::Threading::AtomicCompareAndSwap32 "Atomic Compare And Swaps". These primitives are provided
/// to allow use of this library in advanced ways for developers who are already familiar with
/// multithreaded systems.
/// @n @n
/// The internal work queue is not changed while a frame is executing. Because it is only read, each
/// thread can pick its own work. Synchronization still needs to occur, but it has been moved onto each
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" it is manages this with atomic CPU operations. Like this,
/// contention is less frequent, occurring only when threads simultaneously attempt to start the same
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit", and it consumes far less time because atomic operations
/// are CPU instructions instead of Operating System calls. This is managed by the library, so individual
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit"s do not need to worry synchronization beyond telling
/// each @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" about its data dependencies and making sure
/// all the @ref Mezzanine::Threading::iWorkUnit "iWorkUnit"s added to a
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler".
///
/// @section broken_sec Broken Algorithms
/// To understand why a new multithreading system is needed, it is helpful to look at other methods
/// of threading that have been used in the past. This can give us an understanding of what they lack
///  or how they aren't ideal for the kinds of work this algorithm is intended for. This overview is
/// intentionally simplified. There are variations on many of these algorithms that can fix some of
/// the problems presented. Despite these workarounds there are fundamental limitations that prevent
/// these algorithms from being ideal for the video games and similar tasks.
/// These threading models aren't necessarily broken, some of these clearly have a place in software
/// development. Many of these require complex algorithms, require subtle knowledge or simply aren't
/// performant enough for realtime environments.
/// @n @n
/// I will use charts that plot possible resource use of a computer across time. Generally time will
/// run accross the top a resources, usually CPUs will run down one side. Most of these algorithms have a
/// concept of tasks or workunits, these are just piece of work with a distinct begining and end. The
/// width of a piece of work loosely represents the execution time (the names are just for show and not
/// related to anything real).
/// @subsection broken_Single Single Threaded
/// An application using this threading model is not actually multithreaded at all. However, It has been shown
/// that software can run in a single and get good perfomance. This is benchmark all other threading models
/// get compared too.
/// @n @n
/// There is a term, Speedup ( http://en.wikipedia.org/wiki/Speedup ), which is simply a
/// comparison of the single threaded performance of an algorithm to the mutlithreaded performance. You simply
/// determine how many times more work the multithreaded algorithm does in the same time, or how many times
/// longer the single threaded algorithm takes to the same work. Ideally two threads will be twice as fast
/// (speedup of 2x), and three thread would be three times as fast (3x speedup), and so; this is called linear
/// speedup. In practice there is always some overhead in creating and synchronizing threads, so achieving
/// linear speedup is diffucult.
/// @image html Single.png "Single Threaded Execution - Fig 1."
/// @image latex Single.png "Single Threaded Execution - Fig 1."
/// @image rtf Single.png "Single Threaded Execution - Fig 1."
/// @n @n The DAGFrameScheduler library tries to tailor the threading model to the problem to minimize that
/// overhead. With a single threaded application one thread does all the work and always wastes every other
/// thread, but there is no overhead if the system only has one thread.
/// @n @n
/// @subsection broken_Unplanned Unplanned Thread
/// Sometimes someone means well and tries to increase the performance of a single threaded program and tries
/// to add extra threads to increase performance. Sometimes this works, really well, sometimes there is a
/// marginal increase in performance or a significant increase in bugs. If that someone has a good plan
/// then they can usually achieve close to the best speedup possible in the given situation. This is not easy
/// and many cannot do this or do not want to invest the time it would take. If not carefully planned
/// bugs like deadlock ( http://en.wikipedia.org/wiki/Deadlock ) and race conditions
/// ( http://stackoverflow.com/questions/34510/what-is-a-race-condition )
/// can be introduced. Unfortunately no amount of testing can replace this careful planning. Without a
/// complete understanding of how multithreaded software is assembled (a plan) it is not possible to prove
/// that multithreaded software will not hang/freeze or that it will produce the correct results.
/// @n @n
/// Software with no multithreading plan could have just about any kind of execution behavior. Usually
/// unplanned software performs at least slightly better than single threaded versions of the software, but
/// frequently does not utilize all the available resources. Generally performance does not scale well as
/// unplanned software is run on more processors. Frequently, there is contention for a specific resource and
/// a thread will wait for that resource longer than is actually need.
/// @image html Unplanned.png "Unplanned Threaded Execution - Fig 2."
/// @image latex Unplanned.png "Unplanned Threaded Execution - Fig 2."
/// @image rtf Unplanned.png "Unplanned Threaded Execution - Fig 2."
/// @n @n
/// The DAGFrameScheduler is carefully planned and completely avoids costly synchronization
/// mechanisms in favor of less costly minimalistic ones. Marking one @ref Mezzanine::Threading::iWorkUnit "iWorkUnit"
/// as dependent on another allows the reordering of @ref Mezzanine::Threading::iWorkUnit "iWorkUnits" so that
/// some @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" can be executed with no thread waiting or blocking.
/// @n @n
/// @subsubsection broken_TaskPerThread One Task Per Thread
/// A common example of poor planning is the creation of one thread for each task in a game. Despite
/// being conceptually simple, performance of systems designed this was is poor due to synchronization
/// and complexities that synchronization requires.
/// @subsection broken_ConventionWorkQueue Convention Work Queue/Thread Pools
/// Conventional work queues and thread pools are well known and robust way to increase the throughput of
/// of an application. These are ideal solutions for many systems, but not games.
/// @n @n
/// In conventional workqueues all of the work is broken into a number of small thread-safe
/// units. As these units are created they are stuffed into a queue and threads pull out units of work
/// as it completes other units it has started. This simple plan has many advantages. If there is work
/// to do, then at least one thread will be doing some, and usually more threads will be working; this is
/// good for games and the DAGFrameScheduler mimics it. If the kind of work is unknown when the software is
/// written heuristics and runtime decisions can create the kind of units of work that are required. This
/// is not the case with games and the others kinds of software this library caters to, so changes can
/// be made that remove the problems this causes. One such drawback is that a given unit of work never
/// knows if another is running or has completed, and must therefor make some pessimistic assumptions.
/// @image html Threadpool.png "Convention Work Queue/ThreadPools - Fig 3."
/// @image latex Threadpool.png "Convention Work Queue/ThreadPools - Fig 3."
/// @image rtf Threadpool.png "Convention Work Queue/ThreadPools - Fig 3."
/// @n @n
/// Common synchronization mechanisms like mutexes or semaphores block the thread for an unknown
/// amount of time, and are required by the design of workqueues. There are two times this is required.
/// The first time is whenever a work unit is acquired by a thread, a mutex (or similar) must be used
/// to prevent other threads from modifying the queue as well. This impacts scalability, but can be
/// circumvented to a point. Common ways to work around this try to split up the work queue
/// pre-emptively, or feed the threads work units from varying points in the queue. The
/// DAGFrameScheduler moves the synchronizantion onto each work to greatly reduce the contention as
/// more workunits are added.
/// @n @n
/// The other, and less obvious, point of contention that has not be circumvented in a
/// satisfactory way for games is the large of amount of synchronization required between units of
/// work that must communicate. For example, there may be hundreds of thousands of pieces of data
/// that must be passed from a system into a 3d rendering system. Apply mutexes to each would slow
/// execution an impossibly long time (if it didn't introduce deadlock), while more coarse grained
/// lock would prevent large portions of physics and rendering from occurring at the time causing
/// one or both of them to wait/block. A simple solution would be to run physics before graphics,
/// but common work queues do not provide good guarantees in this regard.
/// @n @n
/// The DAGFrameScheduler was explicitly designed to provide exactly this guarantee. If the
/// physics @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" is added to the graphics
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" with
/// @ref Mezzanine::Threading::iWorkUnit::AddDependency() "AddDependency(WorkUnit*)" then it will
/// always be run before the graphics workunit in a given frame. The drawback of this is that it
/// is more difficult to make runtime creation of workunits (It is possible but it cannot be done
/// during any frame execution), but completely removes the locking
/// mechanisms a conventional work queues. The DAGFrameScheduler has traded one useless feature
/// for a useful guarantee.
///
/// @section algorithm_sec The Algorithm
/// When first creating the DAGFrameScheduler it was called it "Dagma-CP". When describing it this
/// phrase "Directed Acyclic Graph Minimal Assembly of Critical Path" was used. If you are lucky
/// enough to knows what all those terms mean when assembled this way they are very descriptive. For
/// rest of us the algorithm tries to determine what is the shortest way to execute the work in a
/// minimalistic way using a mathematical graph. The graph is based on what work must done each
/// before what other work each frame and executing it. All the work in this graph will have a
/// location somewhere between the beginning and end, and will never circle around back so it can
/// be called acyclic.
/// @n @n
/// This algorithm was designed with practicality as the first priority. To accomodate and integrate
/// with a variety of other algorithms and system a variety of Work Units are provided. New classes
/// can be created that inherit from these to allow them to be in the scheduler where they will work
/// best.
///     @li @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" - The interface for a common workunit.
/// These work units will be executed once be frame after all their dependencies have completed.
/// These are also expected to complete execution in a relatively brief period of time compared to
/// the length of a frame, and create no threads while doing so.
///     @li @ref Mezzanine::Threading::DefaultWorkUnit "DefaultWorkUnit" - A simple implementation
/// of an @ref Mezzanine::Threading::iWorkUnit "iWorkUnit". This may not be suitable for every
/// use case, but it should be suitable for most. Just one function for derived classes to
/// implement, the one that does actual work.
///     @li @ref Mezzanine::Threading::iAsynchronousWorkUnit "iAsynchronousWorkUnit" - Intended to
/// allow loading of files and streams even after the framescheduler has paused. Work units are
/// to spawn one thread and manage it without interfering with other execution. DMA, and other
/// hardware coprocessors are expected to be utilized to their fullest to help accomplish this.
///     @li @ref Mezzanine::Threading::MonopolyWorkUnit "MonopolyWorkUnit" - These are expected
/// to monopolize cpu resources at the beginning of each frame. This is ideal when working with
/// other systems, forexample a phsyics system like Bullet3D. If the calls to a physics system are
/// wrapped in a @ref Mezzanine::Threading::MonopolyWorkUnit "MonopolyWorkUnit" then it will be
/// given full opportunity to run before other work units.
///
/// Once all the @ref Mezzanine::Threading::MonopolyWorkUnit "MonopolyWorkUnit"s are done then the
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" class instance spawns or activates
/// a number of threads based on a simple heuristic. This heuristic is the way work units are sorted
/// in preparation for execution. To understand how these are sorted, the dependency system needs to
/// be understood.
/// @n @n
/// Most other work queues do not provide any guarantee about the order work will be executed in.
/// This means that each piece of work must ensure its own data integrity using synchronization
/// primitives like mutexes and semaphores to protect from being corrupted by multithread access. In
/// most cases these should be removed and one of any two work units that must read/write the data
/// must depend on the other. This allows the code in the workunits to be very simple even if it
/// needs to use a great deal of data other work units may also consume or produce.
/// @n @n
/// Once all the dependencies are in place for any synchronization that has been removed, a
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" can be created and started. At
/// runtime this create a reverse dependency graph, a dependent graph. This is used for determine
/// which work units are the most depended on. For each work unit s simple count of how many work
/// units cannot start until has been completed is generated. The higher this number the earlier
/// the work unit will be executed in a frame. Additionally workunits that take longer to execute
/// will be prioritized ahead of work units that are faster.
/// @n @n
/// Here is a chart that provides an example of this re-factoring and the runtime sorting process:
/// @image html DAGSorting.png "DAG WorkSorting - Fig 4."
/// @image latex DAGSorting.png "DAG WorkSorting - Fig 4."
/// @image rtf DAGSorting.png "DAG WorkSorting - Fig 4."
/// @n @n
/// There are several advantages this sorting provides that are not immediately obvious. It separates
/// the scheduling from the execution allowing, the relatively costly sorting process to be executed
/// only when work units are added, removed or changed. In theory the sorting could be done in a
/// work then updated when the frame is complete. Prioritizing Workunits that take longer to run
/// should help insure the shortest critical path is found by minimizing how often dependencies
/// cause threads to wait for more work.
/// @todo Create a work sorting work unit.
///
/// @n @n
/// Each thread queries the
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" for the next piece of work...asdf
/// @image html DAGThreads.png "DAG threads - Fig 5."
/// @image latex DAGThread.png "DAG threads - Fig 5."
/// @image rtf DAGThreads.png "DAG threads - Fig 5."
/// @n @n
/// Some work must be run on specific threads, such as calls to underlying devices (for example,
/// graphics cards using Directx or OpenGL). These @ref Mezzanine::Threading::iWorkUnit "iWorkUnit"s
/// are put into a different listing where only the main thread will attempt to execute them. Other
/// than running these, and running these first, the behavior of the main thread is very similar to
/// other threads. Once a @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" has been completed the
/// thread will query the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" for more work.
/// Because the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" is never modified during
/// a frame there is no need for synchronization with it specifically, this avoids a key point of
/// contention that reduces scaling. Instead the synchronization is performed with each
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" and is an
/// @ref Mezzanine::Threading::AtomicCompareAndSwap32 "Atomic Compare And Swap" operation to maximize
/// performance.
/// @n @n
/// Even much of the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler"'s work is performed
/// in @ref Mezzanine::Threading::iWorkUnit "iWorkUnit"s, such as log aggregation and certain functions
/// that must be performed each frame.
/// @ref Mezzanine::Threading::iAsynchronousWorkUnit "iAsynchronousWorkUnit"s continue to run in a thread
/// beyond normal scheduling and are intended to will consume fewer CPU resources and more IO resources.
/// For example loading a large file or listening for network traffic. These will be normal
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit"s in most regards and will check on the asynchronous
/// tasks they manage each frame when they run as a normally scheduled.
/// @n @n
/// If a thread should run out of work because all the work is completed the frame will pause until it
/// should start the next frame. This pause length is calulated using a runtime configurable value on
/// the @ref Mezzanine::Threading::FrameScheduler "FrameScheduler". If a thread has checked every
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" and some are still not executing, but could not
/// be started because of incomplete dependencies the thread will simply iterate over every
/// @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" in the
/// @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" until the dependencies of one are
/// met and allows one to be executed. This implicitly guarantees that at least one thread will
/// always do work, and if dependencies chains are kept short then it is more likely that several
/// threads will advance.
/// @n @n
/// The @ref Mezzanine::Threading::iWorkUnit "iWorkUnit" classes are designed to be inherited from
/// and inserted into a @ref Mezzanine::Threading::FrameScheduler "FrameScheduler" which will
/// manage their lifetime and execute them when requested via
/// @ref Mezzanine::Threading::FrameScheduler::DoOneFrame() "FrameScheduler::DoOneFrame()".
/// @n @n
/// Insert DAGFramescheduler picture here.
/// @n @n
/// This documentation should not be considered complete nor should the algorithm
/// both are still under development.



/// @brief All of the Mezzanine game library components reside in this namespace.
/// @details The DAG Frame Scheduler is just one part of many in the Mezzanine. The Mezzanine as a
/// whole is intended to tie a complex collection of libraries into one cohesive library.
namespace Mezzanine
{
    /// @brief This is where game specific threading algorithms and a minimalistic subset of the std threading library a held
    /// @details this implements All of the Multithreaded Algorithms from BlackTopp Studios, parts of std::thread,
    /// std::this_thread, std:: mutex, and maybe a few others. In general only the specialized gaming algorithms store here are
    /// intended to be used in game code.
    namespace Threading
        {}
}

#endif
