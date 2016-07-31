//"http://record.vod.huanjuyun.com/xcrs/15013x03_1330846705_1554444831_1467210406_1467210388.m3u8";//(char *)arg;
  //"http://record.vod.huanjuyun.com/xcrs/15013x03_1330846705_1554444831_1467210406_1467210388.m3u8";
  // av_url_split(proto, sizeof(proto),
  //               auth, sizeof(auth),
  //               hostname, sizeof(hostname),
  //               &port,
  //               path, sizeof(path),
  //               url);

  // if (port < 0)
  // {
  //   port = 80;
  // }
  // int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  // assert( sock_fd >= 0 );

  // struct hostent* hostinfo = gethostbyname(hostname);
  // assert(hostinfo);

  // printHostIp(hostinfo);

  // // fprintf(stdout, "parse url:proto=%s \nauth=%s \nhostname=%s \nport=%d \npath=%s\n",
  // //  proto, auth, hostname, port, path);

  // //char *ip;
  // struct sockaddr_in server;
  // bzero( &server, sizeof( server ) );
  // server.sin_family = AF_INET;
  // server.sin_addr = *((struct in_addr*)(hostinfo->h_addr));
  // //inet_pton( AF_INET, ip, &server.sin_addr );
  // server.sin_port = htons( port );

  // int connfd = connect(sock_fd, (struct sockaddr*)&server,  sizeof(server));
  // assert(connfd != -1);
  // int ret = bind( sock_fd, ( struct sockaddr* )&server, sizeof( server ) );
  // assert(ret != 0);
  //int request_send(int sockfd, char *method, char *host, char *arg)
  // while( 1 )
  // {
  //     data_read = recv( sock_fd, buffer + read_index, BUFFER_SIZE - read_index, 0 );
  //     fprintf(stdout, "recv %d\n", data_read);
  //     if ( data_read == -1 )
  //     {
  //         printf( "reading failed %s\n" , strerror(errno));
  //         break;
  //     }
  //     else if ( data_read == 0 )
  //     {
  //         printf( "remote client has closed the connection\n" );
  //         break;
  //     }


  //     // read_index += data_read;
  //     // HTTP_CODE result = parse_content( buffer, checked_index, checkstate, read_index, start_line );
  //     // if( result == NO_REQUEST )
  //     // {
  //     //     continue;
  //     // }
  //     // else if( result == GET_REQUEST )
  //     // {
  //     //     send( connfd, szret[0], strlen( szret[0] ), 0 );
  //     //     break;
  //     // }
  //     // else
  //     // {
  //     //     send( connfd, szret[1], strlen( szret[1] ), 0 );
  //     //     break;
  //     // }
  // }
  // close( connfd );
  // close(sock_fd);


 // char hostname[1024], hoststr[1024], proto[10];
  // char auth[1024], proxyauth[1024] = "";
  // char path[MAX_URL_SIZE];
  // char buf[1024], urlbuf[MAX_URL_SIZE];
  // int port, use_proxy, err, location_changed = 0, redirects = 0, attempts = 0;
  //const char *url1 = "http://yycloudvod1467488744.bs2dl.yy.com/djJhYmUwODI3YWFkNDI1YTlmMTU1NGViNGJhNWI1OWVkMTM4MDQ5MTdoYw.m3u8";
  // const char *url2 = "http%3A%2F%2Fgodmusic.bs2dl.yy.com%2FdmE5ZDEyMDBiMzg2ZTM4MDE5YjY0ODBlY2MwMmJlOTU2ODE3MTQzN21j";

  // //char *url1_decode = decode(url1);
  // char *url2_decode = urlDecode(url2);

  // fprintf(stdout, "url2_decode=%p\n", url2_decode);
  // //fprintf(stdout, "url2_decode=%s\n", url2_decode);

  // const char *url =
  // "http://fanhe.dwstatic.com/shortvideo/02/20163/015/3d93b82e0fb8e756838cf9c1fac10000.mp4";
  // //"http://record.vod.huanjuyun.com/xcrs/15013x03_1330846705_1554444831_1467210406_1467210388.m3u8";
  // //Location: http://183.134.12.34/shortvideo/02/20163/015/3d93b82e0fb8e756838cf9c1fac10000.mp4?traceid=9138C64C-016E-43FF-9D49-951A15A7D0F1_20160706195900 HTTP/1.0&wsiphost=local\r\n
  
  // free(url2_decode);



void urldecode_test()
{
  const char *url2 = "http://yycloudvod1467488744.bs2dl.yy.com/djEyMGZkMjY2MzU3ZjVkYzAxODU1M2E2YTU2N2MwNDJjMTM4MDkxNTgwaGM.m3u8";

  //char *url1_decode = decode(url1);
  char *url2_decode = urlDecode(url2);

  fprintf(stdout, "url2_decode=%s\n", url2_decode);

  free(url2_decode);
}