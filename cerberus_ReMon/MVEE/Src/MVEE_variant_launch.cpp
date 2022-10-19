/*
 * Cerberus PKU-based Sandbox
 *
 * Check Cerberus/cerberus_ReMon/README.md for licensing terms.
 */

/*-----------------------------------------------------------------------------
    Includes
-----------------------------------------------------------------------------*/
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include "MVEE.h"
#include "MVEE_monitor.h"

/*-----------------------------------------------------------------------------
    parse_and_setenv - 
-----------------------------------------------------------------------------*/
static void parse_and_setenv(const std::string& env)
{
	size_t pos = env.find('=');
	if (pos != std::string::npos) {
		std::string key   = env.substr(0, pos);
		std::string value = env.substr(pos + 1);

		if (value.length() > 0) {
			const char* oldenv = getenv(key.c_str());
			if (oldenv)
				setenv(key.c_str(), (value + ":" + oldenv).c_str(), 1);
			else
				setenv(key.c_str(), value.c_str(), 1);
		}
		else {
			unsetenv(key.c_str());
		}
	}
}

/*-----------------------------------------------------------------------------
    setup_env - sets up the environment for the variant to run in.

    This code is executed by the variants, BEFORE the monitor is attached
-----------------------------------------------------------------------------*/
void mvee::setup_env()
{
	// needed by LD_Loader and SPEC scripts
	setenv("MVEEROOT", os_get_mvee_root_dir().c_str(), 1);

	// per-variant env variables take precedence over global env vars
	if (!mvee::config["variant"]["specs"] ||
		!mvee::config["variant"]["specs"][mvee::variant_ids[0]] ||
		!mvee::config["variant"]["specs"][mvee::variant_ids[0]]["env"])
	{
		for (auto envp : (*mvee::config_variant_exec)["env"])
			parse_and_setenv(envp.asString());
	}
	else {
		for (auto envp : mvee::config["variant"]["specs"][mvee::variant_ids[0]]["env"])
			parse_and_setenv(envp.asString());
	}
}

/*-----------------------------------------------------------------------------
    start_variant
-----------------------------------------------------------------------------*/
void mvee::start_variant()
{
	std::deque<const char*> args;
	Json::Value* variant_config = NULL;

	// See if we have a variant-specific config that might contain program args
	if (!mvee::config["variant"]["specs"].isNull() &&
		!mvee::config["variant"]["specs"][mvee::variant_ids[0]].isNull())
		variant_config = &mvee::config["variant"]["specs"][mvee::variant_ids[0]];

	// per-variant argvs take precedence over global argvs
	if (!variant_config || !(*variant_config)["argv"]) {
		for (auto arg : (*mvee::config_variant_exec)["argv"])
			args.push_back(mvee::strdup(arg.asCString()));
	}
	else {
		for (auto arg : (*variant_config)["argv"])
			args.push_back(mvee::strdup(arg.asCString()));
	}
	args.push_back(nullptr);

	// get absolute path of the binary to start
	std::string binary;
	if (!variant_config || !(*variant_config)["path"])
		binary = (*mvee::config_variant_exec)["path"].asString();
	else
		binary = (*variant_config)["path"].asString();

	// this might be a relative path. Get the full path
	binary = os_normalize_path_name(binary);

	// push the basename of the original binary name as argv[0]
	size_t pos = binary.rfind('/');
	if (pos != std::string::npos)
		args.push_front(mvee::strdup(binary.substr(pos+1).c_str()));
	else
		args.push_front(mvee::strdup(binary.c_str()));

	// Build arg array
	const char** _args = new const char*[args.size()];
	int i = 0;
	for (auto _arg : args)
		_args[i++] = _arg;

	// change to the variant's specified working directory (if any)
	if (variant_config && !(*variant_config)["pwd"].isNull())
		chdir((*variant_config)["pwd"].asCString());

	// this should not return
	execv(binary.c_str(), (char* const*)_args);

	printf("ERROR: Failed to start variant: %s (argv: [", binary.c_str());
	i = 0;
	for (auto _arg : args) {
		if (i++ > 0) printf(", ");
		printf("%s", _arg);
	}
	printf("])\n");
}
