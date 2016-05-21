/*
 ** client_chat.c
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
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd; // Socket file descriptor
    int bytes_recv; // Número de bytes recebidos
    int string_length; // Comprimento da string
    int bytes_sent; // Número de bytes enviados
    int ret_val; // Valor de retorno de funções
    int i; // Contador para reconhecimento do comando

    char comando[7]; // String para armazenar comando
    char buf_recv[MAXDATASIZE]; // String para armazenar buffer de recepção
    char buf_send[MAXDATASIZE]; // String para armazenar buffer de envio
    char buf_stdin[MAXDATASIZE]; // String para armazenar buffer do terminal

    struct addrinfo hints;
    //ponteiro para lista encadeada de "struct addrinfo" com informacoes
    struct addrinfo *servinfo; // Ponteiro para estrutura com informações do server
    struct addrinfo *p;
    fd_set readfds; // Conjunto de file descriptors para monitorar leitura


    //VERIFICACAO DO NUMERO DE ARGUMENTOS PASSADOS
    if (argc != 4) {
        fprintf(stderr, "Uso: client_chat <CLIENT_NAME> <SERVER_ADDRESS> <SERVER_PORT>\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // Preenche struct "hints" com zero
    hints.ai_family = AF_UNSPEC; //IPv4 OU IPv6
    hints.ai_socktype = SOCK_STREAM; // Especifica tipo do socket: TCP stream socket

    //ARGV[1] = CLIENT_NAME
    //ARGV[2] = SERVER_ADDRESS
    //ARGV[3] = SERVER_PORT
    //VERIFICACAO DE GETADDRINFO

    ret_val = getaddrinfo(argv[2], argv[3], &hints, &servinfo);
    if (ret_val != 0) {
        fprintf(stderr, "ERRO (getaddrinfo): %s\n", gai_strerror(ret_val));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {

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
    } else { //conectado. enviar client_name para server

        strcpy(buf_send, argv[1]);
        string_length = strlen(buf_send);
        bytes_sent = send(sockfd, buf_send, string_length, 0);
        if (bytes_sent == -1) {
            perror("send");
        }

        if ((bytes_recv = recv(sockfd, buf_recv, MAXDATASIZE - 1, 0)) == -1) {

            perror("ERRO (recv)");
            exit(1);

        } else if (bytes_recv == 0) {

            printf("ERRO (recv): Servidor fechou conexão\n");
            exit(1);

        } else {

            if (strcmp(buf_recv, "Conectado com sucesso") == 0) {

                printf("%s\n", buf_recv);

            } else {

                printf("%s\n", buf_recv);
                exit(1);

            }
        }

    }

    freeaddrinfo(servinfo); // all done with this structure    

    while (1) {

        // clear the set ahead of time
        FD_ZERO(&readfds);


        // add our descriptors to the set
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);


        ret_val = select(sockfd + 1, &readfds, NULL, NULL, NULL);

        if (ret_val == -1) {

            perror("ERRO (select)"); // error occurred in select()

        } else {
            // one or both of the descriptors have data
            if (FD_ISSET(sockfd, &readfds)) {

                if ((bytes_recv = recv(sockfd, buf_recv, MAXDATASIZE - 1, 0)) == -1) {

                    perror("ERRO (recv)");
                    exit(1);

                } else if (bytes_recv == 0) {

                    printf("ERRO (recv): Servidor fechou conexão\n");
                    exit(1);

                } else {

                    buf_recv[bytes_recv] = '\0';
                    printf("%s\n", buf_recv);

                }
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {

                fgets(buf_stdin, sizeof buf_stdin, stdin);

                // limpar enter do fgets
                string_length = strlen(buf_stdin);
                buf_stdin[string_length - 1] = '\0';

                i = 0;
                while (buf_stdin[i] != ' ' && i < 6) {

                    comando[i] = toupper(buf_stdin[i]);
                    i++;

                }

                if (strcmp(comando, "SEND") == 0 || strcmp(comando, "SENDTO") == 0) {

                    strcpy(buf_send, buf_stdin);

                    string_length = strlen(buf_send);

                    bytes_sent = send(sockfd, buf_send, string_length, 0);

                    if (bytes_sent == -1) {
                        perror("send");
                    }


                } else if (strcmp(comando, "WHO") == 0) {

                    strcpy(buf_send, comando);

                    string_length = strlen(buf_send);

                    bytes_sent = send(sockfd, buf_send, string_length, 0);

                    if (bytes_sent == -1) {
                        perror("send");
                    }

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

                memset(comando, '\0', sizeof comando);

            }

        }
    }

    return 0;
}