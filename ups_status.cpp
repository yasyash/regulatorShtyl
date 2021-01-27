/*
 * Copyright © 2020 Yaroslav Shkliar <mail@ilit.ru>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Research Laboratory of IT
 * www.ilit.ru on e-mail: mail@ilit.ru
 */

#include "ups_status.h"
#include <QDebug>


ups_status::ups_status( QString *ip, quint16 *port, QString *ups_username)
{


    /*
     * Initialize the SNMP library
     */

    init_snmp("snmpapp");


    /*
     * Initialize a "session" that defines who we're going to talk to
     */
    addr = ip;
    u_port = port;
    user_name = ups_username;

    snmp_sess_init( &session );                   /* set up defaults */
    session.peername = strdup(ip->toLocal8Bit().data());


    /* set the SNMP version number */
    session.version = SNMP_VERSION_3;
    const  char *community  = ups_username->toStdString().c_str();

    session.securityName = strdup(community);
    session.securityNameLen = strlen(session.securityName);
    //session.rcvMsgMaxSize = MAX_NAME_LEN;

    /*
     * Open the session
     */

    ss = snmp_open(&session);                     /* establish the session */

    if (!ss) {
        //snmp_sess_perror("ack", &session);
        //SOCK_CLEANUP;
        //exit(1);
        qDebug() << "Error opening SNMP library..."<< "\n\r";
        return;
    }

    // vars = get_data((char*)".1.3.6.1.2.1.1.1.0"); //sysDescription MIB

    vars = get_data((char*)".1.3.6.1.4.1.34498.2.6.1.11.2.0"); //detect ext. sensor connection

    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
        //print_variable(vars->name, vars->val_len, vars);
        //qDebug() << "Error: \n" << snmp_errstring(response->errstat);
        ext_sensor = (*vars->val.integer);
        if (ext_sensor){
            qDebug() << "External temperature sensor is detected ...";
            err_count--;
        }
        else {
            qDebug() << "External temperature sensor is not detected ...";
        }

    } else {
        /*
         * FAILURE: print what went wrong!
         */

        if (status == STAT_SUCCESS)
            qDebug() << "Error in packet\n Reason: \n" << snmp_errstring(response->errstat)<< "\n\r";
        else if (status == STAT_TIMEOUT)
            qDebug() << "Timeout: No response from \n" <<      session.peername<< "\n\r";
        else
            qDebug() << "Somethig error: " << ss<< "\n\r";

    }

    //  if (response)
    //     snmp_free_pdu(response);
    /*err_count = 3;
    vars = get_data((char*)".1.3.6.1.2.1.1.6.0"); //sysLocation MIB
    if (vars){
        print_variable(vars->name, vars->val_len, vars);
err_count--;
    }
    vars = get_data((char*)".1.3.6.1.2.1.1.3.0"); //sysUpTime MIB
    if (vars){

        print_variable(vars->name, vars->val_len, vars);
        err_count--;
    }

    vars = get_data((char*)"SNMPv2-SMI::mib-2.33.1.3.3.1.3.1"); //Input voltage MIB
    if (vars){
        print_variable(vars->name, vars->val_len, vars);
        err_count--;

}*/
    if (response)
        snmp_free_pdu(response);

    sample_t = 0;
    measure = new  QMap<QString, float>;
    if (err_count==0)
        qDebug() << "UPS SNMP monitoring initialization complete."<< "\n\r";
    else {
        qDebug() << "UPS SNMP monitoring initialization with issues.."<< "\n\r";

    }
}

ups_status::ups_status( QString *ip, quint16 *port, QString *ups_username, int version_snmp)
{


    /*
     * Initialize the SNMP library
     */

    init_snmp("snmpapp");


    /*
     * Initialize a "session" that defines who we're going to talk to
     */
    addr = ip;
    u_port = port;
    user_name = ups_username;

    snmp_sess_init( &session );                   /* set up defaults */
    session.peername = strdup(ip->toLocal8Bit().data());


    /* set the SNMP version number */
    session.version = SNMP_VERSION_2c;
    u_char *community  =(u_char *) ups_username->toStdString().c_str();
    QByteArray _comm = ups_username->toUtf8();

    session.community = community;
    session.community_len = (size_t)(_comm.length());
    /* set the security level to authenticated,  encrypted */
    // session.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
    /* set the authentication method to MD5 */
    /*   session.securityAuthProto = usmHMACMD5AuthProtocol;\
           session.securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol)/sizeof(oid);
           session.securityAuthKeyLen = USM_AUTH_KU_LEN;
           /* set the private proto to  */
    /*      session.securityPrivProto = usmDESPrivProtocol;
              session.securityPrivProtoLen = sizeof(usmDESPrivProtocol)/sizeof(oid);
              session.securityPrivKeyLen = USM_PRIV_KU_LEN;


             if( generate_Ku(session.securityAuthProto,
                                  session.securityAuthProtoLen,
                                  (u_char *) our_v3_passphrase, strlen(our_v3_passphrase),
                                  session.securityAuthKey,
                             &session.securityAuthKeyLen) != SNMPERR_SUCCESS) {
              qDebug() << "Error generating Ku from authentication pass phrase. \n";
             }

*/

    //session.rcvMsgMaxSize = MAX_NAME_LEN;

    /*
     * Open the session
     */

    ss = snmp_open(&session);                     /* establish the session */

    if (!ss) {
        //snmp_sess_perror("ack", &session);
        //SOCK_CLEANUP;
        //exit(1);
        qDebug() << "Server SNMP monitoring initialization with issues..."<< "\n\r";
        status = 0;
        return;
    }
    else {
        qDebug() << "Server SNMP monitoring initialization complete..."<< "\n\r";
        status = 1;

    }
    // vars = get_data((char*)".1.3.6.1.2.1.1.1.0"); //sysDescription MIB


}

void ups_status::snmp_reinit( QString *ip, quint16 *port, QString *ups_username, int version_snmp)
{


    /*
     * Initialize the SNMP library
     */

    init_snmp("snmpapp");


    /*
     * Initialize a "session" that defines who we're going to talk to
     */
    addr = ip;
    u_port = port;
    user_name = ups_username;

    snmp_sess_init( &session );                   /* set up defaults */
    session.peername = strdup(ip->toLocal8Bit().data());


    /* set the SNMP version number */
    session.version = SNMP_VERSION_2c;
    u_char *community  =(u_char *) ups_username->toStdString().c_str();
    QByteArray _comm = ups_username->toUtf8();

    session.community = community;
    session.community_len = (size_t)(_comm.length());
    /* set the security level to authenticated,  encrypted */
    // session.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
    /* set the authentication method to MD5 */
    /*   session.securityAuthProto = usmHMACMD5AuthProtocol;\
           session.securityAuthProtoLen = sizeof(usmHMACMD5AuthProtocol)/sizeof(oid);
           session.securityAuthKeyLen = USM_AUTH_KU_LEN;
           /* set the private proto to  */
    /*      session.securityPrivProto = usmDESPrivProtocol;
              session.securityPrivProtoLen = sizeof(usmDESPrivProtocol)/sizeof(oid);
              session.securityPrivKeyLen = USM_PRIV_KU_LEN;


             if( generate_Ku(session.securityAuthProto,
                                  session.securityAuthProtoLen,
                                  (u_char *) our_v3_passphrase, strlen(our_v3_passphrase),
                                  session.securityAuthKey,
                             &session.securityAuthKeyLen) != SNMPERR_SUCCESS) {
              qDebug() << "Error generating Ku from authentication pass phrase. \n";
             }

*/

    //session.rcvMsgMaxSize = MAX_NAME_LEN;

    /*
     * Open the session
     */

    ss = snmp_open(&session);                     /* establish the session */

    if (!ss) {
        //snmp_sess_perror("ack", &session);
        //SOCK_CLEANUP;
        //exit(1);
        qDebug() << "Server SNMP monitoring initialization with issues..."<< "\n\r";
        status = 0;
        return;
    }
    else {
        qDebug() << "Server SNMP monitoring initialization complete..."<< "\n\r";
        status = 1;

    }
    // vars = get_data((char*)".1.3.6.1.2.1.1.1.0"); //sysDescription MIB


}

ups_status::~ups_status()
{
    snmp_close(ss);
    //SOCK_CLEANUP;
}


void ups_status::read_voltage()
{
    vars = get_data((char*)".1.3.6.1.4.1.34498.2.6.2.4.1.0"); //Input voltage MIB
    if (vars)
    {
        voltage = float(*vars->val.integer)/10;
        measure->insert("Напряжение", voltage);
    }

}

void ups_status::read_ext_temp()
{
    vars = get_data((char*)".1.3.6.1.4.1.34498.2.6.1.11.3.0"); //read ext. temperature
    if (vars)
    {
        ext_temp = (*vars->val.integer);
        if (ext_temp > 1000)
            ext_temp = (-4096 + ext_temp);
        //measure->insert("Напряжение", voltage);
    }

}

bool ups_status::read_output_status()
{
    vars = get_data((char*)".1.3.6.1.2.1.33.1.4.1.0"); //read output status
    if (vars)
    {
        int status = (*vars->val.integer);
        if (status == 2)
            return false;
        if (status == 3)
            return true;
        //measure->insert("Напряжение", voltage);
    }
}

bool ups_status::read_srv_status()
{
    vars = get_data((char*)".1.3.6.1.4.1.21317.1.4.0"); //read server power on status
    if (vars)
    {
        int status = (*vars->val.integer);
        if (status == 0)
            return false;
        if (status == 1)
            return true;
    }
}

variable_list * ups_status::get_data(char * mib)
{
    anOID_len = MAX_OID_LEN;

    pdu = snmp_pdu_create(SNMP_MSG_GET);

    if (!read_objid(mib, anOID, &anOID_len)) {
        //snmp_perror("read_objid");
        // exit(1);
        qDebug() << "Unexpected problem while getting object ID via SNMP..."<< "\n\r";
        return nullptr;
    }


    snmp_add_null_var(pdu, anOID, anOID_len);

    /*
     * Send the Request out.
     */

    status = snmp_synch_response(ss, pdu, &response);
    //print_variable(response->variables->name, response->variables->val_len, response->variables);

    err_count++; //add hypotetic error
    if (status == STAT_SUCCESS){
        err_count--;
        return response->variables;
    }
    else {
        return nullptr;
    }
}



//set data

int ups_status::set_data_int(char *mib, char *val)
{
    anOID_len = MAX_OID_LEN;

    pdu = snmp_pdu_create(SNMP_MSG_SET);

    if (!read_objid(mib, anOID, &anOID_len)) {
        //snmp_perror("read_objid");
        // exit(1);
        qDebug() << "Unexpected problem while getting object ID via SNMP..."<< "\n\r";
        return -1;
    }

    snmp_add_var(pdu, anOID, anOID_len, 'i', val);


    /*
     * Send the Request out.
     */

    status = snmp_synch_response(ss, pdu, &response);
    //print_variable(response->variables->name, response->variables->val_len, response->variables);

    err_count++; //add hypotetic error
    if (status == STAT_SUCCESS){
        err_count--;
        return 0;
    }
    else {
        return -1;
    }
}

/*
 * simple printing of returned data
 */
int print_result (int status, struct snmp_session *sp, struct snmp_pdu *pdu)
{
    char buf[1024];
    struct variable_list *vp;
    int ix;
    struct timeval now;
    struct timezone tz;
    struct tm *tm;

    gettimeofday(&now, &tz);
    tm = localtime(&now.tv_sec);
    fprintf(stdout, "%.2d:%.2d:%.2d.%.6d ", tm->tm_hour, tm->tm_min, tm->tm_sec,
            now.tv_usec);
    switch (status) {
    case STAT_SUCCESS:
        vp = pdu->variables;
        if (pdu->errstat == SNMP_ERR_NOERROR) {
            while (vp) {
                snprint_variable(buf, sizeof(buf), vp->name, vp->name_length, vp);
                fprintf(stdout, "%s: %s\n", sp->peername, buf);
                vp = vp->next_variable;
            }
        }
        else {
            for (ix = 1; vp && ix != pdu->errindex; vp = vp->next_variable, ix++)
                ;
            if (vp) snprint_objid(buf, sizeof(buf), vp->name, vp->name_length);
            else strcpy(buf, "(none)");
            fprintf(stdout, "%s: %s: %s\n",
                    sp->peername, buf, snmp_errstring(pdu->errstat));
        }
        return 1;
    case STAT_TIMEOUT:
        fprintf(stdout, "%s: Timeout\n", sp->peername);
        return 0;
    case STAT_ERROR:
        snmp_perror(sp->peername);
        return 0;
    }
    return 0;
}

QString getStringFromUnsignedChar(unsigned char *str, size_t len)
{

    QString s;
    QString result = "";


    // Print String in Reverse order....
    for ( ulong i = 0; i < len; i++)
    {
        s = QString("%1").arg(str[i],0,16);

        if(s == "0"){
            s="00";
        }
        result.append(s);

    }
    return result;
}
