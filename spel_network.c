#ifdef _WIN32
//define something for Windows (32-bit and 64-bit, this part is common)
#include <SDL_net.h>
#include <SDL.h>

#elif __APPLE__
#include <SDL2/SDL.h>
#include <SDL2_net/SDL_net.h>

#elif __linux
#endif

#include <stdio.h>
#include <stdlib.h>
#include "spel_structs.h"

threadCom sendPool;
threadCom recvPool;

int RecvThread(void* ptr);
int SendThread(void* ptr);

TCPsocket net_start(char* ip_p, char*port_p){
    IPaddress ip;		/* Server address */
    TCPsocket sd;

    char buffer[512];
    int bufferSize = 0;
    int readIndex = 0;

    if (SDLNet_Init() < 0)
    {
        fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    /* Resolve the host we are connecting to */
    if (SDLNet_ResolveHost(&ip, ip_p, atoi(port_p)) < 0)
    {
        fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }

    /* Open a connection with the IP provided (listen on the host's port) */
    if (!(sd = SDLNet_TCP_Open(&ip)))
    {
        fprintf(stderr, "SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }
    printf("Connection established!\n");

    sendPool.Size = 0;
    recvPool.Size = 0;

    SDL_Delay(100);

    SDL_CreateThread(SendThread,"Sending thread", (void*)sd);
    //SDL_Delay(10);
    SDL_CreateThread(RecvThread,"Listen thread", (void*)sd);


    SDL_Delay(100);

    return sd;
}

int ProcessMessage(char data[], Scene* scene)
{
    switch(data[0])
    {
        case NET_CHAT_MSG:
            printf("Chat message was received\n");
            break;
        case 2:
            printf("Object position was received\n");
//            net_ChangeObjectPos(data, scene);
            break;
        case 5:
            printf("New object was received\n");
//            net_NewObject(data, scene);
            break;
        case NET_PLAYER_ID:
            net_SetPlayerId(data);
            break;
        default:
            printf("Could not identify header.\n");
            break;
    }
    return 0;
}

int Converter_BytesToInt32(unsigned char data[], int* index){ // G�r om en byte-array till en int.

    int value = 0;
    value += (((int)data[*index]) << 24);
    value += (((int)data[*index + 1]) << 16);
    value += (((int)data[*index + 2]) << 8);
    value += (((int)data[*index + 3]));

    *index += 4;

    return value;
}

int Converter_Int32ToBytes(unsigned char data[], int* size, int value) //G�r om en int till en byte array.
{
    int temp = value;
    data[(*size + 3)] = (unsigned char)(value);
    data[*size + 2] = (unsigned char)((value) >> 8);
    data[*size + 1] = (unsigned char)((value) >> 16);
    data[*size] = (unsigned char)((value) >> 24);

    *size += 4;

    return 0;
}

int SendThread(void* ptr)
{
    TCPsocket sd = (TCPsocket)ptr;
    char buffer[512];

    int tempIndex = 0;

    while(1)
    {
        while(sendPool.Size <= 0){
            SDL_Delay(20);
        }
        while(sendPool.Size > 0)
        {
            ReadPool(&sendPool, buffer);
            SDLNet_TCP_Send(sd, buffer, 512);
            break;
        }
    }
    return 1;
}

int RecvThread(void* ptr) //Lyssnar efter meddelanden fr�n servern och l�gger dem i en stack som main l�ser av varje update.
{
    TCPsocket sd = (TCPsocket)ptr;
    int temp;
    int index = 0;

    char msg[512];
    while(SDLNet_TCP_Recv(sd, msg, 512))
    {
        printf("message received\n");
        AddToPool(&recvPool, msg);
    }
    return 1;
}

int AddToPool(threadCom* pool, char* msg) // Funktion f�r att l�gga till meddelanden i stacks ( pools).
{
    SDL_LockMutex(pool->mtx);
    memcpy(pool->pool[pool->Size], msg, 512);
    pool->Size++;
    SDL_UnlockMutex(pool->mtx);
    return 1;
}

int ReadPool(threadCom* pool, char* msg)
{
    SDL_LockMutex(pool->mtx);
    if(pool->Size > 0)
    {
        memcpy(msg, pool->pool[0], 512);
        for(int i = 0; i < pool->Size - 1; i++)
        {
            memcpy(pool->pool[i], pool->pool[i+1], 512);
        }
        pool->Size--;
    }

    SDL_UnlockMutex(pool->mtx);

    return 1;
}
