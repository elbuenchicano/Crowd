/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    CLIENTSERVER.CPP: Defines classes for a client/service networking scheme

    Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

#define DELTA_PEERS   10  // The buffer of peers is augmented 10 itens in each reallocation


///////////////////////////////////////////////////////////////////////////////
/////////////// Methods for gcgINETPEER  //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Internal struct: needed to free the gcg.h from depending on socket.
typedef struct _GCG_INETPEER {
  SOCKET          socket;       // Peer socket
} GCG_INETPEER;

gcgINETPEER::gcgINETPEER() {
  this->handle = (GCG_INETPEER*) ALLOC(sizeof(GCG_INETPEER));
  if(handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgINETPEER(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  // Connection information
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  peer->socket = 0;

  // Object atributes
  memset(&this->local_ip, 0, sizeof(this->local_ip));
  this->isConnected = false;
  this->local_port = 0;
  memset(&this->remote_ip, 0, sizeof(this->remote_ip));
  this->remote_port = 0;

  // No error
}

gcgINETPEER::~gcgINETPEER() {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(peer) {
    this->disconnect();
    SAFE_FREE(handle);
  }
  // No error
}

bool gcgINETPEER::connectToHost(const char *host, int port) {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "connectToHost(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(isConnected) {
    this->disconnect();
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_NETWORK, GCG_DISCONNECTED), "connectToHost(): current connection to %d.%d.%d.%d:%d is closed. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
  }

  // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  // Initialize Winsock
  WSADATA wsaData;
  memset((void*) &wsaData, 0, sizeof(wsaData));
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  // Try to connect...
  try {
    // The socket() function returns a socket descriptor representing an endpoint. The statement also
    // identifies that the INET (Internet Protocol) address family with the TCP transport (SOCK_STREAM)
    // will be used for this socket.
    if((peer->socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SOCKETERROR), "connectToHost(): call to socket() failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return false;
    }

    struct sockaddr_in serviceaddr;
    memset(&serviceaddr, 0, sizeof(struct sockaddr_in));
    serviceaddr.sin_family = AF_INET;
    serviceaddr.sin_port = htons((u_short) port);

    if((serviceaddr.sin_addr.s_addr = inet_addr(host)) == (unsigned long) INADDR_NONE) {
      // When passing the host name of the service as a parameter to this constructor, use
      // the gethostbyname() function to retrieve the address of the host service.
      struct hostent *hostp;
      hostp = gethostbyname(host);
      if(hostp == (struct hostent*) NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_HOSTNOTFOUND), "connectToHost(): could not find host %s. (%s:%d)", host, basename((char*)__FILE__), __LINE__);
        return false;
      }
      memcpy(&serviceaddr.sin_addr, hostp->h_addr, sizeof(serviceaddr.sin_addr));
    }

    // After the socket descriptor is received, the connect() function is used to establish a
    // connection to the service.
    if(connect(peer->socket, (struct sockaddr*) &serviceaddr, sizeof(serviceaddr)) < 0) {
// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      switch(WSAGetLastError()) {
        case WSAECONNREFUSED:  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONREFUSED), "connectToHost(): host %s refused connection. (%s:%d)", host, basename((char*)__FILE__), __LINE__);
                               break;
        case WSAETIMEDOUT:     gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_TIMEOUT), "connectToHost(): time is out trying to connect to host %s. (%s:%d)", host, basename((char*)__FILE__), __LINE__);
                               break;
        default:               gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTERROR), basename((char*)__FILE__), __LINE__);
                               break;
      };
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      switch(errno) {
        case ECONNREFUSED:     gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONREFUSED), "connectToHost(): host %s refused connection. (%s:%d)", host,  basename((char*)__FILE__), __LINE__);
                               break;
        case ETIMEDOUT:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_TIMEOUT), "connectToHost(): time is out trying to connect to host %s. (%s:%d)", host, basename((char*)__FILE__), __LINE__);
                               break;
        default:               gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTERROR), basename((char*)__FILE__), __LINE__);
                               break;
      };
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

      return false;
    } else isConnected = true;

// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
    // Transfer remote address information
    remote_ip[0] = serviceaddr.sin_addr.S_un.S_un_b.s_b1;
    remote_ip[1] = serviceaddr.sin_addr.S_un.S_un_b.s_b2;
    remote_ip[2] = serviceaddr.sin_addr.S_un.S_un_b.s_b3;
    remote_ip[3] = serviceaddr.sin_addr.S_un.S_un_b.s_b4;
    remote_port  = ntohs(serviceaddr.sin_port);

    // Transfer local addres information
    struct sockaddr_in addr;
    int l = sizeof(addr);
    if(getsockname(peer->socket, (struct sockaddr*) &addr, (socklen_t*) &l) == 0) {
      local_ip[0] = addr.sin_addr.S_un.S_un_b.s_b1;
      local_ip[1] = addr.sin_addr.S_un.S_un_b.s_b2;
      local_ip[2] = addr.sin_addr.S_un.S_un_b.s_b3;
      local_ip[3] = addr.sin_addr.S_un.S_un_b.s_b4;
      local_port  = ntohs(addr.sin_port);
    }
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
    // Transfer remote address information
    remote_ip[0] = ((unsigned char*) &serviceaddr.sin_addr.s_addr)[0];
    remote_ip[1] = ((unsigned char*) &serviceaddr.sin_addr.s_addr)[1];
    remote_ip[2] = ((unsigned char*) &serviceaddr.sin_addr.s_addr)[2];
    remote_ip[3] = ((unsigned char*) &serviceaddr.sin_addr.s_addr)[3];
    remote_port  = ntohs(serviceaddr.sin_port);

    // Transfer local addres information
    struct sockaddr_in addr;
    int l = sizeof(addr);
    if(getsockname(peer->socket, (struct sockaddr*) &addr, (socklen_t*) &l) == 0) {
      local_ip[0] = ((unsigned char*) &addr.sin_addr.s_addr)[0];
      local_ip[1] = ((unsigned char*) &addr.sin_addr.s_addr)[1];
      local_ip[2] = ((unsigned char*) &addr.sin_addr.s_addr)[2];
      local_ip[3] = ((unsigned char*) &addr.sin_addr.s_addr)[3];
      local_port  = ntohs(addr.sin_port);
    }
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  } catch(...) {
    // An exception occured
    disconnect();
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "send(): exception occured while connecting. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // No error
  return true;
}

bool gcgINETPEER::send(void *buffer, unsigned int nbytes) {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;

  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "send(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(!isConnected) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_MEMORY, GCG_NOTCONNECTED), "send(): attempt to access a disconnected gcgINETPEER. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Try to send the message
  try {
    if(::send(peer->socket, (const char*) buffer, nbytes, MSG_NOSIGNAL) < 0) {

// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      switch(WSAGetLastError()) {
        case WSAENOBUFS:      disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "send(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                              break;
        case WSAECONNRESET:   disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), "send(): connection closed by %d.%d.%d.%d. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], basename((char*)__FILE__), __LINE__);
                              break;
        case WSAECONNABORTED: disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), "send(): connection with %d.%d.%d.%d aborted. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], basename((char*)__FILE__), __LINE__);
                              break;
        case WSAEFAULT:       disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEBADADDRESS), basename((char*)__FILE__), __LINE__);
                              break;
        case WSAESHUTDOWN:    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SENDSHUTDOWN), "send(): do not send messages to %d.%d.%d.%d after calling shutdownSend(). (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], basename((char*)__FILE__), __LINE__);
                              break;
        default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEERROR), basename((char*)__FILE__), __LINE__);
                              break;
      };
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      switch(errno) {
        case ENOBUFS:         disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "send(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                              break;
        case EPIPE:
        case ECONNRESET:      disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), "send(): connection closed by %d.%d.%d.%d. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], basename((char*)__FILE__), __LINE__);
                              break;
        case ECONNABORTED:    disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), "send(): connection with %d.%d.%d.%d aborted. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], basename((char*)__FILE__), __LINE__);
                              break;
        case EFAULT:          disconnect();
                              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEBADADDRESS), basename((char*)__FILE__), __LINE__);
                              break;
        case ESHUTDOWN:       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SENDSHUTDOWN), "send(): do not send messages to %d.%d.%d.%d after calling shutdownSend(). (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], basename((char*)__FILE__), __LINE__);
                              break;
        default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEERROR), basename((char*)__FILE__), __LINE__);
                              break;
      };
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
      return false;
    }
  } catch(...) {
    // An exception occured
    disconnect();
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "send(): exception occured while sending a message. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // No error
  return true;
}

int gcgINETPEER::receive(void *buffer, unsigned int nbytes) {
  int  received;
  unsigned int total = 0;
  char *temp = (char*) buffer;

  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "receive(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return total;
  }

  if(!isConnected) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_NOTCONNECTED), "receive(): attempt to access a disconnected gcgINETPEER. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return total;
  }

  // Try to receive bytes
  try {
    do {
      // Computing the amount of data we still have to receive
      int thisIteration = nbytes - total;

// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      if((received = recv(peer->socket, (char*) temp, thisIteration, 0)) <= 0) {
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      if((received = recv(peer->socket, (void*) temp, thisIteration, 0)) <= 0) {
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

        // Got an error or connection closed by client
        if(received == 0) {
          disconnect();
          if(nbytes != total)
            gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), "receive(): peer %d.%d.%d.%d:%d has disconnected while receiving.  Bytes received: %d/%d. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, total, nbytes, basename((char*)__FILE__), __LINE__);
        } else {
          // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
          switch(WSAGetLastError()) {
            case WSAENOBUFS:      disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "receive(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAECONNRESET:   disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), "receive(): connection closed by %d.%d.%d.%d:%d. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAECONNABORTED: disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), "receive(): connection with %d.%d.%d.%d:%d aborted. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAEFAULT:       disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_READBADADDRESS), basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAESHUTDOWN:    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_RECEIVESHUTDOWN), "receive(): do not read messages from %d.%d.%d.%d:%d after calling shutdownReceive(). (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
                                  break;
            default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_READERROR), basename((char*)__FILE__), __LINE__);
                                  break;
          };
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
          switch(errno) {
            case ENOBUFS:         disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "receive(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                  break;
            case ECONNRESET:      disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), "receive(): connection closed by %d.%d.%d.%d:%d. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
                                  break;
            case ECONNABORTED:    disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), "receive(): connection with %d.%d.%d.%d:%d aborted. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
                                  break;
            case EFAULT:          disconnect();
                                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_READBADADDRESS), basename((char*)__FILE__), __LINE__);
                                  break;
            case ESHUTDOWN:       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_RECEIVESHUTDOWN), "receive(): do not read messages from %d.%d.%d.%d:%d after calling shutdownReceive(). (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
                                  break;
            default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_READERROR), basename((char*)__FILE__), __LINE__);
                                  break;
          };
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
        }
        return total; // Connection closed: returns the received bytes
      } else {
        total += received;
        temp = temp + received;
      }
    } while(total < nbytes);
  } catch(...) {
    // An exception occured
    disconnect();
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "receive(): exception occured while receiving a message from %d.%d.%d.%d:%d. (%s:%d)", this->remote_ip[0], this->remote_ip[1], this->remote_ip[2], this->remote_ip[3], this->remote_port, basename((char*)__FILE__), __LINE__);
    return total;
  }

  // Returns the total number of received bytes
  // No error
  return total;
}


bool gcgINETPEER::getConnectionFrom(gcgINETPEER *oldpeerobject) {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getConnectionFrom(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check source
  if(!oldpeerobject) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "getConnectionFrom(): parameter oldpeerobject is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }
  if(!oldpeerobject->isConnected) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "getConnectionFrom(): parameter oldpeerobject is not connected. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Copy connection information
  GCG_INETPEER *oldpeer = (GCG_INETPEER*) oldpeerobject->handle;

  peer->socket = oldpeer->socket;
  this->isConnected = true;
  oldpeerobject->isConnected = false;

  memmove(this->local_ip, oldpeerobject->local_ip, sizeof(oldpeerobject->local_ip));
  this->local_port = oldpeerobject->local_port;
  memmove(this->remote_ip , oldpeerobject->remote_ip , sizeof(oldpeerobject->remote_ip));
  this->remote_port = oldpeerobject->remote_port;

  // No error
  return true;
}

bool gcgINETPEER::shutdownSend() {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "shutdownSend(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

// Platform specific code
// Assures the all remaining data is sent/received
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  shutdown(peer->socket, SD_SEND);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  shutdown(peer->socket, SHUT_WR);
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  // No error
  return true;
}

bool gcgINETPEER::shutdownReceive() {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "shutdownReceive(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

// Platform specific code
// Assures the all remaining data is sent/received
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  shutdown(peer->socket, SD_RECEIVE);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  shutdown(peer->socket, SHUT_RD);
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  // No error
  return true;
}

bool gcgINETPEER::disconnect() {
  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "disconnect(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(isConnected) {
// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
    // Assures all remaining data is sent/received
    shutdown(peer->socket, SD_BOTH);
    closesocket(peer->socket);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
    // Assures all remaining data is sent/received
    shutdown(peer->socket, SHUT_RDWR);
    close(peer->socket); // Close socket descriptor from client side
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
  }

  peer->socket = 0;
  isConnected = false;

  // No error;
  return true;
}

int gcgINETPEER::waitEvent(bool checkReceive, bool checkSend, int timeoutUsec) {
  fd_set  read_fds;
  fd_set  write_fds;
  fd_set  except_fds;
  struct timeval timeout;

  GCG_INETPEER *peer = (GCG_INETPEER*) handle;
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "waitEvent(): attempt to use an invalid GCG_INETPEER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return GCG_EVENT_NOTREADYORDISCONNECTED;
  }

  if(!isConnected) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_NOTCONNECTED), "waitEvent(): this object is not connected. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return GCG_EVENT_NOTREADYORDISCONNECTED;
  }

  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&except_fds);
  FD_SET(peer->socket, &except_fds);

  if(checkReceive) FD_SET(peer->socket, &read_fds);
  if(checkSend) FD_SET(peer->socket, &write_fds);

  timeout.tv_sec = 0;
  timeout.tv_usec = timeoutUsec;

  // Indefinitely waits for an event
  try {
    // Waits for reading events and for exception events
    int retsel;
    if((retsel = select(FD_SETSIZE, ((checkReceive) ? &read_fds : NULL), ((checkSend) ? &write_fds : NULL), &except_fds, ((timeoutUsec < 0) ? NULL : &timeout))) == SOCKET_ERROR) {
      // Some fatal error occurred...

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      switch(WSAGetLastError()) {
        case WSAENOBUFS:      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "waitEvent(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                              break;
        case WSAECONNRESET:   gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), basename((char*)__FILE__), __LINE__);
                              break;
        case WSAECONNABORTED: gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), basename((char*)__FILE__), __LINE__);
                              break;
        case WSAEFAULT:       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEBADADDRESS), basename((char*)__FILE__), __LINE__);
                              break;
        case WSAEINTR:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INTERRUPTED), "waitEvent(): interrupted during select(). (%s:%d)", basename((char*)__FILE__), __LINE__);
                              break;
        case WSAENOTSOCK:     gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INVALIDSOCKET), basename((char*)__FILE__), __LINE__);
                              break;
        default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SELECTERROR), basename((char*)__FILE__), __LINE__);
                              break;
      };
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      switch(errno) {
        case ENOBUFS:         gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "waitEvent(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                              break;
        case EPIPE:
        case ECONNRESET:      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), basename((char*)__FILE__), __LINE__);
                              break;
        case ECONNABORTED:    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), basename((char*)__FILE__), __LINE__);
                              break;
        case EFAULT:          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEBADADDRESS), basename((char*)__FILE__), __LINE__);
                              break;
        case EINTR:           gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INTERRUPTED), "waitEvent(): interrupted during select(). (%s:%d)", basename((char*)__FILE__), __LINE__);
                              break;
        case ENOTSOCK:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INVALIDSOCKET), basename((char*)__FILE__), __LINE__);
                              break;
        default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SELECTERROR), basename((char*)__FILE__), __LINE__);
      };
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
      disconnect();
      return GCG_EVENT_NOTREADYORDISCONNECTED;
    } else
        if(FD_ISSET(peer->socket, &except_fds)) {
          // Connection problem
          disconnect();
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "waitEvent(): connection problem. Peer disconnected. (%s:%d)", basename((char*)__FILE__), __LINE__);
          return GCG_EVENT_NOTREADYORDISCONNECTED;
        }

    // Check timeout
    if(retsel == 0) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_TIMEOUT), "waitEvent(): time is out while waiting for a network event. (%s:%d)", basename((char*)__FILE__), __LINE__);
    else {
      // An expected event occured
      int eventcode = 0;
      if(checkReceive && FD_ISSET(peer->socket, &read_fds)) eventcode |= GCG_EVENT_RECEIVEREADY;
      if(checkSend && FD_ISSET(peer->socket, &write_fds)) eventcode |= GCG_EVENT_SENDREADY;

      // No error
      return eventcode;
    }
  } catch(...) {
      // An exception occurred
      disconnect();
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "waitEvent(): connection problem. Peer disconnected. (%s:%d)", basename((char*)__FILE__), __LINE__);
  }

  return GCG_EVENT_NOTREADYORDISCONNECTED;
}


///////////////////////////////////////////////////////////////////////////////
/////////////// Methods for gcgINETSERVICE  /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Internal struct: needed to free the gcg.h from depending on socket.
typedef struct _GCG_SERVICEITEM {
  gcgINETPEER     *peer;
  SOCKET          socket;               // This copy assures we won't need to access the peer memory location again
  int             checkevents;          // Events to be checked for this peer
  int             events;               // Events to be processed for this peer
  pthread_mutex_t available_mutex;      // Mutex for available variable access
  int             available;            // True if this peer is accessible for processing
  struct _GCG_SERVICEITEM *next;  // Next peer in the list
} GCG_SERVICEITEM;


// Internal struct: needed to free the gcg.h from depending on pthread.
typedef struct _GCG_INETSERVICE {
  unsigned int          numpeers;           // Number of peers in the list
  GCG_SERVICEITEM       *firstpeer;         // Linked list of peers
  pthread_mutex_t       list_mutex;         // Mutex for accessing the linked list
  pthread_cond_t        select_cond;        // Condition for select
  pthread_mutex_t       processevents_mutex;// Mutex for processEvents() mutual exclusion
  int                   processevents;      // Flag for running processEvents()
  unsigned int          timeout;            // Timeout for each round of processEvents() in microseconds

  // Listening options
  SOCKET                listener;           // Listening socket
} GCG_INETSERVICE;


gcgINETSERVICE::gcgINETSERVICE() {
  this->handle = (GCG_INETSERVICE*) ALLOC(sizeof(GCG_INETSERVICE));
  if(handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgINETSERVICE(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;

  // Connections information
  pthread_mutex_init(&service->list_mutex, NULL);
  pthread_mutex_lock(&service->list_mutex);
    service->numpeers = 0;
    service->firstpeer = NULL;
    pthread_cond_init(&service->select_cond, NULL);
    service->processevents = true;
    service->timeout = 100000;
    service->listener = 0;
    this->isListening = false;
  pthread_mutex_unlock(&service->list_mutex);

  pthread_mutex_init(&service->processevents_mutex, NULL);
}

// Group destructor
gcgINETSERVICE::~gcgINETSERVICE() {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;

  if(service) {
    stopListening();

    // If we get this lock, there is no processEvents() running
    postExit();
    for(int i = 0; i < 10; i++) {
      // Waits threads during the time specified by the user
      struct timespec timeout;
      gcgComputeTimeout(&timeout, service->timeout);

      if(pthread_mutex_timedlock(&service->processevents_mutex, &timeout) == 0) {
        pthread_mutex_unlock(&service->processevents_mutex); // Unlock processEvents()
        break;
      } else postExit(); // Ask processEvents() to exit
    }

    // Lock group list
    pthread_mutex_lock(&service->list_mutex);

    while(service->firstpeer) {
      GCG_SERVICEITEM *item = service->firstpeer;

      service->firstpeer = item->next;

      // Free item
      pthread_mutex_destroy(&item->available_mutex);
      SAFE_FREE(item);
    }

    pthread_cond_destroy(&service->select_cond);
    pthread_mutex_destroy(&service->processevents_mutex);

    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    pthread_mutex_destroy(&service->list_mutex);
    SAFE_FREE(handle);
  }

  // No error
}


// Attend clients listening this port
bool gcgINETSERVICE::listenPort(int port) {
  // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  // Initialize Winsock
  WSADATA wsaData;
  memset((void*) &wsaData, 0, sizeof(wsaData));
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "listenPort(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group array
  pthread_mutex_lock(&service->list_mutex);

  // Stops previous service
  if(isListening) {
    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    stopListening();

    // Lock group array
    pthread_mutex_lock(&service->list_mutex);
  }

  // Socket
  int yes = 1;      // For setsockopt() SO_REUSEADDR, below

  // Try to listen...
  try {
    // Create the listener socket
    if((service->listener = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SOCKETERROR), "listenPort(): socket creation error for port %d. (%s:%d)", port, basename((char*)__FILE__), __LINE__);

      // Unlock group list
      pthread_mutex_unlock(&service->list_mutex);
      return false;
    }

    // "Address already in use" error message
    if(setsockopt(service->listener, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(int)) == -1) {

// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      closesocket(service->listener);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      close(service->listener); // Close socket descriptor from client side
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SETSOCKETOPTERROR), "listenPort(): address already in use for port %d. (%s:%d)", port, basename((char*)__FILE__), __LINE__);

      // Unlock group list
      pthread_mutex_unlock(&service->list_mutex);
      return false;
    }

    struct  sockaddr_in serviceaddr;

    // Bind the socket to the specified port
    serviceaddr.sin_family = AF_INET;
    serviceaddr.sin_addr.s_addr = INADDR_ANY;
    serviceaddr.sin_port = htons((u_short) port);
    memset(&(serviceaddr.sin_zero), '\0', 8);

    if(bind(service->listener, (struct sockaddr*) &serviceaddr, sizeof (serviceaddr)) == SOCKET_ERROR) {
// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      closesocket(service->listener);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      close(service->listener); // Close socket descriptor from client side
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_BINDERROR), "listenPort(): could not bind to the port %d. (%s:%d)", port, basename((char*)__FILE__), __LINE__);

      // Unlock group list
      pthread_mutex_unlock(&service->list_mutex);
      return false;
    }

    // Listening
    if(listen(service->listener, SOMAXCONN) == SOCKET_ERROR) {  // SOMAXCONN is the maximum number of connections addmited by SO
// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      closesocket(service->listener);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      close(service->listener); // Close socket descriptor from client side
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_LISTENERROR), "listenPort(): could not listen port %d. (%s:%d)", port, basename((char*)__FILE__), __LINE__);

      // Unlock group list
      pthread_mutex_unlock(&service->list_mutex);
      return false;
    }
  } catch(...) {
    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);

    // An exception occurred
    stopListening();
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "listenport(): exception occured while listening. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  this->isListening = true;

  // Unlock group list
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}



// Stops the listening. No further connections are accepted.
bool gcgINETSERVICE::stopListening() {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "stopListening(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  if(isListening) {
  // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
    // Assures all remaining data is sent/received
    shutdown(service->listener, SD_BOTH);
    closesocket(service->listener);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
    // Assures all remaining data is sent/received
    shutdown(service->listener, SHUT_RDWR);
    close(service->listener); // Close socket descriptor from listener side
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
  }

  service->listener = 0;
  isListening = false;

  // Unlock group list
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}


// Insert a new peer. The peer must be connected.
bool gcgINETSERVICE::insertPeer(gcgINETPEER *peer) {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "insertPeer(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group array
  pthread_mutex_lock(&service->list_mutex);

  // Check parameter
  if(!peer) {
    // Unlock group array
    pthread_mutex_unlock(&service->list_mutex);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "insertPeer(): the parameter peer is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }
  if(!peer->isConnected) {
    // Unlock group array
    pthread_mutex_unlock(&service->list_mutex);
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "insertPeer(): the parameter peer is not connected. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check if the peer is already included
  GCG_SERVICEITEM *item;
  for(item = service->firstpeer; item != NULL; item = item->next)
    if(peer == item->peer) break;

  if(item) {
    // Peer found
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "insertPeer(): peer already inserted. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    return false;
  }

  // Allocates a new item
  item = (GCG_SERVICEITEM*) ALLOC(sizeof(GCG_SERVICEITEM));
  if(!item) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "insertPeer(): could not allocate a new GCG_SERVICEITEM. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    return false;
  }

  GCG_INETPEER *p = (GCG_INETPEER*) peer->handle;

  // Here we have room for a new peer
  pthread_mutex_init(&item->available_mutex, NULL);
  pthread_mutex_lock(&item->available_mutex);
    item->peer = peer;
    item->socket = p->socket;                   // This copy assures we won't need to access the peer again
    item->checkevents = GCG_EVENT_RECEIVEREADY; // Initially checks for incoming messages
    item->events = GCG_EVENT_INSERTED;          // Call insertion event
    item->next = service->firstpeer;
    item->available = true;
  pthread_mutex_unlock(&item->available_mutex);

  // New peer inserted
  service->numpeers++;
  service->firstpeer = item;

  // Unlock group array
  pthread_cond_signal(&service->select_cond); // New peer. Wake processEvents() up.
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}

// Returns the number of peers of the group.
int gcgINETSERVICE::getPeerCount() {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getPeerCount(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  unsigned int n = service->numpeers;

  // Unlock group list
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return (int) n;
}


// Flags which events are to be checked for a specific peer. If checkReceive is
// true, the verification of events will check if there are available messages and, if
// so, will call the eventReceive() handler. If checkSend is true, the event verification
// will check if the peer can send messages and, if so, will call the eventSend() handler.
// If both are false, then event processor just watchs for errors or disconnections.
bool gcgINETSERVICE::flagEvents(gcgINETPEER *peer, bool checkReceive, bool checkSend) {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "flagEvents(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  // Check parameter
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "flagEvents(): parameter peer is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    return false;
  }

  // Search peer entry
  GCG_SERVICEITEM *item;
  for(item = service->firstpeer; item != NULL; item = item->next)
    if(peer == item->peer) break;

  if(item) {
    // Peer found
    item->checkevents = 0;
    if(checkReceive) item->checkevents |= GCG_EVENT_RECEIVEREADY;
    if(checkSend) item->checkevents |= GCG_EVENT_SENDREADY;

    // Unlock list
    pthread_cond_signal(&service->select_cond); // Modified event flags. Wake processEvents() up.
    pthread_mutex_unlock(&service->list_mutex);

    // No error
    return true;
  }

  // Unlock list
  pthread_mutex_unlock(&service->list_mutex);

  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "flagEvents(): parameter peer is not included in this gcgINETSERVICE instance. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return false;
}


// Flags the peer to be removed
bool gcgINETSERVICE::postRemove(gcgINETPEER *peer) {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "postRemove(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  // Check parameter
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "postRemove(): parameter peer is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    return false;
  }

  // Search peer entry
  GCG_SERVICEITEM *item;
  for(item = service->firstpeer; item != NULL; item = item->next)
    if(peer == item->peer) break;

  if(item) {
    // Peer found
    item->events |= GCG_EVENT_REMOVED;

    // Unlock list
    pthread_cond_signal(&service->select_cond); // Modified check
    pthread_mutex_unlock(&service->list_mutex);

    // No error
    return true;
  }

  // Unlock list
  pthread_mutex_unlock(&service->list_mutex);

  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "postRemove(): parameter peer is not included in this gcgINETSERVICE instance. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return false;
}

// Flags all current peers to be removed
bool gcgINETSERVICE::postRemoveAll() {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "postRemoveAll(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  GCG_SERVICEITEM *item;
  for(item = service->firstpeer; item != NULL; item = item->next)
    item->events |= GCG_EVENT_REMOVED;

  // Unlock group list
  pthread_cond_signal(&service->select_cond); // Modified check
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}

// Flags the peer to be visited
bool gcgINETSERVICE::postVisit(gcgINETPEER *peer) {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "postVisit(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  // Check parameter
  if(!peer) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "postVisit(): parameter peer is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Unlock group list
    pthread_mutex_unlock(&service->list_mutex);
    return false;
  }

  // Search peer entry
  GCG_SERVICEITEM *item;
  for(item = service->firstpeer; item != NULL; item = item->next)
    if(peer == item->peer) break;

  if(item) {
    // Peer found
    item->events |= GCG_EVENT_VISIT;

    // Unlock list
    pthread_cond_signal(&service->select_cond); // Modified check
    pthread_mutex_unlock(&service->list_mutex);

    // No error
    return true;
  }

  // Unlock list
  pthread_mutex_unlock(&service->list_mutex);

  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_BADPARAMETERS), "postVisit(): parameter peer is not included in this gcgINETSERVICE instance. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return false;
}


// Flags all current peers to be visited
bool gcgINETSERVICE::postVisitAll() {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "postVisitAll(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);

  GCG_SERVICEITEM *item;
  for(item = service->firstpeer; item != NULL; item = item->next) item->events |= GCG_EVENT_VISIT;

  // Unlock group list
  pthread_cond_signal(&service->select_cond); // Modified check
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}

// Flags the processEvents() to exit
bool gcgINETSERVICE::postExit() {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "postExit(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);
    service->processevents = false;

    pthread_cond_signal(&service->select_cond); // Modified check

  // Unlock group list
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}

// Class for thread pool execution
class gcgJOBEVENT : public gcgJOB {
  public:
    int               flagevents;
    gcgINETSERVICE    *group;
    GCG_SERVICEITEM   *item;

    gcgJOBEVENT(int flagevents, gcgINETSERVICE *group, GCG_SERVICEITEM *item) {
      this->flagevents = flagevents;
      this->group = group;
      this->item  = item;
    }

    void run() {
      if(flagevents == GCG_EVENT_REMOVED ||
         flagevents == GCG_EVENT_NEWCONNECTION) group->eventHandler(flagevents, (gcgINETPEER*) item);
      else {
        group->eventHandler(flagevents, item->peer);

        // Lock item
        pthread_mutex_lock(&item->available_mutex);
          item->available = true;
          pthread_cond_broadcast(&((GCG_INETSERVICE*) group->handle)->select_cond);
        // Unlock item
        pthread_mutex_unlock(&item->available_mutex);
      }
    }
};



bool gcgINETSERVICE::processEvents(unsigned int numThreads) {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "processEvents(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Only a processEvents() running at a time
  pthread_mutex_lock(&service->processevents_mutex);

  bool noerror = true;

  gcgTHREADPOOL *threadpool = new gcgTHREADPOOL();
  if(!threadpool) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "processEvents(): could not create a new gcgTHREADPOOL. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Allow another processEvents to run
    pthread_mutex_unlock(&service->processevents_mutex);
    return false;
  }

  if(!threadpool->setNumberOfThreads(numThreads)) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CREATIONFAILED), "processEvents(): failed to create a threadpool with %d threads. (%s:%d)", numThreads, basename((char*)__FILE__), __LINE__);

    // Allow another processEvents to run
    pthread_mutex_unlock(&service->processevents_mutex);
    SAFE_DELETE(threadpool);
    return false;
  }

  fd_set  read_fds;
  fd_set  write_fds;
  fd_set  except_fds;
  struct timeval timeout;

  do {
    unsigned int timeoutUsec = service->timeout;
    unsigned int npeers = 0;

    // Lock group list
    pthread_mutex_lock(&service->list_mutex);

    // Processing removals
    GCG_SERVICEITEM *item, *ant = NULL;
    for(item = service->firstpeer; item != NULL;) {
      gcgINETPEER *peer = item->peer;
      int events = item->events & (GCG_EVENT_REMOVED);

      // Lock item
      pthread_mutex_trylock(&item->available_mutex);
      if(item->available && (events != 0 || !item->peer->isConnected)) {
        // Remove item
        GCG_SERVICEITEM *p = item;
        if(ant) item = ant->next = item->next;
        else item = service->firstpeer = item->next;
        pthread_mutex_unlock(&p->available_mutex); // Unlock item
        pthread_mutex_destroy(&p->available_mutex);
        SAFE_DELETE(p);

        service->numpeers--;

        // Unlock list
        pthread_mutex_unlock(&service->list_mutex);

        // Call handler with remove event flaged
        gcgJOBEVENT *job = new gcgJOBEVENT(GCG_EVENT_REMOVED, this, (GCG_SERVICEITEM*) peer);

        if(!threadpool->assignJob(job)) { // May block
          SAFE_DELETE(job);
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_JOBNOTEXECUTED), "processEvents(): a peer job was not assigned to threadpool. (%s:%d)", basename((char*)__FILE__), __LINE__);

          // Allow another processEvents to run
          pthread_mutex_unlock(&service->processevents_mutex);
          SAFE_DELETE(threadpool);
          return false;
        }
        // Lock list
        pthread_mutex_lock(&service->list_mutex);
      } else {
        pthread_mutex_unlock(&item->available_mutex); // Unlock item
        ant = item;
        item = item->next;
      }
    }
    // Unlock list
    pthread_mutex_unlock(&service->list_mutex);

    // Lock group list
    pthread_mutex_lock(&service->list_mutex);

    // Processing insertions and visitations
    for(item = service->firstpeer; item != NULL; item = item->next) {
      int events = item->events & (GCG_EVENT_INSERTED | GCG_EVENT_VISIT);

      // Lock item
      pthread_mutex_lock(&item->available_mutex);
      if(item->available && events != 0) {
        gcgJOBEVENT *job = new gcgJOBEVENT(item->events & ((GCG_EVENT_INSERTED | GCG_EVENT_VISIT)), this, item);

        // Unlock list
        pthread_mutex_unlock(&service->list_mutex);

        if(!threadpool->assignJob(job)) { // May block
          SAFE_DELETE(job);
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_JOBNOTEXECUTED), "processEvents(): a peer job was not assigned to threadpool. (%s:%d)", basename((char*)__FILE__), __LINE__);

          // Lock item
          pthread_mutex_unlock(&item->available_mutex);

          // Allow another processEvents() to run
          pthread_mutex_unlock(&service->processevents_mutex);
          SAFE_DELETE(threadpool);
          return false;
        } else {
          // Lock list
          pthread_mutex_lock(&service->list_mutex);
          item->available = false;
        }
        item->events = item->events & ~((int) GCG_EVENT_INSERTED | GCG_EVENT_VISIT);
      }

      // Unlock item
      pthread_mutex_unlock(&item->available_mutex);
    }
    // Unlock list
    pthread_mutex_unlock(&service->list_mutex);

    // Lock group list
    pthread_mutex_lock(&service->list_mutex);

    // Cancelation point
    if(!service->processevents) {
      threadpool->wait();

      // Checking if there is an event to process BEFORE exiting
      bool processagain = false;
      for(item = service->firstpeer; item != NULL && !processagain; item = item->next)
        processagain = ((item->events & (GCG_EVENT_INSERTED | GCG_EVENT_VISIT | GCG_EVENT_REMOVED)) != 0);

      // No error
      noerror = true;

      // Unlock list
      pthread_mutex_unlock(&service->list_mutex);

      if(processagain) continue;
      else break;
    }

    // Unlock list
    pthread_mutex_unlock(&service->list_mutex);

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    // Lock group list
    pthread_mutex_lock(&service->list_mutex);

    // Adds the listener for selection
    if(this->isListening) {
      FD_SET(service->listener, &read_fds);
      FD_SET(service->listener, &except_fds);
    }

    for(item = service->firstpeer; item != NULL; item = item->next) {
      // Try to lock
      pthread_mutex_lock(&item->available_mutex);
      if((item->available || (item->events & (GCG_EVENT_RECEIVEREADY | GCG_EVENT_SENDREADY))) && item->peer->isConnected) {
        if((item->checkevents & GCG_EVENT_RECEIVEREADY) != 0) FD_SET(item->socket, &read_fds);
        if((item->checkevents & GCG_EVENT_SENDREADY) != 0) FD_SET(item->socket, &write_fds);
        FD_SET(item->socket, &except_fds);
        npeers++;
      }
      // Unlock item
      pthread_mutex_unlock(&item->available_mutex);
    }

    // Check the number of peers
    if(npeers == 0 && !this->isListening) {
      // Waits a peer during the time specified by the user
      struct timespec timeout;
      gcgComputeTimeout(&timeout, timeoutUsec);
      pthread_cond_timedwait(&service->select_cond, &service->list_mutex, &timeout);

      // Adjust timeout
      // Platform specific code
  #if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      struct timeval now;
      gettimeofday(&now, NULL);
      if((long) now.tv_sec > (long) timeout.tv_sec ||
        ((long) now.tv_sec == (long) timeout.tv_sec && (long) now.tv_usec * 1000L > timeout.tv_nsec))
        timeoutUsec = 0;
      else timeoutUsec = ((long) timeout.tv_sec - (long) now.tv_sec) * 1000000L + // Seconds to microseconds
                         (((long) timeout.tv_nsec / 1000L) - (long) now.tv_usec); // Nanoseconds to microseconds
  #elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      if((long) now.tv_sec > (long) timeout.tv_sec ||
        ((long) now.tv_sec == (long) timeout.tv_sec && (long) now.tv_nsec > timeout.tv_nsec))
        timeoutUsec = 0;
      else timeoutUsec = ((long) timeout.tv_sec  - (long) now.tv_sec) * 1000000L + // Seconds to microseconds
                         ((long) timeout.tv_nsec - (long) now.tv_nsec) / 1000L;    // Nanoseconds to microseconds
  #else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
  #endif

      //if(res == 0);// A modification was done.
      //else; // Timeout ocurred

      // Unlock list
      pthread_mutex_unlock(&service->list_mutex);

      continue; // Check the number of peers again
    }

    // Unlock list
    pthread_mutex_unlock(&service->list_mutex);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutUsec;

    // Waits for events
    try {
      // Waits for reading events and for exception events
      int retsel;
      if((retsel = select(FD_SETSIZE, &read_fds, &write_fds, &except_fds, &timeout)) == SOCKET_ERROR) {
        // Lock group list
        pthread_mutex_lock(&service->list_mutex);
          // Some fatal error occured...
          // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
          switch(WSAGetLastError()) {
            case WSAENOBUFS:      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "processEvents(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAECONNRESET:   gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAECONNABORTED: gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAEFAULT:       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEBADADDRESS), basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAEINTR:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INTERRUPTED), "processEvents(): interrupted during select(). (%s:%d)", basename((char*)__FILE__), __LINE__);
                                  break;
            case WSAENOTSOCK:     gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INVALIDSOCKET), basename((char*)__FILE__), __LINE__);
                                  break;
            default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SELECTERROR), basename((char*)__FILE__), __LINE__);
                                  break;
          };
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
          switch(errno) {
            case ENOBUFS:         gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "processEvents(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                  break;
            case EPIPE:
            case ECONNRESET:      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), basename((char*)__FILE__), __LINE__);
                                  break;
            case ECONNABORTED:    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), basename((char*)__FILE__), __LINE__);
                                  break;
            case EFAULT:          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_WRITEBADADDRESS), basename((char*)__FILE__), __LINE__);
                                  break;
            case EINTR:           gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INTERRUPTED), "processEvents(): interrupted during select(). (%s:%d)", basename((char*)__FILE__), __LINE__);
                                  break;
            case ENOTSOCK:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INVALIDSOCKET), basename((char*)__FILE__), __LINE__);
                                  break;
            default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SELECTERROR), basename((char*)__FILE__), __LINE__);
          };
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

        // Unlock group list
        pthread_mutex_unlock(&service->list_mutex);
        noerror = false;
      }

      // Check timeout
      if(retsel == 0) {
        gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_NETWORK, GCG_TIMEOUT), "processEvents(): time is out during select(). (%s:%d)", basename((char*)__FILE__), __LINE__);
        noerror = false;
        continue; // Try again
      }

      // Check the port listened
      unsigned int nevents = 0;
      if(this->isListening) {
        // Check if a error ocurred with the listener
        if(FD_ISSET(service->listener, &except_fds)) {
          // A problem ocurred with the listener
          stopListening();
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_SELECTERROR), "processEvents(): an error has occurred with the listener. (%s:%d)", basename((char*)__FILE__), __LINE__);
          nevents++;
          noerror = false;
        } else
            // Check if a new connection was made
            if(FD_ISSET(service->listener, &read_fds))
              try { // Try to accept the new connection
                struct  sockaddr_in clientaddr;
                int     addrlen;
                SOCKET  lastsocketconnected;

                nevents++;

                // Handle new connections
                addrlen = sizeof(clientaddr);

                // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
                if((lastsocketconnected = accept(service->listener, (struct sockaddr*) &clientaddr, &addrlen)) == INVALID_SOCKET) {
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
                if((lastsocketconnected = accept(service->listener, (struct sockaddr*) &clientaddr, (socklen_t*) &addrlen)) == INVALID_SOCKET) {
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
                  stopListening();

                  // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
                  switch(WSAGetLastError()) {
                    case WSAENOBUFS:      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "processEvents(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                          break;
                    case WSAECONNRESET:   gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), basename((char*)__FILE__), __LINE__);
                                          break;
                    case WSAECONNABORTED: gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), basename((char*)__FILE__), __LINE__);
                                          break;
                    case WSAEFAULT:       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_ACCEPTBADADDRESS), basename((char*)__FILE__), __LINE__);
                                          break;
                    case WSAEINTR:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INTERRUPTED), "processEvents(): interrupted during accept(). (%s:%d)", basename((char*)__FILE__), __LINE__);
                                          break;
                    case WSAENOTSOCK:     gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INVALIDSOCKET), "processEvents(): maybe the socket was suddenly closed. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                          break;
                    default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_ACCEPTERROR), basename((char*)__FILE__), __LINE__);
                                          break;
                  };
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
                  switch(errno) {
                    case ENOBUFS:         gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_NOBUFFER), "processEvents(): buffer unavailable. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                          break;
                    case EPIPE:
                    case ECONNRESET:      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_NETWORK, GCG_CONNECTIONCLOSED), basename((char*)__FILE__), __LINE__);
                                          break;
                    case ECONNABORTED:    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_CONNECTIONABORTED), basename((char*)__FILE__), __LINE__);
                                          break;
                    case EFAULT:          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_ACCEPTBADADDRESS), basename((char*)__FILE__), __LINE__);
                                          break;
                    case EINTR:           gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INTERRUPTED), "processEvents(): interrupted during accept(). (%s:%d)", basename((char*)__FILE__), __LINE__);
                                          break;
                    case ENOTSOCK:        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_INVALIDSOCKET), "processEvents(): maybe the socket was suddenly closed. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                          break;
                    default:              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_ACCEPTERROR), basename((char*)__FILE__), __LINE__);
                                          break;
                  };
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
                  noerror = false;
                } else {
                    // New connection detected
                    // Lock group list
                    pthread_mutex_lock(&service->list_mutex);

                    // Alloc a new peer
                    gcgINETPEER *peer = new gcgINETPEER();

                    if(peer) {
                      // Set the peer connection
                      GCG_INETPEER *peerhandle = (GCG_INETPEER*) peer->handle;
                      peerhandle->socket = lastsocketconnected;
                      peer->isConnected = true;

                      // Transfer local addres information
                      struct sockaddr_in addr;
                      int l = sizeof(addr);

                      // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
                      if(getsockname(peerhandle->socket, (struct sockaddr*) &addr, (socklen_t*) &l) == 0) {
                        peer->local_ip[0] = addr.sin_addr.S_un.S_un_b.s_b1;
                        peer->local_ip[1] = addr.sin_addr.S_un.S_un_b.s_b2;
                        peer->local_ip[2] = addr.sin_addr.S_un.S_un_b.s_b3;
                        peer->local_ip[3] = addr.sin_addr.S_un.S_un_b.s_b4;
                        peer->local_port  = ntohs(addr.sin_port);
                      }

                      if(getpeername(peerhandle->socket, (struct sockaddr*) &addr, (socklen_t*) &l) == 0) {
                        peer->remote_ip[0] = addr.sin_addr.S_un.S_un_b.s_b1;
                        peer->remote_ip[1] = addr.sin_addr.S_un.S_un_b.s_b2;
                        peer->remote_ip[2] = addr.sin_addr.S_un.S_un_b.s_b3;
                        peer->remote_ip[3] = addr.sin_addr.S_un.S_un_b.s_b4;
                        peer->remote_port  = ntohs(addr.sin_port);
                      }
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
                      if(getsockname(peerhandle->socket, (struct sockaddr*) &addr, (socklen_t*) &l) == 0) {
                        peer->local_ip[0] = ((unsigned char*) &addr.sin_addr.s_addr)[0];
                        peer->local_ip[1] = ((unsigned char*) &addr.sin_addr.s_addr)[1];
                        peer->local_ip[2] = ((unsigned char*) &addr.sin_addr.s_addr)[2];
                        peer->local_ip[3] = ((unsigned char*) &addr.sin_addr.s_addr)[3];
                        peer->local_port  = ntohs(addr.sin_port);
                      }

                      if(getpeername(peerhandle->socket, (struct sockaddr*) &addr, (socklen_t*) &l) == 0) {
                        peer->remote_ip[0] = ((unsigned char*) &addr.sin_addr.s_addr)[0];
                        peer->remote_ip[1] = ((unsigned char*) &addr.sin_addr.s_addr)[1];
                        peer->remote_ip[2] = ((unsigned char*) &addr.sin_addr.s_addr)[2];
                        peer->remote_ip[3] = ((unsigned char*) &addr.sin_addr.s_addr)[3];
                        peer->remote_port  = ntohs(addr.sin_port);
                      }
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

                      // Call handler with new conneciton event flaged
                      gcgJOBEVENT *job = new gcgJOBEVENT(GCG_EVENT_NEWCONNECTION, this, (GCG_SERVICEITEM*) peer);

                      // Unlock list
                      pthread_mutex_unlock(&service->list_mutex);

                      if(!threadpool->assignJob(job)) {
                        SAFE_DELETE(job);
                        SAFE_DELETE(peer);
                        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_JOBNOTEXECUTED), "processEvents(): a peer job was not assigned to threadpool. (%s:%d)", basename((char*)__FILE__), __LINE__);

                        // Allow another processEvents to run
                        pthread_mutex_unlock(&service->processevents_mutex);
                        SAFE_DELETE(threadpool);
                        return false;
                      }

                      // Lock list
                      pthread_mutex_lock(&service->list_mutex);
                    } else {
                      // Socket not connected. Let's close it.

// Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
                        closesocket(lastsocketconnected);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
                        close(lastsocketconnected); // Close socket descriptor from service side
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
                    }

                    // Unlock group list
                    pthread_mutex_unlock(&service->list_mutex);
                  }
              } catch(...) {
                // Error accepting a new connection
                stopListening();
                gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "processEvents(): exception occured while accepting a new connection. (%s:%d)", basename((char*)__FILE__), __LINE__);
                noerror = false;
              }
      }


      threadpool->wait();

      // Lock group list
      pthread_mutex_lock(&service->list_mutex);

      // Scan all peers of the group looking for events
      for(item = service->firstpeer; item != NULL; item = item->next) {
        int events = 0; // Send receive events

        // Check exception array
        if(FD_ISSET(item->socket, &except_fds)) {
          FD_CLR(item->socket, &except_fds); // Already attended
          FD_CLR(item->socket, &read_fds);   // Do not attend
          FD_CLR(item->socket, &write_fds);  // Do not attend

          // Socket had a connection problem
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "processEvents(): exception occured in select() for peer %d:%d:%d:%d:%d. (%s:%d)", item->peer->remote_ip[0], item->peer->remote_ip[1], item->peer->remote_ip[2], item->peer->remote_ip[3], item->peer->remote_port, basename((char*)__FILE__), __LINE__);
          item->checkevents = 0;
          item->events = GCG_EVENT_REMOVED; // Mark to remove
          nevents++;
          noerror = false;
        }

        // Check read array
        if(FD_ISSET(item->socket, &read_fds)) {
          // Socket got a message: call event handler
          FD_CLR(item->socket, &read_fds); // Already attended
          events |= GCG_EVENT_RECEIVEREADY;
          nevents++;
        }

        // Check write array
        if(FD_ISSET(item->socket, &write_fds)) {
          // Socket is ready to send: call event handler
          FD_CLR(item->socket, &write_fds); // Already attended
          events |= GCG_EVENT_SENDREADY;
          nevents++;
        }

        if(events) {
          gcgJOBEVENT *job = new gcgJOBEVENT(events, this, item);

          // Lock item
          pthread_mutex_lock(&item->available_mutex);
          item->events |= events;

          if(item->available) {
            if(!threadpool->assignJob(job)) { // May block
              SAFE_DELETE(job);
              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_JOBNOTEXECUTED), "processEvents(): a peer job was not assigned to threadpool. (%s:%d)", basename((char*)__FILE__), __LINE__);

              // Unlock item
              pthread_mutex_unlock(&item->available_mutex);

              // Unlock list
              pthread_mutex_unlock(&service->list_mutex);

              // Allow another processEvents to run
              pthread_mutex_unlock(&service->processevents_mutex);
              SAFE_DELETE(threadpool);
              return false;
            } else {
              item->available = false;
            }
          }

          // Unlock item
          pthread_mutex_unlock(&item->available_mutex);
        } else item->events = item->events & ~(GCG_EVENT_SENDREADY | GCG_EVENT_RECEIVEREADY);
      }

      // Unlock list
      pthread_mutex_unlock(&service->list_mutex);
    } catch(...) {
        // A fatal exception occured
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_NETWORK, GCG_EXCEPTION), "processEvents(): exception occured in select(). (%s:%d)", basename((char*)__FILE__), __LINE__);
        noerror = false;
    }
  } while(true);

  // Allow another processEvents to run
  pthread_mutex_unlock(&service->processevents_mutex);

  SAFE_DELETE(threadpool);
  return noerror;
}


bool gcgINETSERVICE::setTimeout(int timeoutUsec) {
  GCG_INETSERVICE *service = (GCG_INETSERVICE*) handle;
  if(!service) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setTimeout(): attempt to use an invalid GCG_INETSERVICE handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock group list
  pthread_mutex_lock(&service->list_mutex);
    service->timeout = timeoutUsec;
  // Unlock group list
  pthread_mutex_unlock(&service->list_mutex);

  // No error
  return true;
}
