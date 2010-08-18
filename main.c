#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <util.h>
#include <string.h>
#include <time.h>

/* Sourced http://aprs.gids.nl/nmea/
 * eg3. $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
 * 1    = UTC of Position
 * 2    = Latitude
 * 3    = N or S
 * 4    = Longitude
 * 5    = E or W
 * 6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
 * 7    = Number of satellites in use [not those in view]
 * 8    = Horizontal dilution of position
 * 9    = Antenna altitude above/below mean sea level (geoid)
 * 10   = Meters  (Antenna height unit)
 * 11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
 *        mean sea level.  -=geoid is below WGS-84 ellipsoid)
 *        12   = Meters  (Units of geoidal separation)
 *        13   = Age in seconds since last update from diff. reference station
 *        14   = Diff. reference station ID#
 *        15   = Checksum
 */
static int format_gpgga(char*buf, size_t max_size) {
    char time_buf[32];
    time_t current_time = time(NULL); 
    strftime(time_buf, sizeof(time_buf), "%H%M%S", localtime(&current_time));
    snprintf(buf, max_size, "$GPGGA,%s.00,MORE-TO-COME\r\n", time_buf);
    return 1;
}

int main(int argc, char *argv[])
{
    int pt;
    struct fd_set master_set, working_set, error_set;
    struct timeval timeout;
    char buffer[512];
    struct termios tio;

    pt = open("/dev/ptmx", O_RDWR | O_NOCTTY);

    if (pt < 0)
    {
        perror("open /dev/ptmx");
        return 1;
    }

    grantpt(pt);
    unlockpt(pt);

    fprintf(stderr, "Slave device: %s\n", ptsname(pt));

    if (tcgetattr(pt, &tio) < 0) {
        perror("tcgetattr() failed");
        return 1;
    }

    /* disable echo etc */
    tio.c_lflag &= ~(ECHO|ICANON);

    if (tcsetattr(pt, TCSAFLUSH, &tio) < 0) {
        perror("tcsetattr() failed");
        return 1;
    }

    FD_ZERO(&master_set);
    FD_SET(pt, &master_set);
    do {
        int rc = 0;

        memcpy(&working_set, &master_set, sizeof(master_set));
        memcpy(&error_set, &master_set, sizeof(master_set));
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        rc = select(pt+1, &working_set, NULL, &error_set, &timeout);

        if (rc < 0) {
            perror("select() failed");
            break;
        }

        if (rc == 0) {
            /* Timeout */
            printf("WRITING:");
            format_gpgga(buffer, sizeof(buffer));
            rc = write(pt, buffer, strlen(buffer), strlen(buffer));
            if (rc < 0) {
                perror("write() failed");
                break;
            }
            continue;
        } 

        if (FD_ISSET(pt, &error_set)) {
            perror("error running select()");
            break;
        }
        
        if (FD_ISSET(pt, &working_set)) {
            rc = read(pt, buffer, sizeof(buffer) - 1);
            if (rc < 0) {
                perror("read() failed");
                break;
            } else {
                printf("READING:");
                buffer[rc] = '\0';
                printf("%s", buffer);
            }
            memset(buffer, '\0', sizeof(buffer));
        }

    } while (1);
    return 0;
}

