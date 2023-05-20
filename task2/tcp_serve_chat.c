
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

//모든 클라이언트에게 메시지를 보내는 함수
void broadcastMessage(SOCKET sender, SOCKET listener, const char* message, fd_set* master, SOCKET max_socket) {
    SOCKET i;
    for (i = 1; i <= max_socket; ++i) {
        if (FD_ISSET(i, master)) {
            //if ( i != sender && i != listener )
                send(i, message, strlen(message), 0);
        }
    }
}

int main() {

    //printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);


    //create socket
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


   //socket binding
    if (bind(socket_listen,
            bind_address->ai_addr, bind_address->ai_addrlen)) {
            fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
            freeaddrinfo(bind_address);
            return 1;
    }



    //listening
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        freeaddrinfo(bind_address);
        return 1;
    }

    freeaddrinfo(bind_address);

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf(">>Server Start\n");

    //server start
    while(1) {

        time_t currentTime = time(NULL);
		struct tm *localTime = localtime(&currentTime);
		char datetime[20];
		strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localTime);

        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {

                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[1000];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    //client와 연결 후 해당 클라이언트를 제외한 모든 연결된 클라이언트에게 참여 알림 보내기
                    printf(">>Accept connection From Client\n");
                    char join_message[1000];
                    memset(join_message, 0, 1000);

                    snprintf(join_message, sizeof(join_message), "사용자 %d님이 입장하셨습니다.\n", socket_client - 3);
                    broadcastMessage(socket_client, socket_listen, join_message, &master, max_socket);
                }
                else {
                    //클라이언트로부터의 메시지 저장하고 서버에 출력
                    char read[1024];
                    memset(read, 0, 1024);
                    int bytes_received = recv(i, read, 1024, 0);
                    printf("From Client: 사용자 %d: %s", i-3, read);
                    
                    if (bytes_received < 1) {
                        printf("leave user: 사용자 %d\n", i-3);
                      
                        //퇴장한해당 클라이언트를 제외한 모든 연결된 클라이언트에게 퇴장 알림 보내기
                        char leave_message[1000];
                        memset(leave_message, 0, 1000);
                        snprintf(leave_message, sizeof(leave_message), "사용자 %d님이 대화방을 나갔습니다.\n", i - 3);
                        broadcastMessage(i, socket_listen, leave_message, &master, max_socket);
                        
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
		                continue;
                    }
                    //client와 연결 후 해당 클라이언트를 제외한 모든 연결된 클라이언트에게 메시지 보내기
                    char Msg[2000];
                    memset(Msg, 0, 2000);
					sprintf(Msg, "[ %s ] 사용자 %d: %s", datetime, i - 3, read);
                    broadcastMessage(i, socket_listen, Msg, &master, max_socket);
                    
                }
            } //if FD_ISSET
        } //for i to max_socket
    } //while(1)



    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

    printf("Finished.\n");

    return 0;
}
