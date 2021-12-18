/*
 * Copyright © 2020-2021 Yaroslav Shkliar <mail@ilit.ru>
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


#include "processor.h"
#include "app.h"
#include <QDebug>
#include <QSqlQuery>
#include <QRegExp>
#include <QDateTime>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlField>
#include <QThread>
#include <QProcess>

#include <errno.h>

extern _App	*globalApp;

QMap<QString, int> *processor::ms_range = new QMap<QString, int>;
QMap<QString, int> *processor::ms_data = new QMap<QString, int>;
QMap<QString, int> *processor::ms_measure = new QMap<QString, int>;

//processor::ms_range->insert("Пыль общая", 1000);
//ms_range->insert("PM1", 1000);
//ms_range->insert("PM2.5", 1000);
//ms_range->insert("PM4", 1000);
//ms_range->insert("PM10", 1000);

processor::processor(QObject *_parent, QStringList *cmdline) : QObject (_parent),
    verbose(false)

{

    cmdline_args =  *cmdline;

    int _verbose = cmdline_args.indexOf("-verbose");
    if (_verbose > 0)
    {
        verbose = true;
        qDebug () << "\n\r**********************************************************\n\r           Temperature SNMP driver version " <<  (APP_VERSION) << "\n\r*************************************************\n\r";

    }

    int _ver = cmdline_args.indexOf("-version");
    if (_ver > 0)
    {
        qDebug () << "\n\r**********************************************************\n\r           Temperature SNMP driver version" <<   (APP_VERSION) << "\n\r*************************************************\n\r";
    }

    //parse ssh parameters
    if (cmdline_args.indexOf("-sship") > -1)
        m_ssh_ip = cmdline_args.value(cmdline_args.indexOf("-sship") +1);
    if (m_ssh_ip == "")
    {
        qDebug ( "SSH IP address is not set.\n\r");
    }
    else
    {
        m_ssh_port = cmdline_args.value(cmdline_args.indexOf("-sshport") +1).toUShort();
        if (m_ssh_port <= 0)
        {
            qDebug ("SSH port error:  expected parameter\n\r");
        }
        else
        {
            m_ssh_user = cmdline_args.value(cmdline_args.indexOf("-sshuser") +1);
            if (m_ssh_user == "")
            {
                qDebug ("SSH user error:  expected parameter\n\r");
            } else {
                m_ssh_pwd = cmdline_args.value(cmdline_args.indexOf("-sshpwd") +1);
                if (m_ssh_pwd ==""){
                    qDebug ("Password user error:  expected parameter\n\r");
                }
                //m_ssh_command = cmdline_args.value(cmdline_args.indexOf("-sshcommand") +1).toUShort();
                // if (m_ssh_user == "")
                // {
                //    qDebug ("SSH command error:  expected parameter\n\r");
                //} else {
                m_ssh_cmdr = new ssh_cmd_ex(m_ssh_ip, m_ssh_port, m_ssh_user, m_ssh_pwd);
                // }

            }
        }
    }


    //parse command
    int _ind = cmdline_args.indexOf("-command");

    if (_ind > -1){

        _command = cmdline_args.value(cmdline_args.indexOf("-command") +1);
        if (_command != "")
        {
            qDebug ( "Command is set.\n\r");
        }
    }
    // UPS init
    m_ups_ip = cmdline_args.value(cmdline_args.indexOf("-upsip") +1);
    if (m_ups_ip == "")
    {
        qDebug ( "IP address of UPS is not set.\n\r");
    }
    else
    {

        m_ups_port = cmdline_args.value(cmdline_args.indexOf("-upsport") +1).toUShort();
        if (m_ups_port <= 0)
        {
            qDebug ("UPS port error:  expected parameter\n\r");
        }
        else
        {
            m_ups_username = cmdline_args.value(cmdline_args.indexOf("-upsuser") +1);

            m_ups = new ups_status(&m_ups_ip, &m_ups_port, &m_ups_username);
            //qDebug() << "UPS model: "<< QString::fromStdString(m_ups->m_model->getValue().data()[0]) <<"\n Voltage: "
            //<< QString::fromStdString(m_ups->m_voltage->getValue().data()[0]);
        }
    }

    m_srv_ip = cmdline_args.value(cmdline_args.indexOf("-srvip") +1);
    if (m_ups_ip == "")
    {
        qDebug ( "IP address of Server is not set.\n\r");
    }
    else
    {

        m_srv_port = cmdline_args.value(cmdline_args.indexOf("-srvport") +1).toUShort();
        if (m_ups_port <= 0)
        {
            qDebug ("Server SNMP port error:  expected parameter\n\r");
        }
        else
        {
            m_srv_username = cmdline_args.value(cmdline_args.indexOf("-srvuser") +1);
            // m_srv_auth =  cmdline_args.value(cmdline_args.indexOf("-srvauth") +1);
            //   m_srv_prv = cmdline_args.value(cmdline_args.indexOf("-srvprv") +1);
            m_srv = new ups_status(&m_srv_ip, &m_srv_port, &m_srv_username, 2);
            //qDebug() << "UPS model: "<< QString::fromStdString(m_ups->m_model->getValue().data()[0]) <<"\n Voltage: "
            //<< QString::fromStdString(m_ups->m_voltage->getValue().data()[0]);
        }
    }

    QString dbhost = cmdline_args.value(cmdline_args.indexOf("-dbhost") +1);
    if (dbhost == "")
    {
        // releaseModbus();

        qDebug ( "Host address of the database parameter is not set...\n\r");
        //exit(-1);

    }


    QString db = cmdline_args.value(cmdline_args.indexOf("-db") +1);
    if (db == "")
    {
        // releaseModbus();

        qDebug ( "Fatal error: wrong data of the database parameter\n\r");
        //exit(-1);

    }

    QString user = cmdline_args.value(cmdline_args.indexOf("-user") +1);
    if (user == "")
    {
        // releaseModbus();

        qDebug ( "Fatal error: wrong data of the user parameter\n\r");
        //exit(-1);

    }

    QString pw = cmdline_args.value(cmdline_args.indexOf("-pwd") +1);
    if (pw == "")
    {
        // releaseModbus();

        qDebug ( "Fatal error: wrong data of the password parameter\n\r");
        // exit(-1);

    }

    m_conn = new QSqlDatabase();
    *m_conn = QSqlDatabase::addDatabase("QPSQL");

    if (dbhost == "")
    {
        m_conn->setHostName("localhost");
    }
    else
    {
        m_conn->setHostName(dbhost);
    }
    m_conn->setDatabaseName(db);
    m_conn->setUserName(user);
    m_conn->setPassword(pw);


    bool status = m_conn->open();
    if (!status)
    {
        //releaseModbus();

        qDebug() << ( QString("Connection error: " + m_conn->lastError().text()).toLatin1().constData()) <<   " \n\r";

        // Meteostation init if connection not open
        //  -meteoip 192.168.1.200 -meteoport 22222

        m_meteo_ip = cmdline_args.value(cmdline_args.indexOf("-meteoip") +1);
        if (cmdline_args.indexOf("-meteoip"))
        {
            if (!m_ivtm)
            {
                if (cmdline_args.indexOf("-ivtmip")>-1)
                    m_ivtm_ip = cmdline_args.value(cmdline_args.indexOf("-ivtmip") +1);
                if (m_ivtm_ip == "")
                {
                    qDebug ( "IP address of meteostation or thermohygrometer is not set.\n\r");
                }
                else
                {
                    m_ivtm_port = cmdline_args.value(cmdline_args.indexOf("-ivtmport") +1).toUShort();
                    if (m_ivtm_port <= 0)
                    {
                        qDebug ("IP address of meteostation or IVTM TCP Modbus port error:  expected parameter\n\r");
                    }
                    else
                    {
                        QUrl _url;
                        _url.setHost(m_ivtm_ip);
                        _url.setPort(m_ivtm_port);

                        m_ivtm = new ivtm(_url, 0, 4, 1, 1000, this );

                    }

                }
            } // end of IVTM
        }
        else
        {
            m_meteo_port = cmdline_args.value(cmdline_args.indexOf("-meteoport") +1).toUShort();
            if (m_meteo_port <= 0)
            {
                qDebug ( "Port of meteostation is not set.");
            }
            else
            {
                QString meteoparams = cmdline_args.value(cmdline_args.indexOf("-meteoparams") +1);



                QSqlQuery *query= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внутренняя' order by date_time desc", *m_conn);
                qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                query->first();
                QSqlRecord rec = query->record();

                float temp_in = (rec.field("measure").value().toFloat());
                query->finish();

                QSqlQuery *queryout= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внешняя' order by date_time desc", *m_conn);
                qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                queryout->first();
                rec = queryout->record();

                float temp_out = (rec.field("measure").value().toFloat());
                query->finish();


                QString model="";
                if (cmdline_args.indexOf("-meteomodel") > -1){
                    model = cmdline_args.value(cmdline_args.indexOf("-meteomodel") +1);
                }

                if (model == ""){
                    if (meteoparams == "db")
                    {
                        m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out);
                    } else{
                        m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port);
                    }

                } else {
                    if (meteoparams == "db")
                    {
                        m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out, &model);
                    } else {
                        m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, &model);

                    }

                }
            }

        }


    } else {
        qDebug() << "Connected... ";
        is_conn = true;
    }

    m_transactTimer = new QTimer(this);
    m_pollTimer = new QTimer(this);

    connect( m_pollTimer, SIGNAL(timeout()), this, SLOT(readSocketStatus()));
    connect( m_transactTimer, SIGNAL(timeout()), this, SLOT(transactionDB()));

    m_pollTimer->start(12000);
    //m_transactTimer->start(60000   );

    m_uuid = new  QMap<QString, QUuid>;
    m_data = new  QMap<QString, float>;
    m_measure =  new QMap<QString, int>;

    //this->ms_data = new  QMap<QString, int>;
    //this->ms_measure =  new QMap<QString, int>;
    if (is_conn){
        startTransactTimer(m_conn);    //start transaction timer
    } else {
        {
            m_transactTimer->start(60000); //change
        }
    }
    //range coefficients init
    m_range = new QMap<QString, int>;
    //this-> ms_range = new QMap<QString, int>;


    m_range->insert("Напряжение мин.", 1);
    m_range->insert("Напряжение макс.", 1);



    //UPS data init
    if (m_ups){
        m_ups->read_voltage();
        m_data->insert("Напряжение мин.", m_ups->voltage);
        //m_measure->insert("Напряжение мин.", 1);
        m_data->insert("Напряжение макс.", m_ups->voltage);
        //m_measure->insert("Напряжение макс.", 1);
    }


}


processor::~processor()
{

}

static inline QString embracedString( const QString & s )
{
    return s.section( '(', 1 ).section( ')', 0, 0 );
}


static inline int stringToHex( QString s )
{
    return s.replace( "0x", "" ).toInt( NULL, 16 );
}



void processor::transactionDB(void)
{


    QMap<QString, QUuid>::iterator sensor;
    QMap<QDateTime, QString>::iterator event_iterator;
    QMap<QString, float>::iterator meteo_iterator;
    QString _key;

    QSqlQuery query = QSqlQuery(*m_conn);
    QSqlQuery query_log = QSqlQuery(*m_conn);
    QString tmp_time = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss"); //all SQL INSERTs should be at the same time


    /*

    int val;
    float average;

    //prepare for trouble logging
    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                      "VALUES ( :date_time, :type, :descr)");
*/

    qDebug() << "Напряжение мин. = "<< m_data->value("Напряжение мин.") << " В.\n\r" << "Напряжение макс. = "<< m_data->value("Напряжение макс.") << " В.\n\r";
    if (m_ups)
    { //command execute
        if (_command == "down")
        {
            QStringList params;

            qDebug() << "ESXi Status:  " << (m_ssh_cmdr->sshChannel_is_open() ? "host running" : "host down");

            params << "-c" <<"1"<< "-q" <<m_ssh_ip;

            if ( m_ssh_cmdr->sshChannel_is_open())
            {
                m_ssh_cmdr->doCmd("/vmfs/volumes/datastore1/down.sh"); //remote script

                while (QProcess::execute(QString("ping"), params ) == 0 ) {
                }
                qDebug() << "ESXi host is down...";

                QThread::msleep(5000); //5 sec waiting for shutdown

                m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
                m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");
                qDebug() << "UPS down command is executed...";

            }

            exit (0);
        }

        if (_command == "rise")
        {
            m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.3.0","1");
            m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");
            qDebug() << "UPS rise command is executed...";
            exit (0);
        }

        if (_command == "serverup")
        {
            if (m_srv){
                m_srv->set_data_int(".1.3.6.1.4.1.21317.1.4.0","1");
                qDebug() << "Server power on command is executed...";
                exit (0);
            }

        }
        // end of command section

        m_ups->read_ext_temp();
        if (!m_ups->is_read)
        {
            if (abs(m_ups->ext_temp) <35)
            {
                m_data->insert("Темп. сенсор ИБП", m_ups->ext_temp);
                m_measure->insert("Темп. сенсор ИБП", 1);
                m_ups->is_read = true;
            } else {
                m_data->insert("Темп. сенсор ИБП", 20.2f);
                m_measure->insert("Темп. сенсор ИБП", 1);
                m_ups->is_read = true;
                qDebug() << "UPS temperature sensor value is inconsistent when started! T = "<< m_ups->ext_temp<< " C.\n\r";


            }

        }

        if (abs(m_data->value("Темп. сенсор ИБП") - m_ups->ext_temp) > 10)
        {
            qDebug() << "UPS temperature sensor value is inconsistent! T = "<< m_ups->ext_temp<< " C.\n\r";

        } else {
            m_data->insert("Темп. сенсор ИБП", m_ups->ext_temp);
            m_measure->insert("Темп. сенсор ИБП", 1); //we don't interested in average temperature - we need a value per sample
        }

        //qDebug() << "Темп. сенсор ИБП = "<< m_ups->ext_temp << " C.";
    }

    is_conn = m_conn->isOpen();
    if (!is_conn) {
        if (!m_conn->isOpen())
            m_conn->open();

        if(!m_conn->isOpen())
        {
            qDebug() << "Unable to reopen database connection and reload sensors!\n\r";

            if (!m_meteo)
            {
                // Meteostation init
                //  -meteoip 192.168.1.200 -meteoport 22222

                m_meteo_ip = cmdline_args.value(cmdline_args.indexOf("-meteoip") +1);
                if (cmdline_args.indexOf("-meteoip"))
                {
                    if (!m_ivtm)
                    {
                        if (cmdline_args.indexOf("-ivtmip")>-1)
                            m_ivtm_ip = cmdline_args.value(cmdline_args.indexOf("-ivtmip") +1);
                        if (m_ivtm_ip == "")
                        {
                            qDebug ( "IP address of meteostation or thermohygrometer is not set.\n\r");
                        }
                        else
                        {
                            m_ivtm_port = cmdline_args.value(cmdline_args.indexOf("-ivtmport") +1).toUShort();
                            if (m_ivtm_port <= 0)
                            {
                                qDebug ("IP address of meteostation or IVTM TCP Modbus port error:  expected parameter\n\r");
                            }
                            else
                            {
                                QUrl _url;
                                _url.setHost(m_ivtm_ip);
                                _url.setPort(m_ivtm_port);

                                m_ivtm = new ivtm(_url, 0, 4, 1, 1000, this );

                            }

                        }
                    }// end of IVTM
                }
                else
                {
                    m_meteo_port = cmdline_args.value(cmdline_args.indexOf("-meteoport") +1).toUShort();
                    if (m_meteo_port <= 0)
                    {
                        qDebug ( "Port of meteostation is not set.");
                    }
                    else
                    {
                        QString meteoparams = cmdline_args.value(cmdline_args.indexOf("-meteoparams") +1);



                        QSqlQuery *query= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внутренняя' order by date_time desc", *m_conn);
                        qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                        query->first();
                        QSqlRecord rec = query->record();

                        float temp_in = (rec.field("measure").value().toFloat());
                        query->finish();

                        QSqlQuery *queryout= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внешняя' order by date_time desc", *m_conn);
                        qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                        queryout->first();
                        rec = queryout->record();

                        float temp_out = (rec.field("measure").value().toFloat());
                        query->finish();


                        QString model="";
                        if (cmdline_args.indexOf("-meteomodel") > -1){
                            model = cmdline_args.value(cmdline_args.indexOf("-meteomodel") +1);
                        }

                        if (model == ""){
                            if (meteoparams == "db")
                            {
                                m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out);
                            } else{
                                m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port);
                            }

                        } else {
                            if (meteoparams == "db")
                            {
                                m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out, &model);
                            } else {
                                m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, &model);

                            }

                        }
                    }

                }
            }
        } else {
            startTransactTimer(m_conn);    //reload sensors and restart transaction timer
            is_conn = true;

            if (m_meteo)
                m_meteo->~MeteoTcpSock();

            qDebug() << "Database reconnection and reload sensors is done!\n\r";

        }

        if (m_ups)
        {
            bool _status = m_ups->read_output_status();
            if (_status){//if output is connected
                qDebug() << "Output is work!\n\r";


            } else {
                if (m_meteo){
                    float _temp_in = m_meteo->measure->value("temp_in");//momental measure
                    if (( _temp_in > 14.9f) && (_temp_in < 35.01f))
                    {//if output is not connected
                        m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.3.0","1");
                        m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");

                        m_watchdogTimerLow = 0;
                        m_watchdogTimerHigh = 0;

                        qDebug() << "Output is not work! Try to start!\n\r";

                    }

                    qDebug() << "Temperature on meteostation is "<< _temp_in <<" degrees Celcius\n\r";

                } else {
                    if (m_ivtm)
                    {
                        // IVTM data reading with gamma function as callback method meteo data filling

                        if (m_ivtm){

                            m_ivtm->doQueryTemp( [this](const QString & _key, const float & _val){
                                float _temp_in = _val;//momental measure
                                if (( _temp_in > 14.9f) && (_temp_in < 35.01f))
                                {//if output is not connected
                                    m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.3.0","1");
                                    m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");

                                    m_watchdogTimerLow = 0;
                                    m_watchdogTimerHigh = 0;

                                    qDebug() << "Output is not work! Try to start!\n\r";

                                }
                                qDebug() << "Temperature on thermohygrometer is "<< _temp_in <<" degrees Celcius\n\r";
                            });
                        }
                    } else {
                        if ((m_ups->ext_temp > 14)&&(m_ups->ext_temp < 36)){//if output is not connected
                            m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.3.0","1");
                            m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");

                            m_watchdogTimerLow = 0;
                            m_watchdogTimerHigh = 0;

                            qDebug() << "Output is not work! Try to start!\n\r";

                        }
                        qDebug() << "Temperature of UPS sensor is "<< m_ups->ext_temp <<" degrees Celcius\n\r";
                    }

                }//


            }
        }

        if (m_srv) //try to start server
        {
            if (!m_srv->status)
            {
                //m_srv->snmp_reinit(&m_srv_ip, &m_srv_port, &m_srv_username, 2);
            } else {
                bool _statussrv = m_srv->read_srv_status();
                if (_statussrv){//if output is connected
                    qDebug() << "Server is work!\n\r";


                } else {
                    if (m_meteo){
                        float _temp_in = m_meteo->measure->value("temp_in");//momental measure
                        if (( _temp_in > 14.9f) && (_temp_in < 35.01f))
                        {//if meteostation exist
                            m_srv->set_data_int(".1.3.6.1.4.1.21317.1.4.0","1");

                            qDebug() << "Server is not running! Try to start the server via SNMP!\n\r";
                        }
                        qDebug() << "Temperature on meteostation is " << _temp_in << " degrees Celcius\n\r";

                    } else {

                        if (m_ivtm)
                        {
                            // IVTM data reading with gamma function as callback method meteo data filling

                            if (m_ivtm){

                                m_ivtm->doQueryTemp( [this](const QString & _key, const float & _val){
                                    float _temp_in = _val;//momental measure
                                    if (( _temp_in > 14.9f) && (_temp_in < 35.01f))
                                    {//if output is not connected
                                        m_srv->set_data_int(".1.3.6.1.4.1.21317.1.4.0","1");

                                        qDebug() << "Server is not running! Try to start the server via SNMP!\n\r";

                                    }
                                    qDebug() << "Temperature on thermohygrometer is "<< _temp_in <<" degrees Celcius\n\r";
                                });
                            }
                        } else {
                            if ((m_ups->ext_temp > 14)&&(m_ups->ext_temp < 36)){//if meteostation is not connected
                                m_srv->set_data_int(".1.3.6.1.4.1.21317.1.4.0","1");

                                qDebug() << "Server is not running! Try to start the server via SNMP!\n\r";
                            }
                            qDebug() << "Temperature on UPS temperature sensor is " << m_ups->ext_temp << " degrees Celcius\n\r";

                        }
                    }
                }
                //qDebug() << "Server is not work! Try to start!\n\r";
            }
        }
    }


    //Sensor data processing
    for (sensor = m_uuid->begin(); sensor != m_uuid->end(); ++sensor)
    {
        _key = sensor.key();
        if (_key != "Темп. внутренняя")
        {
            query.prepare("INSERT INTO sensors_data (idd, serialnum, date_time, typemeasure, measure, is_alert) "
                          "VALUES (:idd, :serialnum, :date_time, :typemeasure, :measure, false)");


            query.bindValue(":idd", QString(m_uuidStation->toString()).remove(QRegExp("[\\{\\}]")));
            query.bindValue(":serialnum",  QString(m_uuid->value(sensor.key()).toString()).remove(QRegExp("[\\{\\}]")));
            query.bindValue(":date_time", tmp_time);
            query.bindValue(":typemeasure", sensor.key());
            query.bindValue(":measure", m_data->value(sensor.key()) );
            qDebug() << "\n\rTransaction prepare: \n\r idd === "<< QString(m_uuidStation->toString()).remove(QRegExp("[\\{\\}]")) << "\n\r serial === " <<  QString(m_uuid->value(sensor.key()).toString()).remove(QRegExp("[\\{\\}]")) <<
                        "\n\r date_time ===" << QDateTime::currentDateTime().toString( "yyyy-MM-ddThh:mm:ss") << "\n\r typemeasure " <<  sensor.key() <<
                        "\n\r measure ===" << m_data->value(sensor.key()) <<"\n\r";
            if (!m_conn->isOpen())
                m_conn->open();

            if(!m_conn->isOpen())
            {
                qDebug() << "Unable to reopen database connection!\n\r";
            }
            else
            {
                if (verbose)
                {
                    qDebug() << "Transaction status is " << ((query.exec() == true) ? "successful!\n\r" :  "not complete! \n\r");
                    qDebug() << "The last error is " << (( query.lastError().text().trimmed() == "") ? "absent " : query.lastError().text()) << "\n\r";
                }
                else
                {
                    if (query.exec())
                    {
                        qDebug() << "Insertion is successful!\n\r";
                    }
                    else
                    {
                        qDebug() << "Insertion is not successful!\n\r";

                    }
                }

                query.finish();
                //  query.~QSqlQuery();

                //m_data->remove(sensor.key());
                //m_measure->remove(sensor.key());

            }
        }
        else
        {
            if (is_conn) { //member's properties
                QSqlQuery *queryt = new QSqlQuery ("select * from sensors_data where serialnum = '"+ QString(m_uuid->value(sensor.key()).toString()).remove(QRegExp("[\\{\\}]")) +"' order by date_time desc", *m_conn);
                queryt->first();
                QSqlRecord rec = queryt->record();

                if (queryt->size() > 0){
                    m_data->insert("Темп. внутренняя", rec.field("measure").value().toFloat());
                }
                else {
                    m_data->insert("Темп. внутренняя", m_ups->ext_temp);
                }
                qDebug() << "Темп. внутренняя град С = "<< ((queryt->size() > 0) ? rec.field("measure").value().toString() : "данных пока нет ...");
                qDebug() << "Темп. сенсор ИБП = "<< m_ups->ext_temp << " C. \n\r";

                queryt->finish();

                queryt = new QSqlQuery ("select * from stations where idd = '"+ QString(m_uuidStation->toString()).remove(QRegExp("[\\{\\}]")) +"'", *m_conn);
                queryt->first();
                rec = queryt->record();
                queryt->finish();



                //if temperature measures don't correlate
                if (abs(m_data->value("Темп. сенсор ИБП") - m_data->value("Темп. внутренняя")) > 10)
                {

                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 404 );
                    query_log.bindValue(":descr", "Расхождение измерений температуры метеокомплекса и ИБП более 10 градусов на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Расхождение измерений температуры метеокомплекса и ИБП более 10 градусов на " + rec.field("namestation").value().toString();

                }



                if (m_data->value("Темп. внутренняя") > 35){ //if temperature is too high

                    if (m_watchdogTimerHigh > 5){


                        query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                          "VALUES ( :date_time, :type, :descr)");
                        query_log.bindValue(":date_time", tmp_time );
                        query_log.bindValue( ":type", 120 );
                        query_log.bindValue(":descr", "Электропитание оборудования отключено по причине критического превышения температуры более 35 градусов Цельсия на " + rec.field("namestation").value().toString());
                        query_log.exec();
                        query_log.finish();

                        //m_conn->close();

                        //m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
                        //m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");

                        qDebug() << "Электропитание оборудования"<< (shutdown() ? " отключено " : "не отключено (ошибка комманды) - ") <<"по причине критического превышения температуры более 35 градусов Цельсия на  " + rec.field("namestation").value().toString();

                        if (!m_meteo)
                        {
                            // Meteostation init
                            //  -meteoip 192.168.1.200 -meteoport 22222

                            m_meteo_ip = cmdline_args.value(cmdline_args.indexOf("-meteoip") +1);
                            if (cmdline_args.indexOf("-meteoip"))
                            {
                                if (!m_ivtm)
                                {
                                    if (cmdline_args.indexOf("-ivtmip")>-1)
                                        m_ivtm_ip = cmdline_args.value(cmdline_args.indexOf("-ivtmip") +1);
                                    if (m_ivtm_ip == "")
                                    {
                                        qDebug ( "IP address of meteostation or thermohygrometer is not set.\n\r");
                                    }
                                    else
                                    {
                                        m_ivtm_port = cmdline_args.value(cmdline_args.indexOf("-ivtmport") +1).toUShort();
                                        if (m_ivtm_port <= 0)
                                        {
                                            qDebug ("IP address of meteostation or IVTM TCP Modbus port error:  expected parameter\n\r");
                                        }
                                        else
                                        {
                                            QUrl _url;
                                            _url.setHost(m_ivtm_ip);
                                            _url.setPort(m_ivtm_port);

                                            m_ivtm = new ivtm(_url, 0, 4, 1, 1000, this );

                                        }

                                    }

                                }// end of IVTM
                            }
                            else
                            {
                                m_meteo_port = cmdline_args.value(cmdline_args.indexOf("-meteoport") +1).toUShort();
                                if (m_meteo_port <= 0)
                                {
                                    qDebug ( "Port of meteostation is not set.");
                                }
                                else
                                {
                                    QString meteoparams = cmdline_args.value(cmdline_args.indexOf("-meteoparams") +1);



                                    QSqlQuery *query= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внутренняя' order by date_time desc", *m_conn);
                                    qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                                    query->first();
                                    QSqlRecord rec = query->record();

                                    float temp_in = (rec.field("measure").value().toFloat());
                                    query->finish();

                                    QSqlQuery *queryout= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внешняя' order by date_time desc", *m_conn);
                                    qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                                    queryout->first();
                                    rec = queryout->record();

                                    float temp_out = (rec.field("measure").value().toFloat());
                                    query->finish();


                                    QString model="";
                                    if (cmdline_args.indexOf("-meteomodel") > -1){
                                        model = cmdline_args.value(cmdline_args.indexOf("-meteomodel") +1);
                                    }

                                    if (model == ""){
                                        if (meteoparams == "db")
                                        {
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out);
                                        } else{
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port);
                                        }

                                    } else {
                                        if (meteoparams == "db")
                                        {
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out, &model);
                                        } else {
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, &model);

                                        }

                                    }
                                }

                            }

                        }

                    }
                    qDebug() << "\n\rTemperature dangerous state is " << m_watchdogTimerHigh << " minutes\n\r";
                    m_watchdogTimerHigh++;



                } else {
                    m_watchdogTimerHigh = 0;
                }

                if (m_data->value("Темп. внутренняя") > 30){ //if temperature is high

                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 404 );
                    query_log.bindValue(":descr", "Зафиксировано превышение температуры более 30 градусов Цельсия на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Зафиксировано на ИБП превышение температуры более 30 градусов Цельсия на " + rec.field("namestation").value().toString();

                }

                if (m_data->value("Темп. внутренняя") < 15 ){ //if temperature is too low
                    if (m_watchdogTimerLow > 5){ //waiting 5 minutes and shutdown

                        query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                          "VALUES ( :date_time, :type, :descr)");
                        query_log.bindValue(":date_time", tmp_time );
                        query_log.bindValue( ":type", 120 );
                        query_log.bindValue(":descr", "Электропитание оборудования отключено по причине критического понижения температуры менее 15 градусов Цельсия на " + rec.field("namestation").value().toString());
                        query_log.exec();
                        query_log.finish();

                        //m_conn->close();

                        //m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
                        //m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");


                        qDebug() << "Электропитание оборудования"<< (shutdown() ? " отключено " : "не отключено (ошибка комманды) - ") <<"по причине критического понижения температуры менее 15 градусов Цельсия на " + rec.field("namestation").value().toString();

                        if (!m_meteo)
                        {
                            // Meteostation init
                            //  -meteoip 192.168.1.200 -meteoport 22222

                            m_meteo_ip = cmdline_args.value(cmdline_args.indexOf("-meteoip") +1);
                            if (cmdline_args.indexOf("-meteoip"))
                            {
                                if (!m_ivtm)
                                {
                                    if (cmdline_args.indexOf("-ivtmip")>-1)
                                        m_ivtm_ip = cmdline_args.value(cmdline_args.indexOf("-ivtmip") +1);
                                    if (m_ivtm_ip == "")
                                    {
                                        qDebug ( "IP address of meteostation or thermohygrometer is not set.\n\r");
                                    }
                                    else
                                    {
                                        m_ivtm_port = cmdline_args.value(cmdline_args.indexOf("-ivtmport") +1).toUShort();
                                        if (m_ivtm_port <= 0)
                                        {
                                            qDebug ("IP address of meteostation or IVTM TCP Modbus port error:  expected parameter\n\r");
                                        }
                                        else
                                        {
                                            QUrl _url;
                                            _url.setHost(m_ivtm_ip);
                                            _url.setPort(m_ivtm_port);

                                            m_ivtm = new ivtm(_url, 0, 4, 1, 1000, this );

                                        }

                                    }
                                } // end of IVTM
                            }
                            else
                            {
                                m_meteo_port = cmdline_args.value(cmdline_args.indexOf("-meteoport") +1).toUShort();
                                if (m_meteo_port <= 0)
                                {
                                    qDebug ( "Port of meteostation is not set.");
                                }
                                else
                                {
                                    QString meteoparams = cmdline_args.value(cmdline_args.indexOf("-meteoparams") +1);



                                    QSqlQuery *query= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внутренняя' order by date_time desc", *m_conn);
                                    qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                                    query->first();
                                    QSqlRecord rec = query->record();

                                    float temp_in = (rec.field("measure").value().toFloat());
                                    query->finish();

                                    QSqlQuery *queryout= new QSqlQuery ("select * from sensors_data where typemeasure = 'Темп. внешняя' order by date_time desc", *m_conn);
                                    qDebug() << "Temp. inner status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
                                    queryout->first();
                                    rec = queryout->record();

                                    float temp_out = (rec.field("measure").value().toFloat());
                                    query->finish();


                                    QString model="";
                                    if (cmdline_args.indexOf("-meteomodel") > -1){
                                        model = cmdline_args.value(cmdline_args.indexOf("-meteomodel") +1);
                                    }

                                    if (model == ""){
                                        if (meteoparams == "db")
                                        {
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out);
                                        } else{
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port);
                                        }

                                    } else {
                                        if (meteoparams == "db")
                                        {
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, temp_in, temp_out, &model);
                                        } else {
                                            m_meteo = new MeteoTcpSock(this, &m_meteo_ip, &m_meteo_port, &model);

                                        }

                                    }
                                }

                            }

                        }

                    }
                    qDebug() << "\n\rTemperature dangerous state is " << m_watchdogTimerLow << " minutes\n\r";

                    m_watchdogTimerLow++;



                } else {
                    m_watchdogTimerLow = 0;
                }

                if (m_data->value("Темп. внутренняя") < 15){ //if temperature is low


                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 404 );
                    query_log.bindValue(":descr", "Зафиксировано на ИБП понижение температуры менее 15 градусов Цельсия на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Зафиксировано понижение температуры менее 15 градусов Цельсия на " + rec.field("namestation").value().toString();
                }
            }
        }
    }
    //  m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.3.0","0");
}

void processor::startTransactTimer( QSqlDatabase *conn) //start by signal dbForm
{


    //m_conn = conn;
    QSqlQuery *query= new QSqlQuery ("select * from equipments where measure_class = 'temp_in' and is_present ='true' order by date_time_out desc", *conn);
    qDebug() << "Devices have been added, status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
    query->first();
    QSqlRecord rec = query->record();

    m_transactTimer->start(rec.field("average_period").value().toInt() *1000); //change

    m_uuidStation  = new QUuid(rec.field("idd").value().toUuid());


    for (int i = 0; i < query->size(); i++ )
    {
        qDebug() << query->value("typemeasure").toString() << "  -----  "<< query->value("serialnum").toUuid() <<"\n\r";

        m_uuid->insert( query->value("typemeasure").toString(), query->value("serialnum").toUuid());
        query->next();
    }
    query->finish();



    query= new QSqlQuery ("select * from equipments where  ( typemeasure = 'Темп. сенсор ИБП' or  typemeasure = 'Напряжение мин.' or   typemeasure = 'Напряжение макс.') and is_present ='true'  ", *conn);
    query->first();

    for (int i = 0; i < query->size(); i++ )
    {
        m_uuid->insert( query->value("typemeasure").toString(), query->value("serialnum").toUuid());
        qDebug() << query->value("typemeasure").toString() << "  -----  "<< query->value("serialnum").toUuid() <<"\n\r";

        query->next();
    }
    query->finish();



    query->finish();

    if (!m_ssh_cmdr)
    {
        //parse ssh parameters
        if ((m_ssh_ip != "")&&(m_ssh_port > 0)&& (m_ssh_user != "")&&(m_ssh_pwd !=""))
            m_ssh_cmdr = new ssh_cmd_ex(m_ssh_ip, m_ssh_port, m_ssh_user, m_ssh_pwd);
    }
}


//Read the status of devices that are connected via TCP
void processor::readSocketStatus()
{
    QString tmp_type_measure;
    QStringList dust = {"PM1", "PM2.5", "PM4", "PM10", "PM"  };
    int tmp;


    int j = 0;
    QMap<int, int>::iterator slave;
    //if( m_tcpActive )
    //  ui->tcpSettingsWidget->tcpConnect();


    //m_liga->getLastResult();

    //UPS acqusition data reading
    if (m_ups){
        if (m_ups->err_count <10){ //minimum error threshold
            m_ups->read_voltage();
            qDebug() << "Напряжение = " << m_ups->voltage << " В. \n\r";
            if (!m_measure->value("Напряжение мин."))
            {   m_data->insert("Напряжение мин.", m_ups->voltage);
                m_measure->insert("Напряжение мин.", 1); //we don't interested in average voltage - we need lowest or highest values

            }

            if (!m_measure->value("Напряжение макс."))
            {   m_data->insert("Напряжение макс.", m_ups->voltage);
                m_measure->insert("Напряжение макс.", 1); //we don't interested in average voltage - we need lowest or highest values

            }
            if (m_ups) {
                if (m_ups->voltage < m_data->value("Напряжение мин.")){
                    m_data->insert("Напряжение мин.", m_ups->voltage);
                }

                if (m_ups->voltage > m_data->value("Напряжение макс.")){
                    m_data->insert("Напряжение макс.", m_ups->voltage);
                }
            }
        }
    }

    //Meteostation data reading
    if (m_meteo) {
        if (m_meteo->connected){
            if (m_meteo->model){
                m_meteo->sendData("E0");
            } else {
                m_meteo->sendData("LPS 2 1");//sendData("LOOP 1");
            }

        }

    }

}

bool processor::shutdown()
{
    QStringList params;

    if (m_ssh_cmdr){
        qDebug() << "ESXi Status:  " << (m_ssh_cmdr->sshChannel_is_open() ? "host running" : "host down");

        params << "-c" <<"1"<< "-q" <<m_ssh_ip;

        if ( m_ssh_cmdr->sshSession_is_open())
        {
            m_ssh_cmdr->doCmd("/vmfs/volumes/datastore1/down.sh"); //remote script

            while (QProcess::execute(QString("ping"), params ) == 0 ) {
            }
            qDebug() << "ESXi host is down...";

            QThread::msleep(5000); //5 sec waiting for shutdown

            m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
            m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");
            qDebug() << "UPS down command is executed...";
            m_ssh_cmdr->~ssh_cmd_ex();
            delete m_ssh_cmdr;
            m_ssh_cmdr = nullptr;
            return 1;

        } else {
            return 0;
        }
    } else {
        return 0;
    }

}



