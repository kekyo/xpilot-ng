#include "xpclient.h"

#include <stdlib.h>
#include <string.h>

int clientPortStart;
int clientPortEnd;

int main(int argc, char **argv)
{
    Connect_param_t connection;
    Connect_defaults_t defaults = {
        1,
        GAME_TRANSPORT_UDP,
        GAME_TRANSPORT_TCP
    };
    Connect_target_t *targets;
    Connect_target_status_t status;
    int invalid_index;
    int result;

    if (argc != 3)
        return 2;
    init_error(argv[0]);
    if (sock_startup() != 0)
        return 3;

    memset(&connection, 0, sizeof(connection));
    strlcpy(connection.nick_name, "TargetProbe",
            sizeof(connection.nick_name));
    strlcpy(connection.user_name, "target-probe",
            sizeof(connection.user_name));
    strlcpy(connection.host_name, "localhost",
            sizeof(connection.host_name));
    strlcpy(connection.disp_name, "test-display",
            sizeof(connection.disp_name));
    connection.team = TEAM_NOT_SET;

    status = Connect_targets_parse(2, &argv[1], &defaults, &targets,
                                   &invalid_index);
    if (status != CONNECT_TARGET_STATUS_OK) {
        sock_cleanup();
        return 4;
    }

    result = Contact_servers(2, targets, 1, 0, 0, NULL, &connection);
    free(targets);
    sock_cleanup();
    if (!result)
        return 5;
    if (connection.contact_transport != GAME_TRANSPORT_UDP
        || connection.game_transport != GAME_TRANSPORT_UDP)
        return 6;
    return 0;
}
