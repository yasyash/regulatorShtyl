#ifndef SSH_CMD_EX_H
#define SSH_CMD_EX_H

#include <libssh/libssh.h>

#include <QString>
#include <QDebug>

class ssh_cmd_ex
{
public:
    ssh_cmd_ex(QString &_ssh_ip,  quint16 &_ssh_port, QString &_ssh_user, QString &_ssh_pwd);
    ~ssh_cmd_ex();
    bool doCmd(QString _str);
    void free_channel(ssh_channel channel);
    void free_session(ssh_session session);
    void error(ssh_session session);
    bool sshSession_is_open();
    bool sshChannel_is_open();

public:
    QString m_ssh_ip;
    quint16 m_ssh_port;
    QString m_ssh_user;
    QString m_ssh_pwd;
    QString m_ssh_command;

    ssh_session m_session;
    ssh_channel m_channel;
};

#endif // SSH_CMD_EX_H
