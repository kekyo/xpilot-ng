#include "xpclient.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int clientPortStart;
int clientPortEnd;

static int parse_port(const char *text, int *port)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0'
        || value <= 0 || value > 65535)
        return -1;
    *port = (int)value;
    return 0;
}

int main(int argc, char **argv)
{
    Connect_param_t connection;
    Connect_target_t targets[2];
    int contact_port;
    int result;

    if (argc != 3 || parse_port(argv[2], &contact_port) != 0)
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

    if (!Connect_target_init(&targets[0], argv[1], contact_port,
                             GAME_TRANSPORT_TCP, GAME_TRANSPORT_TCP)
        || !Connect_target_init(&targets[1], argv[1], contact_port,
                                GAME_TRANSPORT_UDP, GAME_TRANSPORT_UDP)) {
        sock_cleanup();
        return 4;
    }

    result = Contact_servers(2, targets, 1, 0, 0, NULL, &connection);
    sock_cleanup();
    if (!result)
        return 5;
    if (connection.contact_transport != GAME_TRANSPORT_UDP
        || connection.game_transport != GAME_TRANSPORT_UDP)
        return 6;
    return 0;
}
