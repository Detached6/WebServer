#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;

    enum METHOD {
        GET = 0,
        POST
    };

    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0, // 解析请求行
        CHECK_STATE_HEADER, // 解析请求头
        CHECK_STATE_CONTENT // 解析请求体
    };

    enum HTTP_CODE {
        NO_REQUEST, // 未解析
        GET_REQUEST, // 解析成功
        BAD_REQUEST, // 解析失败
        NO_RESOURCE, // 无资源
        FORBIDDEN_REQUEST, // 无权限
        FILE_REQUEST, // 有资源
        INTERNAL_ERROR, // 内部错误
        CLOSED_CONNECTION // 关闭连接
    };

    enum LINE_STATUS {
        LINE_OK = 0, // 解析成功
        LINE_BAD, // 解析失败
        LINE_OPEN // 解析中
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in& addr, char *, int, int, string user, string passwd, string sqlName);

    void close_conn(bool real_close = true);
    
    void process();
    
    bool read_once();
    
    bool write();

    sockaddr_in *get_address() {
        return &m_address;
    }
    void initmysql_result(connection_pool* connPool); // 初始化数据库连接池
    int timer_flag; // 是否开启定时器
    int improv; // 是否启用写模式

private:
    void init();

    HTTP_CODE process_read();

    bool process_write(HTTP_CODE ret);

    HTTP_CODE parse_request_line(char *text);

    HTTP_CODE parse_headers(char *text);

    HTTP_CODE parse_content(char *text);

    HTTP_CODE do_request(); 

    char *get_line() {
        return m_read_buf + m_start_line;
    }

    LINE_STATUS parse_line();

    void unmap();

    bool add_response(const char *format,...); // 添加响应信息
    bool add_content(const char *content); // 添加响应体
    bool add_status_line(int status, const char *title); // 添加状态行
    bool add_headers(int content_length); // 添加响应头
    bool add_content_type(); // 添加响应头
    bool add_content_length(int content_length); // 添加响应头
    bool add_linger(); // 添加响应头
    bool add_blank_line(); // 添加空行

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state; //读为0，写为1

private:
    int m_sockfd; // 客户端套接字
    sockaddr_in m_address; // 客户端地址信息
    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx; // 读缓冲区中已经读入的字符个数
    int m_checked_idx; // 当前正在分析的字符位置
    int m_start_line; // 当前正在解析的行的起始位置
    
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx; // 写缓冲区中已经发送的字符个数

    CHECK_STATE m_check_state; // 主状态机当前所处的状态
    METHOD m_method; // 请求方法

    char m_real_file[FILENAME_LEN]; // 客户请求的目标文件的完整路径
    char *m_url; // 客户请求的目标文件的文件名
    char *m_version; // 协议版本号
    char *m_host; // 主机名
    int m_content_length; // http请求的消息体的长度
    bool m_linger; // 是否保持连接
    char *m_file_address; // 客户请求的目标文件被mmap到内存中的起始位置
    struct stat m_file_stat; // 目标文件的状态
    struct iovec m_iv[2]; 
    int m_iv_count; // mmap内存块的数量
    int cgi; //是否启用的POST   
    char *m_string; // 存储请求头数据的起始位置
    int bytes_to_send; // 将要发送的字节数
    int bytes_have_send; // 已经发送的字节数
    char *doc_root; // 服务器的根目录

    map<string, string> m_users;
    int m_TRIGMode; // 触发模式
    int m_close_log; // 是否关闭日志

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100]; 
};

#endif