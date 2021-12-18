#include "ssh_cmd_ex.h"

ssh_cmd_ex::ssh_cmd_ex(QString &_ssh_ip,  quint16 &_ssh_port, QString &_ssh_user, QString & _ssh_pwd)
{
    //begin ssh session
    int rc;

    m_ssh_ip = _ssh_ip;
    m_ssh_port = _ssh_port;
    m_ssh_user = _ssh_user;
    m_ssh_pwd = _ssh_pwd;

    qDebug("Session...\n");
    m_session = ssh_new();
    if (m_session != NULL)
    {
        ssh_options_set(m_session, SSH_OPTIONS_HOST, m_ssh_ip.toLatin1());
        ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_ssh_port);
        ssh_options_set(m_session, SSH_OPTIONS_USER, m_ssh_user.toLatin1());

        qDebug("Connecting...\n");
        rc = ssh_connect(m_session);
        if (rc != SSH_OK)
        {
            error(m_session);
        } else {

            qDebug("Password Autentication...\n");
            rc = ssh_userauth_password(m_session, m_ssh_user.toLatin1(), m_ssh_pwd.toLatin1());
            if (rc != SSH_AUTH_SUCCESS)
            {
                error(m_session);
            } else {
                qDebug("Channel...\n");
                m_channel = ssh_channel_new(m_session);
                if (m_channel == NULL)
                {
                    error(m_session);
                } else {
                    qDebug("Opening...\n");
                    rc = ssh_channel_open_session(m_channel);
                    if (rc != SSH_OK) {
                        error(m_session);
                    } else {
                        qDebug("SSH session is opened!\n") ;
                    }
                }
            }
        }
    } else {
        qDebug("opening error!\n");
    }

}
ssh_cmd_ex::~ssh_cmd_ex()
{

}

bool ssh_cmd_ex::doCmd(QString _str)
{
    int rc;
    char buffer[1024];
    unsigned int nbytes;

    qDebug("Executing remote command...\n");
    rc = ssh_channel_request_exec(m_channel, _str.toLatin1());
    if (rc != SSH_OK) error(m_session);

    qDebug() << "Command send:\n" << QString(_str);
    nbytes = uint( ssh_channel_read(m_channel, buffer, sizeof(buffer), 0));
    qDebug() << "\nReceived:\n" << QString(buffer).remove(QRegExp("[\\n{\\n}]"));
    while (nbytes > 0) {
        //      fwrite(buffer, 1, nbytes, stdout);
        memset( buffer, '\0', sizeof(char)*1024);
        nbytes = ssh_channel_read(m_channel, buffer, sizeof(buffer), 0);
        if (nbytes)
            qDebug() << QString(buffer).remove(QRegExp("[\\n{\\n}]"));
    }
    return true;
}

void ssh_cmd_ex::free_channel(ssh_channel channel) {
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}

void ssh_cmd_ex::free_session(ssh_session session) {
    ssh_disconnect(session);
    ssh_free(session);
}

void ssh_cmd_ex::error(ssh_session session) {
    qDebug( "Error SSH: %s\n", ssh_get_error(session));
    free_session(session);
}

bool ssh_cmd_ex::sshSession_is_open()
{
    return ssh_is_connected(m_session);
}

bool ssh_cmd_ex::sshChannel_is_open()
{
    bool _ok = ssh_channel_is_open(m_channel);

    return  _ok;
}



