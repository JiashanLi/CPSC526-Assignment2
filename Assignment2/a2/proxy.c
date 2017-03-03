/*
 * A simple proxy server that forward and log data between server and client.
 * Group members : Ivan Leung 10058364 T01
 *                 Jiashan Li 10171607 T03
 * Contact information : iwcleung@ucalgary.ca
 *                       jiashan.li@ucalgary.ca
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include<ctype.h>
#include <time.h>
#define BUF_SIZE 4096

//wget -p localhost:2221

extern int sys_nerr, errno;
char * raw(const char * ,int );
char* replaceAll(char*,char * ,char* );
char* replaceLast(char* ,char * ,char* );
void hexDump (char *, void *, int );
void strip(const char* );
void autoN(const char* , int, int );

    char client_hostname[64];
    


    void set_nonblock(int fd)
    {
        int fl;
        int x;
        x = fcntl(fd, F_GETFL, &fl);
        if (x < 0) {

        }
        fl |= O_NONBLOCK;
        x = fcntl(fd, F_SETFL, &fl);
        if (x < 0) {

        }
    }


    int proxy_server(char *addr, int port)
    {
        int addrlen, s, on = 1, x;
        static struct sockaddr_in client_addr;

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0){
        perror("\nFailed to create socket"), exit(1);
        }else {printf("Proxy socket created\n");}

        addrlen = sizeof(client_addr);
        memset(&client_addr, '\0', addrlen);
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = inet_addr(addr);
        client_addr.sin_port = htons(port);
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);
        x = bind(s, (struct sockaddr *) &client_addr, addrlen);
        if (x < 0)
        perror("Failed to bind a socket"), exit(1);

        x = listen(s, 5);
        if (x < 0)
        perror("listen"), exit(1);
    
        printf("waiting for connection...\n");

        return s;
    }

    int remote_host(char *host, int port)
    {
        struct sockaddr_in rem_addr;
        int len, s, x;
        struct hostent *H;
        int on = 1;

        H = gethostbyname(host);
        if (!H){
            printf("cannot get host ip address\n");
            return (-2);
        }
        
        //code to connect to main server via this proxy server  
        len = sizeof(rem_addr);
        // create a socket  
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0){
            printf("server socket not created\n");
            return s;
        }else {printf("server socket created\n"); }
        

        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);

        len = sizeof(rem_addr);
        memset(&rem_addr, '\0', len);
        rem_addr.sin_family = AF_INET;
        memcpy(&rem_addr.sin_addr, H->h_addr, H->h_length);
        rem_addr.sin_port = htons(port);
        x = connect(s, (struct sockaddr *) &rem_addr, len);
        if (x < 0) {
            printf("server connection not established\n"); 
        close(s);
        return x;
        }else {printf("server socket connected\n"); }
        set_nonblock(s);
        return s;
    }

    int sock_addr_info(struct sockaddr_in addr, int len, char *fqdn)
    {
        struct hostent *hostinfo;

        hostinfo = gethostbyaddr((char *) &addr.sin_addr.s_addr, len, AF_INET);
        if (!hostinfo) {
        sprintf(fqdn, "%s", inet_ntoa(addr.sin_addr));
        return 0;
        }
        if (hostinfo && fqdn)
        sprintf(fqdn, "%s [%s]", hostinfo->h_name, inet_ntoa(addr.sin_addr));
        return 0;
    }


    int proxy_connec(int s,int* user)
    {
       int newsock;
       int * usern = user;
        time_t rawtime;
    static struct sockaddr_in peer;
    socklen_t len;
    len = sizeof(struct sockaddr);
    newsock = accept(s, (struct sockaddr *) &peer, &len);
    
        if (newsock < 0) {
        if (errno != EINTR)
            perror("accept");
        }else {
            ++*usern;
            printf("client no. %d connected\n",*user); }
        sock_addr_info(peer, len, client_hostname);
        time ( &rawtime );
        printf("New connection: from %s, %s",inet_ntoa(peer.sin_addr), ctime (&rawtime));
        set_nonblock(newsock);
        return (newsock);
    }

    int sendData(int fd, char *buf, int *len)
    {
        int x = write(fd, buf, *len);
        if (x < 0)
            return x;
        if (x == 0)
            return x;
        if (x != *len)
            memmove(buf, buf+x, (*len)-x);
        *len -= x;
        return x;
    }

    void handle_client(int cfd, int sfd, int opt, int chunk)
    {
        int maxfd;
        char *sbuf;
        char *cbuf;
        int x, n;
        int cbytes = 0;
        int sbytes = 0;
        fd_set R;

        sbuf = (char *)malloc(BUF_SIZE);
        cbuf = (char *)malloc(BUF_SIZE);

        maxfd = cfd > sfd ? cfd : sfd;
        maxfd++;
        
        int right ,left;

       while (1)
       {
        struct timeval to;
        if (cbytes)
            {
                // --->
                right = sendData(sfd, cbuf, &cbytes);
                if (opt == 1){
                    printf("---> %s\n", raw(cbuf,1));
                }else if(opt == 2){
                    printf("---> ");
			strip(cbuf);
                }else if (opt == 3){
                    hexDump ("--->", cbuf, sizeof(char)*strlen(cbuf)+1);
                }else if (opt ==4){
                    autoN(cbuf,chunk,1);
                    
                }
            if (right < 0 && errno != EWOULDBLOCK) {
                    exit(1);
            }
        }
        if (sbytes) {
            // <---
            if (opt == 1){
                printf("<--- %s\n", raw(sbuf,0));
            }else if(opt == 2){
                printf("<--- ");
		strip(sbuf);
            }else if (opt == 3){
                hexDump ("<---", sbuf, sizeof(char)*strlen(sbuf) +1);
            }else if (opt ==4){
                autoN(sbuf,chunk,0);
            }
            left = sendData(cfd, sbuf, &sbytes);
            if ( left < 0 && errno != EWOULDBLOCK) {
                    exit(1);
            }
            
        }

        FD_ZERO(&R);
        if (cbytes < BUF_SIZE)
            FD_SET(cfd, &R);
        if (sbytes < BUF_SIZE)
            FD_SET(sfd, &R);

        to.tv_sec = 0;
        to.tv_usec = 1000;
        x = select(maxfd+1, &R, 0, 0, &to);
        if (x > 0) {
            if (FD_ISSET(cfd, &R)) {
                //read data from client
            memset(cbuf, '\0', BUF_SIZE);
            n = read(cfd, cbuf+cbytes, BUF_SIZE-cbytes);
            
            if (n > 0) {
                cbytes += n;
            } else {
                printf("Client connection closed.\n");
                close(cfd);
                close(sfd);
                _exit(0);
            }
            }
            if (FD_ISSET(sfd, &R)) {
                //read data from server
            memset(sbuf, '\0', BUF_SIZE);    
            n = read(sfd, sbuf+sbytes, BUF_SIZE-sbytes);
            if (n > 0) {
                sbytes += n;
            } else {
                printf("Server connection closed.\n");
                close(sfd);
                close(cfd);
                _exit(0);
            }
            }
        } else if (x < 0 && errno != EINTR) {
            close(sfd);
            close(cfd);
            _exit(0);
        }
        }
        free(sbuf);
        free(cbuf);
    }

//http://csourcecodes.blogspot.ca/2016/04/c-program-to-replace-all-occurrences-of.html 
char* replaceAll(char* mainstr,char * substr,char* newstr)
{
    int lenmain,lensub,i,j,lennew,startindex=-1,limit,c;
    lenmain=strlen(mainstr);
    lensub=strlen(substr);
    lennew=strlen(newstr);
    char *result=(char*)malloc(sizeof(char)*(lenmain+200));
    for(c=0,i=0;i<lenmain;i++)
    {
        if(lenmain-i>=lensub&&*(mainstr+i)==*(substr))
        {
            startindex=i;
            for(j=1;j<lensub;j++)
            if(*(mainstr+i+j)!=*(substr+j))
            {
                startindex=-1;
                break;
            }
            if(startindex!=-1)
            {
                for(j=0;j<lennew;j++,c++)
                {
                    *(result+c)=*(newstr+j);
                }
                i=i+lensub-1;
            }
            else
            {
                *(result+c)=*(mainstr+i);
                c++;
            }
        }
        else
        {
            *(result+c)=*(mainstr+i);
            c++;
        }
    }
    *(result+c)='\0';
    return result;
}
//http://csourcecodes.blogspot.ca/2016/04/c-program-to-replace-last-occurrence-of.html
char* replaceLast(char* mainstr,char * substr,char* newstr)
{
    int lenmain,lensub,i,j,lennew,startindex=-1,limit,c;
    lenmain=strlen(mainstr);
    lensub=strlen(substr);
    lennew=strlen(newstr);
    char *result=(char*)malloc(sizeof(char)*(lenmain+100));
    for(i=lenmain-1;i>=0;i--)
    {
        if(lenmain-i>=lensub&&*(mainstr+i)==*(substr))
        {
            startindex=i;
            for(j=1;j<lensub;j++)
            if(*(mainstr+i+j)!=*(substr+j))
            {
                startindex=-1;
                break;
            }
            if(startindex!=-1)
            break;
        }
    }
    limit=(startindex==-1)?lenmain:startindex;
    for(i=0;i<limit;i++)
    *(result+i)=*(mainstr+i);
    if(startindex!=-1)
    {
        for(j=0;j<lennew;j++)
        *(result+i+j)=*(newstr+j);
        c=i+lensub;
        i=i+j;
        for(;c<lenmain;c++,i++)
        *(result+i)=*(mainstr+c);
    }
    *(result+i)='\0';
    return result;
}

char * raw(const char * data, int flag ){
    int size = strlen(data)+1;
    char *d = malloc(sizeof(char)*(size));
    memset(d, '\0', sizeof(char)*(size) );
    strncpy(d, data,size );
    if (flag == 1){
        return replaceLast(replaceAll(d,"\n", "\n---> "),"\n---> ","") ;}
    if (flag == 0){
        return replaceLast(replaceAll(d,"\n", "\n<--- "),"\n<--- ","");}
    
}

void  strip(const char* data){
    int size = strlen(data)+1;
//    char *d = malloc(sizeof(char)*(size));
//    memset(d, '\0', sizeof(char)*(size) );
//    strncpy(d, data,size );
    int i; 
	
    for (i = 0; i < size; i++){
        if (!isprint(data[i])){
            putchar('.');
        }else {putchar(data[i]);}
    }
	printf("\n");

}
// Edited from :
// http://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
    printf ("%s\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
            printf ("  %s\n", buff); // no 00000000

            // Output the offset.
            printf ("  %08x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
        buff[i % 16] = '.';
        else
        buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
   // hex only printf ("  \n" );
}

// assume \n \r '\0' one bytes in every systems
void autoN(const char* data, int chuk, int flag){
    int size = strlen(data)+1;
    char *d = malloc(sizeof(char)*(size));
    memset(d, '\0', sizeof(char)*(size) );
    strncpy(d, data,size );
    int charsLeft = size % chuk;
    int divNum = size / chuk +(charsLeft != 0) ;
    char piece[chuk];
    memset(piece, '\0', chuk);
    int i,j;
    for ( i =0 ; i < divNum ; i++){


        
        memset(piece, '\0', chuk);
        strncpy(piece,d +(chuk*i),chuk);
        
        if (flag == 1){printf("---> ");}
        else if (flag == 0){printf("<--- ");}
        
        int j;
        for (j = 0; j < chuk; j++){
            
            if (piece[j] == '\t' ){
                printf("\\t"); 
            }
            else if (piece[j] == '\n'){
                printf("\\n");
            }
            else if (piece[j] == '\r'){
                printf("\\r");
            }
            else if (piece[j] == 0x5c){   // backslash
                printf("\\\\");
            }else if (piece[j] >= 32 && piece[j] <= 127 ){
                putchar(piece[j]);
            }else {
                printf ("\\%02x", (unsigned char ) piece[j]);
            }
            
            if (i == (divNum-1) && divNum != 0 && j == charsLeft-1){
                break;
            }
            
            
  /*          
            if (!isprint(piece[j])){
                if (piece[j] == '\t' ){
                   printf("\\t"); 
                }
                else if (piece[j] == '\n'){
                    printf("\\n");
                }
                else if (piece[j] == '\r'){
                    printf("\\r");
                }
                else if(i == 0 || i > 3){
                    printf ("\\%02x",(unsigned char ) piece[j]);
                    if (i == (divNum-1) && divNum != 0 && j == charsLeft-1){
                        break;
                    }
                }
                else {
                    if (i == (divNum-1) && divNum != 0 && j == charsLeft-1){
                        printf ("\\%02x", (unsigned char ) piece[j]);
                        break;
                    }else{
                    printf("%c",piece[j]);}
                }
            }else if (piece[j] == 0x5c){
                printf("\\\\");
            }
            else {
                printf("%c",piece[j]);

                if (i == (divNum-1) && divNum != 0 && j == charsLeft-1){
                    break;
                }
            }
        */
        }
        
        printf("\n");
    }

}


    int main(int argc, char *argv[])
    {
        char *localaddr = (char *)"127.0.0.1";
        int localport ;
        char *remoteaddr;
        int remoteport ;
        int client, server;
        int master_sock;
        int logN , bytesN =0;
        
        if(argc == 4){
            localport = atoi(argv[1]);
            remoteaddr = (char *)(argv[2]);
            remoteport = atoi(argv[3]);
        printf("host : %s and port %s " , argv[2],argv[3]);   
        printf("proxy port is %s ",argv[1]);        
        printf("\n");  

            
        }else if(argc == 5) {
            localport = atoi(argv[2]);
            remoteaddr = (char *)(argv[3]);
            remoteport = atoi(argv[4]);
            if (strncmp(argv[1], "-raw", 4) == 0)
            logN = 1;
            else if (strncmp(argv[1], "-strip", 6) == 0)
            logN = 2;
            else if (strncmp(argv[1], "-hex", 4) == 0)
            logN = 3;
            else if (strncmp(argv[1], "-auto", 5) == 0){
                logN = 4;
                bytesN = atoi(strrchr(argv[1], 'o')+1);
                if (bytesN == 0){
                   printf("Wrong bytes\n");
                   exit(1);
                }
            }
            
        printf("Port logger running : ");
        printf("srcPort %s ",argv[2]); 
        printf("host = %s and dstPort %s " , argv[3],argv[4]);
        printf("\n");  

        }

        else 
        {
            fprintf(stderr, "usage: %s [logOptions] port host port\n", argv[0]);
            exit(1);
        }

        assert(localaddr);
        assert(localport > 0);
        assert(remoteaddr);
        assert(remoteport > 0);
        

        master_sock = proxy_server(localaddr, localport);
        
        int usernum = 0;


        for (;;)
        {
            
            if ((client = proxy_connec(master_sock, &usernum)) < 0)
                continue;

            if ((server = remote_host(remoteaddr, remoteport)) < 0)
                continue;
            if (!fork()) {
                handle_client(client, server, logN, bytesN);
            }


            close(client);
            close(server);        
        }


        return 0;
    }
// HTML character codes https://www.obkb.com/dcljr/charstxt.html
// letter count http://www.lettercount.com/
