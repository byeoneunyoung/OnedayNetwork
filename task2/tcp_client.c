
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include<time.h>
#include<sys/time.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

void printTimestamp() {
    time_t rawtime;
    struct tm* timeinfo;
    char timestamp[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    printf("\033[0;31m[%s] \033[0m", timestamp);
}

void printTimeDifference(const struct timeval* start, const struct timeval* end) {
    long seconds = end->tv_sec - start->tv_sec;
    long microseconds = end->tv_usec - start->tv_usec;
    long milliseconds = seconds * 1000 + microseconds / 1000;
    long microseconds_remainder = microseconds % 1000;

    printf("Time Difference: %ld.%09ld milliseconds\n", milliseconds, microseconds_remainder);
}



int main(int argc, char *argv[]) {


    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }

    //printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    //printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    //printf("%s %s\n", address_buffer, service_buffer);


    //printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    //printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);
    struct timeval start_time;
    struct timeval end_time;

    printf("채팅 서버에 연결되었습니다.\n");
    //printf("To send data, enter text followed by enter.\n");

    while(1) {

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
        FD_SET(0, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }
        //받은 메시지 출력
        if (FD_ISSET(socket_peer, &reads)) {
            char read[4096];
            memset(read, 0, 4096);
            gettimeofday(&start_time, NULL);
            int bytes_received = recv(socket_peer, read, 4096, 0);
            gettimeofday(&end_time, NULL);
            
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            printTimestamp();
            printf("%s", read);
            printTimeDifference(&start_time, &end_time);
        }
        //입력으로 들어온 메시지가 없으면 break, 있으면 서버에 보내기
        if(FD_ISSET(0, &reads)) {
            char read[4096];
            memset(read, 0, 4096);
            if (!fgets(read, 4096, stdin)) break;
            printTimestamp();
            gettimeofday(&start_time, NULL);
            int bytes_sent = send(socket_peer, read, strlen(read), 0);
            gettimeofday(&end_time, NULL);
            printTimeDifference(&start_time, &end_time);
        }
    } //end while(1)

    printf("Closing socket...\n");
    CLOSESOCKET(socket_peer);

    printf("Finished.\n");
    return 0;
}

