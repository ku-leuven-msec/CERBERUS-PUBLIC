/*
 * Cerberus PKU-based Sandbox
 *
 * Check Cerberus/cerberus_ReMon/README.md for licensing terms.
 */

#ifndef MVEE_H_
#define MVEE_H_

/*-----------------------------------------------------------------------------
    Includes
-----------------------------------------------------------------------------*/
#include <cstddef>
#include <cstdio>
#include <pthread.h>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <deque>
#include <set>
#include <unordered_set>
#include <json/json.h>
#include "MVEE_build_config.h"
#include "MVEE_macros.h"

/*-----------------------------------------------------------------------------
    Forward Declarations
-----------------------------------------------------------------------------*/
class monitor;
class detachedvariant;
class mmap_table;

//
// Convenience class for locking a mutex
//
class MutexLock
{
private:
	pthread_mutex_t* mutex;
public:
	MutexLock(pthread_mutex_t* _mutex)
	{
		mutex = _mutex;
		pthread_mutex_lock(mutex);
	}

	~MutexLock()
	{
		pthread_mutex_unlock(mutex);
	}
};

/*-----------------------------------------------------------------------------
    Constants
-----------------------------------------------------------------------------*/
#include "MVEE_numcalls.h" // defines MAX_CALLS
#define NO_CALL   0x01000000

/*-----------------------------------------------------------------------------
    Global MVEE state
-----------------------------------------------------------------------------*/
//
// This represents the global state of the MVEE.
//
class mvee
{
public:

	// *************************************************************************
	// Logging Functions - These are implemented in MVEE_logging.cpp
	// *************************************************************************

	// 
	// Log a message to the log files. This function is disabled if the
	// MVEE_BENCHMARK preprocessor option is set.
	// 
	static void        logf                        (const char* format, ...) __attribute__((format (printf, 1, 2)));

	// 
	// Log a message to stdout and to the log files. Even with MVEE_BENCHMARK
	// enabled, the message is still printed to stdout.
	//
	static void        warnf                       (const char* format, ...) __attribute__((format (printf, 1, 2)));

	//
	// Create a process by executing the command specified by @proc and return
	// its output.
	//
	static std::string log_read_from_proc_pipe     (const char* proc, size_t* output_length);

	//
	// Convert the buffer specified by @hexbuffer to hex string format
	//
	static std::string log_do_hex_dump             (const void* hexbuffer, int buffer_size);

	//
	// Helper function to log sigaction structures.
	//
	static void        log_sigaction               (struct sigaction* action);

	// *************************************************************************
	// MVEE Initialization/Shutdown - Implemented in MVEE.cpp
	// *************************************************************************

	//
	// Start the monitor and mvee::numvariants variants.  This is the default
	// mode for the MVEE
	//
	static void start_monitored             ();

	//
	// Reads the MVEE configuration from MVEE.ini, possibly creating the file in
	// the process.
	//
	static void init_config                 ();
	static void init_config_set_defaults    ();
	static bool process_opts                (int argc, char** argv, bool add_args);
	static void add_argv                    (const char* arg, bool first_extra_arg);

	//
	// Asynchronously request a shutdown of the entire MVEE
	//
	static void        request_shutdown       ();

	//
	// Implemented in MVEE_config.cpp. Sets up the config to launch a known
	// variant set. We mainly use this to create shortcuts for benchmarks
	//
	static void        set_builtin_config     (int builtin);

	// *************************************************************************
	// Monitor Management - Implemented in MVEE.cpp
	// *************************************************************************

	//
	// Safely store the specified set of variant pids in the
	// mvee::variant_pid_mapping map.
	//
	static void                                 register_variants           (std::vector<pid_t>& pids);

	//
	// Safely store the specified monitor in the mvee::monitor_id_mapping map
	// and activate the monitor through monitor::signal_registration
	//
	static void                                 register_monitor            (monitor* mon);

	//
	// Remove the specified monitor from the mvee::monitor_id_mapping map and
	// add it to the garbage collection list.
	//
	// This function also shuts down the MVEE when the last running monitor
	// unregisters.
	//
	static void                                 unregister_monitor          (monitor* mon, bool move_to_dead_monitors=true);

	//
	// Returns true if the MVEE is monitoring variants that consist of multiple
	// processes (i.e. tasks that have different tgids.
	//
	static bool                                 is_multiprocess             ();

	//
	// Get the next available monitor identifier.
	//
	static int                                  get_next_monitorid          ();

	//
	// Tell all monitors to check if their variants are multithreaded.
	// We use this to enable/disable the synchronization agents.
	//
	static void                                 set_should_check_multithread_state (int monitorid);

	//
	// Add the specified variant to the mvee::detachlist vector. A newly created
	// monitor can look through this list to fetch information about the
	// variants it should attach to.
	//
	static void                                 add_detached_variant        (detachedvariant* variant);

	// 
	// Returns true if the mvee::detachlist vector contains information about
	// variants that have been detached from the specified monitor
	//
	static bool                                 have_detached_variants      (monitor* mon);

	// 
	// Removes the specified variant from the mvee::detachlist vector.
	// Returns the detachedvariant struct for the removed variant.
	//
	static detachedvariant*                     remove_detached_variant     (pid_t variantpid);

	// 
	// Returns true if the mvee::detachlist vector contains information
	// about variants to which the specified monitor should attach
	//
	static int                                  have_pending_variants       (monitor* mon);

	//
	// Initializes a table of locks that should be locked for each syscall
	// executed by the variants. This function is implemented in
	// MVEE_syscalls_handlers.cpp.
	//
	static void init_syslocks               ();

	// *************************************************************************
	// OS/Environment configuration
	// *************************************************************************

	//
	// Returns the name of the folder from which we started the MVEE.
	//
	static std::string   os_get_orig_working_dir     ();

	// 
	// Returns the name of the root directory for the MVEE. This is the
	// directory that contains the MVEE, MVEE_LD_Loader, patched_binaries,
	// ... subdirectories
	// 
	static std::string   os_get_mvee_root_dir        ();

	//
	// Check and possibly change the kernel.yama.ptrace_scope setting through
	// sysctl. This is a temporary fix for the problem described in
	// https://lkml.org/lkml/2014/12/24/196
	//
	static void          os_check_ptrace_scope       ();

	//
	// Get the monitor process id
	//
	static pid_t         os_getpid                   ();

	// 
	// Get the monitor thread id
	//
	static pid_t         os_gettid                   ();

	// 
	// Get the full path to the default ELF interpreter on the host machine
	// 
	static std::string   os_get_interp               ();

	//
	// Test if the specified binary is dynamically linked and/or PIE
	//
	static bool          os_can_load_indirect        (std::string& image);

	// 
	// Determine the name of the interpreter to be used to execute @file
	// and add it to the @add_to_list deque.
	//
	// We do this by: 
	//
	// 1) Adding no interpreter at all if the file is an ELF file for the
	// host platform
	//
	// 2) checking the first line of the file to see if it starts with a
	// hashbang (#!). If it does, we add the interpreter specified by the
	// hashbang line.
	//
	// 3) looking at the file extension. We currently support .sh and .rb files
	// 
	static bool          os_add_interp_for_file      (std::deque<char*>& add_to_list, std::string& file);

	// 
	// Cache the interpreter name for the specified file
	//
	static void          os_register_interp          (std::string& file, const char* interp);

	// 
	// Get the full path for the MVEE_LD_Loader to be used on the host platform
	//
	static std::string   os_get_mvee_ld_loader       ();

	// 
	// Resets the environment variables we may have set earlier
	// 
	static void          os_reset_envp               ();

	//
	// Normalizes @path by handling relative paths, double slashes, etc.
	//
	static std::string   os_normalize_path_name      (std::string path);

	// *************************************************************************
	// Miscellaneous Support Functions
	// *************************************************************************

	//
	// Tokenizes a string
	// 
	static std::deque<std::string> strsplit(const std::string& s, char delim);

	// 
	// Returns true if the search string ends with the specified suffix
	//
	static bool                    str_ends_with(std::string& search_in_str, const char* suffix);

	//
	// Valgrind-friendly version of strdup
	//
	static char*                   strdup(const char* orig);

	//
	// converts a string to upper case
	//
	static std::string             upcase(const char* lower_case_string);


	// 
	// Convert an "old" integer-style sigset to a "new" sigset_t-style sigset
	//
	static sigset_t                old_sigset_to_new_sigset(unsigned long old_sigset);

	//
	// Access to global state - This lock protects the public variables that may
	// be modified at run-time
	//
	static void                    lock ();
	static void                    unlock ();

	// *************************************************************************
	// Monitor settings and properties. All of these are initialized during
	// monitor startup and not modified afterwards. It is therefore safe to read
	// these without holding the mvee lock
	// *************************************************************************

	// Number of variants we're running ... this should always be one for Cerberus
	static int                      numvariants;

	// Spec ids for the variants we're running ... this should always contain just one variant for Cerberus
	static std::vector<std::string> variant_ids;

	// Configuration read from MVEE.ini
	static std::string              config_file_name;
	static std::string              config_variant_set;
	static bool                     config_show;
	static Json::Value              config;
	static Json::Value*             config_variant_global;
	static Json::Value*             config_variant_exec;
	static Json::Value*             config_monitor;

	// monitor object and id of the monitor we're running in this thread we used
	// to use this for almost everything but nowadays it's really just here for
	// logging...
	static __thread monitor*        active_monitor;
	static __thread int             active_monitorid;

	//
	// This is set when the mvee has been signalled for shutdown.
	// There are very frequent accesses to this variable and those
	// accesses are intentionally lock-free
	//
	static int                      shutdown_signal;

	// 
	// this maps syscalls onto the locks they need to execute reliably
	//
	static std::map<unsigned long, unsigned char>
									syslocks_table;

	//
	// Lock/Cond that protects the variables below
	//
	static pthread_mutex_t          global_lock;

	//
	// This cond var is used to coordinate the safe shutdown of the MVEE.
	// There are two types of threads that can wait on this cond var:
	//
	// 1) The "management thread" (aka the main thread of the MVEE process)
	// waits for this cond var and gets woken up whenever:
	// * a monitor moves from the active to the inactive list
	// * a monitor moves from the inactive to the dead list
	// * an external user or process requests a full MVEE shutdown 
	// by sending a signal to the MVEE process
	// 
	// 2) Monitor threads wait for this cond var when they're shutting down
	// and they're waiting for other monitors in the same thread group to
	// shut down
	static pthread_cond_t           global_cond;

	//
	// We use this to coordinate the initial transfer of the variants' ptrace
	// control from the main MVEE process to the primary monitor thread.
	//
	volatile static unsigned long   can_run;

	// *************************************************************************
	// Cerberus special
	// *************************************************************************

#define SPECIALDIR "./special/"

	//
	// Lock that protects the variables below
	//
	static pthread_mutex_t                       special_lock;

	//
	// All special files, seen so far
	//
	static std::unordered_set<unsigned long>     special_files;

	//
	// Creates the folder that Cerberus uses for "special" executables
	//
	static void        special_init                  ();

	//
	// Create special cerberus file if it is needed
	// @full_path full path to the original executable file
	//
	static std::string cerberus_create_special_file  (const std::string& full_path);
private:

	// *************************************************************************
	// Protected Logging functions - Implemented in MVEE_logging.cpp
	// *************************************************************************

	//
	// Wipes the Logs folder
	//
	static void clear_log_folder();

	// 
	// Opens the global log files (i.e. MVEE.log, MVEE_ptrace.log, ...)
	//
	static void log_init();

	//
	// Closes the global log files
	// 
	static void log_fini(bool terminated);

	// *************************************************************************
	// Variant Initialization
	// *************************************************************************

	//
	// Add the specified path to the LD_LIBRARY_PATH environment variable.  By
	// default, we prepend the specified @library_path with the path to the MVEE
	// root folder and append the library path suffix (e.g. "/amd64/") to the
	// path.
	// 
	static void        add_library_path       (const char* library_path, bool append_arch_suffix=true, bool prepend_mvee_root=true);

	//
	// Implemented in MVEE_variant_launch.cpp. Set up the environment variables
	// for the current variant. This is called just after forking the variants
	// off from the main MVEE process, but before they have execve'd.
	//
	static void        setup_env              ();

	//
	// Starts the variant using the loaded configuration
	//
	static void        start_variant          ();

	//
	// Get the name of the SPEC2006 profile to be used to run the current
	// variant
	//
	static const char* get_spec_profile       (bool native);

	// *************************************************************************
	// Monitor management
	// *************************************************************************

	//
	// Shuts down the MVEE. This should not be called directly! Use
	// request_shutdown instead.
	// 
	static void  shutdown();

	// 
	// Runs the garbage collection loop to clean up the metadata for monitors
	// that have been shut down.
	// 
	static void  garbage_collect();

	// *************************************************************************
	// Monitor Management Variables
	// *************************************************************************

	// set to true when we've added new monitors to the garbage collection list
	static bool                                 should_garbage_collect;

	// list of monitors to be garbage collected
	static std::vector<monitor*>                dead_monitors;

	// list of active monitors
	static std::vector<monitor*>                active_monitors;

	// list of inactive monitors
	// We move monitors into this list if the variants they're monitoring
	// are suspended indefinitely (e.g., because they triggered a divergence),
	// but we don't want to kill these variants just yet.
	static std::vector<monitor*>                inactive_monitors;

	// maps every variant pid onto the set of pids it's part of
	// i.e. this would contain M[A] -> {M[A], S[A]} and also S[A] -> {M[A], S[A]}
	static std::map<pid_t, std::vector<pid_t> > variant_pid_mapping;

	// maps every monitor id onto its monitor object
	static std::map<int, monitor*>              monitor_id_mapping;

	// monitor id to be used by the next monitor we spawn
	static int                                  next_monitorid;

	// variant threads that are in the process of being transferred from one monitor to the other
	static std::vector<detachedvariant*>        detachlist;

	//
	// OS/Environment configuration
	//
	static std::string                          orig_working_dir;
	static pid_t                                process_pid;
	static __thread pid_t                       thread_pid;
	static std::map<std::string, std::string>   interp_map;

	//
	// Logging Vars
	//
	static FILE*                                logfile;
	static double                               startup_time;
	static pthread_mutex_t                      loglock;
};

#define warnf mvee::warnf

#ifdef MVEE_BENCHMARK
# ifdef __clang__
#  define debugf(...)
# else
#  define debugf(a...)
# endif
# define DEBUGVAR __attribute__((unused))
#else
# define debugf mvee::logf
# define DEBUGVAR
#endif

#endif /* MVEE_H_ */
