/*
 * XPilot Infinity, a multiplayer space war game.
 *
 * Windows Service Control Manager integration for the dedicated server.
 */

#ifndef WINDOWS_SERVICE_H
#define WINDOWS_SERVICE_H

/** Server entry point invoked after the service control handler is ready. */
typedef int (*windows_service_run_fn)(int argc, char **argv);

/**
 * Check whether the internal Windows service argument is present.
 *
 * @param argc Command-line argument count.
 * @param argv Command-line arguments.
 * @return true when the process should connect to the Windows SCM.
 */
bool Windows_service_requested(int argc, char **argv);

/**
 * Connect the process to the Windows SCM and run the server as a service.
 *
 * The internal service and logging arguments are removed before @p run_server
 * receives the remaining server options.
 *
 * @param argc Command-line argument count.
 * @param argv Command-line arguments.
 * @param run_server Server entry point to invoke from ServiceMain.
 * @return Zero after a clean service shutdown, or nonzero on failure.
 */
int Windows_service_dispatch(int argc, char **argv,
			     windows_service_run_fn run_server);

/** Report that server initialization completed and controls can be accepted. */
void Windows_service_report_running(void);

/** Mark an orderly server shutdown as a successful service exit. */
void Windows_service_prepare_clean_exit(void);

#endif /* WINDOWS_SERVICE_H */
