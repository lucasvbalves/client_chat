/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <arpa/inet.h>

#define MAXDATASIZE 2000 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd;             //socket file descriptor
    int numbytes;
    int string_length;
    int bytes_sent;
    
    int rv;
    int i;
    char comando[MAXDATASIZE];
    char buf_recv[MAXDATASIZE];
    char buf_send[MAXDATASIZE];
    char ip_string[INET6_ADDRSTRLEN];   //STRING COM IP. TAMANHO DE IPv6.
    struct addrinfo hints;
    
    //ponteiro para lista encadeada de "struct addrinfo" com informacoes
    struct addrinfo *servinfo;  
    struct addrinfo *p;

    struct timeval timeout;
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;


    

    /*  PROTOTIPO DE "struct addrinfo"

    struct addrinfo {
        int ai_flags;       // AI_PASSIVE, AI_CANONNAME, ...
        int ai_family;      // AF_xxx
        int ai_socktype;    // SOCK_xxx
        int ai_protocol;    // 0 (auto) or IPPROTO_TCP, IPPROTO_UDP 

        socklen_t ai_addrlen;       // length of ai_addr
        char *ai_canonname;         // canonical name for nodename
        struct sockaddr *ai_addr;   // binary address
        struct addrinfo *ai_next;   // next structure in linked list
    };
    */    
    
    //VERIFICACAO DO NUMERO DE ARGUMENTOS PASSADOS
    if (argc != 4) {
        fprintf(stderr,"Uso: client_chat <CLIENT_NAME> <SERVER_ADDRESS> <SERVER_PORT>\n");
        exit(1);
    }
    
    memset(&hints, 0, sizeof hints);    //PREENCHE STRUCT "HINTS" COM ZERO
    hints.ai_family = AF_UNSPEC;        //IPv4 OU IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP STREM SOCKET
    
    //ARGV[1] = CLIENT_NAME
    //ARGV[2] = SERVER_ADDRESS
    //ARGV[3] = SERVER_PORT
    //VERIFICACAO DE GETADDRINFO
    
    rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo);    
    if (rv != 0) {
        fprintf(stderr, "ERRO (getaddrinfo): %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        
        // sockfd == -1 erro
        if (sockfd == -1) {
            perror("ERRO (socket)");
            continue;
        }

        // connect retorna -1 eh erro
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("ERRO (connect)");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "ERRO: client_chat falhou em conectar\n");
        return 2;
    }
    else{
        printf("Conectado com sucesso\n");
    }
    
    
    freeaddrinfo(servinfo); // all done with this structure
   
    
    while(1) {

        // clear the set ahead of time
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);


        // add our descriptors to the set
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &exceptfds);
        FD_SET(STDIN_FILENO, &exceptfds);


        rv = select(sockfd+1, &readfds, NULL, &exceptfds, NULL);

        if (rv == -1) {
            
            perror("ERRO (select)"); // error occurred in select()
            
        } else if (rv == 0) {
            
            printf("ERRO: Timeout ocorreu\n");\
            
        } else {
            // one or both of the descriptors have data
            if (FD_ISSET(sockfd, &readfds)) {

                if ((numbytes = recv(sockfd, buf_recv, MAXDATASIZE - 1, 0)) == -1) {

                    perror("ERRO (recv)");
                    exit(1);

                } else if (numbytes == 0) {

                    printf("ERRO (recv): Servidor fechou conexão\n");
                    exit(1);

                } else {
                    
                    buf_recv[numbytes] = '\0';
                    printf("client: received '%s'\n", buf_recv);

                }
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {

                fgets(buf_send, sizeof buf_send, stdin);
                
                // limpar enter do fgets
                string_length = strlen(buf_send);
                buf_send[string_length-1] = '\0';
                //printf("stdin read:'%s'\n", buf_send);
                
                i = 0;
                while (buf_send[i] != ' ') {

                    comando[i] = toupper(buf_send[i]);
                    i++;

                }
                
                printf("comando: %s\n", comando);

                if (strcmp(comando, "SEND") == 0) {
                    
                    strcat(argv[1], ": ");
                    strcat(argv[1], buf_send);
                    strcpy(buf_send, argv[1]);

                    //sprintf(buf_send, "%s: %s", argv[1], buf_send);
                    
                    printf("enviado pro servidor -> %s\n", buf_send);

                    string_length = strlen(buf_send);

                    bytes_sent = send(sockfd, buf_send, string_length, 0);

                    printf("executar comando %s\n", comando);


                } else if (strcmp(comando, "SENDTO") == 0) {

                    printf("executar comando %s\n", comando);


                } else if (strcmp(comando, "WHO") == 0) {

                    printf("executar comando %s\n", comando);

                } else if (strcmp(comando, "HELP") == 0) {

                    printf("========== COMANDOS SUPORTADOS ==========\n");
                    printf("\tSEND <MESSAGE>\n");
                    printf("\tSENDTO <CLIENT_NAME> <MESSAGE>\n");
                    printf("\tWHO\n");
                    printf("\tHELP\n");
                    printf("=========================================\n");

                } else {

                    printf("ERRO: Comando não suportado\n");
                    printf("Comando HELP para lista de comandos suportados\n");
                    
                }

            }


            /*
            if (FD_ISSET(sockfd, &exceptfds)) {
                recv(s2, buf2, sizeof buf2, 0);
            }*/
        }
    }

    
    /*  PROTOTIPO
     const char *inet_ntop(int address_family, const void *src, char *dst, socklen_t size);
     */
    /*    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), ip_string, sizeof ip_string);
    printf("client_chat: Conectando em %s\n", ip_string);
     
     */

    return 0;
}