# README #
The DAGFrameScheduler is a Multi-Threaded lock free and wait free scheduling library designed for use in video games.

The source code is stored publically at https://github.com/BlackToppStudios/DAGFrameScheduler

This is licensed under the GPL v3 license as per the file 'LICENSE'.

See COMPILING.md for details on building this library.

## Project layout  ##
The 'src' folder includes the source and header in the library.

The contents of the 'tests' folder are required only for verifying the library works.

The 'doc' folder contains further licensing details, technical documentation and notes BTS developers may a have left.

### Doxygen Docs ###
Doxygen is used to generate the technical docs in the 'doc/html' folder. This generates a series of html files from the source code. See 'index.html' for an overview of the API and a good starting point for perusing the documentation. 

The Doxygen configuration resides in 'doc/doxyfiles'. From this directory the contents of the 'doc/html directory with a command similar to `doc/doxyfiles$ doxygen Doxyfile 2> DoxygenWarnings.txt` depending on your platform. To get the higher quality graphs graphviz and dot need to be installed an doxygen needs to be able to find them.

## NEWS ##

### Dec 19, 2012 ###
* Added option for configuring how long frames track performance. A few other options throughout the library also use this.
* Complete the Work Unit Sorter and tested. There is no proper unit test for this, but the helperunits does emit a verbose log in debug mode to facilitate debugging features like this.
* I added a potential optimization that should skip many cache flush when working with large amount of workunits, this can be enabled with the Mezz_DecacheWorkUnits CMake option.

### Dec 16, 2012 ###
* I added proper markdown to the README and added a document, COMPILING.md that loosely describes the build process and some of the technicial decisions made while creating this library.
* Further cleaning of doxygen docs. Almost done with main algorithm page.
* Started work on a WorkSorter work unit to offload execution time of that into another thread, to maximize concurrency.
* Simplifed FrameScheduler::DoOneFrame() and made frame pause timings more accurate, down to around .004% variance on my dev system.

### Dec 14, 2012 ###
* A large documentation update. No more warnings in doxygen doc generation. I still need to describe the algorithm in greater detail on the main page though.

### Dec 13, 2012 ###
* I reimplemented parts of the algorithm allowing for the creation of an option that enables or disables thread creation each frame. This uses two Barriers to performs synchronizations twice during the a frame. In theory on systems with atomic operations this should be faster, and on system without it should be slower, but my testing has not been so conlusive.
* Grouped CMake Options.
* Updated doxygen docs. I also added some random scriblings and notes I have written to the docs folder.

### Dec 9, 2012 ###
* Added an example Asynchronous File loading workunit that seems to perform near the peak capacity of the hardware I tested it on. Also a little bit of documentation cleanup.
* I fixed an issue with removing workunits from the framescheduler, but this functionality currently has no test.
* Again, I compile one multiple compilers (gcc, mingw, vs2010) ensuring that all tests pass and no warnings are produced.

### Dec 7, 2012 ###
* Removed friendships between WorkUnit and FrameScheduler (Some core algorithm classes keep this though).
* Separated the WorkUnits Implementation from its interface. I did not do this with the other work units, it seems that this will satisfy most customization needs along with changing the inheritance of the other workunits.

### Dec 6, 2012 ###
* Removed all occurrences of volatile keyword.
* Added a Barrier synchronization class.

### Dec 4, 2012 ###
* Fixed a scheduling issue which caused a thread to stall on a specific workunit if it had unment dependencies.
* Test for thread affinity emits manually checkable contents and passes manual inspection.
* I found a bug with the the 'threadrestart' test, where it sometimes runs all wokunits on one thread. This is not ideal, but it is valid, I have found a way to fix this, but it will take time to implement.

### Dec 3, 2012 ###
* Simplified parameter passing to WorkUnits now they only recieve one ThreadSpecific storage.
* Re-worked pause timing algorithm to make it more simple. Logically it just carries a portion of any amount it was off into the next frame.
* Updated documentation. I have started a small project attempt to read/revise every piece of doxygen docs going over each file alphabetically to and I am at framescheduler.h.

### Dec 2, 2012 ###
* Fixed warnings in Mingw, clang and VS2012. 0 warnings in any of these compilers at this moment and it passes tests in windows/linux with these compilers where applicable.
* Removed integer overflow in some places where the results of GetTimeStamp() were assigned to datatypes smaller than 64 bit.


### Dec 1, 2012 ###
* Enabled all build warnings, and corrected them in GCC.
* Updated the way unit tests are run to allow the isolated of execution of one test or all the tests at once. Fixed Framerate calculation bugs in performane tests.

### Nov 30, 2012 ###
* Created a single point at which to update all the creation of double buffered resources. The goal is to make changing this easy if another project wants to add double buffered resources to each thread.
* Added WorkUnit affinity. A special way to add WorkUnits so that they will be scheduled with the rest of the WorkUnits, but only executed on the main thread.
* More Documentation updates.

### Nov 29, 2012 ###
* Documentation updates.

### Nov 28, 2012 ###
* Bugfixes with workunit sorting and a simple performance test has been added.

### Nov 27, 2012 ###
* Removed Bi-Directionality from Dependency Graph. At their largest WorkUnits are now 56 bytes. This was done so that they would fit in a single cache line on most systems. This also speeds up adding or removing WorkUnits. When the information the Reverse dependency graph needed is required a temporary cache is updated on the scheduler itself. When this depedent (no cy) graph is updated the WorkUnitKeys used for sorting the work are also updated. This can done with calls to FrameScheduler::UpdateDependencyCache and can be control with a bool argument passed in to FrameScheduler::SortWorkUnits().

### Nov 26, 2012 ###
* Verified support for Mac OS X. Now this compiles on Mac os X 10.6.8 with GCC 4.2.1, Ubuntu x64 with GCC 4.3.2 or Clang 3.0.6-6, or on windows xp with MinGW or visual studio 10.
* Timing test on Mac OS X and Linux both have submicrosecond accuracy, and they both provide 1 microsecond times, winxp does not, but this is still accurate on the order of 1000s of microseconds, and the timer is usually about that accurate.
