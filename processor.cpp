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

// -slaveid 1 3 11 12 15 -port /dev/ttyr00 -baud 9600 -data 8 -stop 1 -parity none -db weather -user weather -pwd 31415 -dustip 192.168.1.3 -dustport 3602 -alarmip 192.168.1.110 -alarmport 5555 -upsip 192.168.1.120 -upsport 3493 -upsuser liebert -meteoip 192.168.1.200 -meteoport 22222 -polltime 10 -verbose

#include "processor.h"
#include "app.h"
#include <QDebug>
#include <QSqlQuery>
#include <QRegExp>
#include <QDateTime>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlField>

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

processor::processor(QObject *_parent,    QStringList *cmdline) : QObject (_parent),
    verbose(false)

{

    QStringList cmdline_args =  *cmdline;

    int _verbose = cmdline_args.indexOf("-verbose");
    if (_verbose > 0)
    {
        verbose = true;
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
    m_conn->setHostName("localhost");
    m_conn->setDatabaseName(db);
    m_conn->setUserName(user);
    m_conn->setPassword(pw);


    bool status = m_conn->open();
    if (!status)
    {
        //releaseModbus();

        qDebug() << ( QString("Connection error: " + m_conn->lastError().text()).toLatin1().constData()) <<   " \n\r";
        //   exit(-1);

    }




    m_transactTimer = new QTimer(this);
    m_pollTimer = new QTimer(this);

    connect( m_pollTimer, SIGNAL(timeout()), this, SLOT(readSocketStatus()));
    connect( m_transactTimer, SIGNAL(timeout()), this, SLOT(transactionDB()));

    m_pollTimer->start(5000);
    m_transactTimer->start(60000   );

    m_uuid = new  QMap<QString, QUuid>;
    m_data = new  QMap<QString, float>;
    m_measure =  new QMap<QString, int>;

    //this->ms_data = new  QMap<QString, int>;
    //this->ms_measure =  new QMap<QString, int>;





    //startTransactTimer(m_conn);    //start transaction timer

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

    /* QSqlQuery query = QSqlQuery(*m_conn);
    QSqlQuery query_log = QSqlQuery(*m_conn);


    int val;
    float average;
    QString tmp_time = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss"); //all SQL INSERTs should be at the same time

    //prepare for trouble logging
    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                      "VALUES ( :date_time, :type, :descr)");
*/



    //Sensor data processing



    qDebug() << "Напряжение мин. = "<< m_data->value("Напряжение мин.") << " В.\n\r" << "Напряжение макс. = "<< m_data->value("Напряжение макс.") << " В.\n\r";

   // m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
     m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.3.0","0");
}
void processor::startTransactTimer( QSqlDatabase *conn) //start by signal dbForm
{


    //m_conn = conn;
    QSqlQuery *query= new QSqlQuery ("select * from equipments where is_present = 'true' and measure_class = 'data' order by date_time_in", *conn);
    qDebug() << "Devices have been added, status is " <<   query->isActive()<< " and err " << query->lastError().text() << "\n\r";
    query->first();
    QSqlRecord rec = query->record();

    m_transactTimer->start(rec.field("average_period").value().toInt() *1000);
    if( !m_pollTimer->isActive() ) m_transactTimer->stop();

    m_uuidStation  = new QUuid(rec.field("idd").value().toUuid());


    for (int i = 0; i < query->size(); i++ )
    {
        qDebug() << query->value("typemeasure").toString() << "  -----  "<< query->value("serialnum").toUuid() <<"\n\r";

        m_uuid->insert( query->value("typemeasure").toString(), query->value("serialnum").toUuid());
        query->next();
    }
    query->finish();
    //    query->~QSqlQuery();

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

}




