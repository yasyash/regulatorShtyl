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
        exit(-1);

    }




    m_transactTimer = new QTimer(this);
    m_pollTimer = new QTimer(this);

    connect( m_pollTimer, SIGNAL(timeout()), this, SLOT(readSocketStatus()));
    connect( m_transactTimer, SIGNAL(timeout()), this, SLOT(transactionDB()));

    m_pollTimer->start(5000);
    //m_transactTimer->start(60000   );


    m_uuid = new  QMap<QString, QUuid>;
    m_data = new  QMap<QString, float>;
    m_measure =  new QMap<QString, int>;

    //this->ms_data = new  QMap<QString, int>;
    //this->ms_measure =  new QMap<QString, int>;

    startTransactTimer(m_conn);    //start transaction timer

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
    {
        m_ups->read_ext_temp();
        m_data->insert("Темп. сенсор ИБП", m_ups->ext_temp);
        m_measure->insert("Темп. сенсор ИБП", 1); //we don't interested in average temperature - we need a value per sample
        //qDebug() << "Темп. сенсор ИБП = "<< m_ups->ext_temp << " C.";
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
            QSqlQuery *queryt = new QSqlQuery ("select * from sensors_data where serialnum = '"+ QString(m_uuid->value(sensor.key()).toString()).remove(QRegExp("[\\{\\}]")) +"' order by date_time desc", *m_conn);
            queryt->first();
            QSqlRecord rec = queryt->record();
            m_data->insert("Темп. внутренняя", rec.field("measure").value().toFloat());
            qDebug() << "Темп. внутренняя = "<< rec.field("measure").value().toFloat() << " C.";
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

            else
            {
                if (m_ups->ext_temp > 45){ //if temperature is too high
                    m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
                    m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");


                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 120 );
                    query_log.bindValue(":descr", "Электропитание оборудования отключено по причине критического превышения температуры более 45 градусов Цельсия на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Электропитание оборудования отключено по причине критического превышения температуры более 45 градусов Цельсия на  " + rec.field("namestation").value().toString();

                }

                if (m_ups->ext_temp > 40){ //if temperature is high



                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 404 );
                    query_log.bindValue(":descr", "Зафиксировано превышение температуры более 40 градусов Цельсия на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Зафиксировано на ИБП превышение температуры более 40 градусов Цельсия на " + rec.field("namestation").value().toString();

                }

                if (m_ups->ext_temp < 2 ){ //if temperature is too low
                    m_ups->set_data_int(".1.3.6.1.2.1.33.1.8.2.0","0");
                    m_ups->set_data_int(".1.3.6.1.4.1.34498.2.6.9.1.4.0","0");


                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 120 );
                    query_log.bindValue(":descr", "Электропитание оборудования отключено по причине критического понижения температуры менее 2 градусов Цельсия на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Электропитание оборудования отключено по причине критического понижения температуры менее 2 градусов Цельсия на " + rec.field("namestation").value().toString();

                }
                if (m_ups->ext_temp < 10){ //if temperature is low


                    query_log.prepare("INSERT INTO logs (date_time, type, descr ) "
                                      "VALUES ( :date_time, :type, :descr)");
                    query_log.bindValue(":date_time", tmp_time );
                    query_log.bindValue( ":type", 404 );
                    query_log.bindValue(":descr", "Зафиксировано на ИБП понижение температуры менее 10 градусов Цельсия на " + rec.field("namestation").value().toString());
                    query_log.exec();
                    query_log.finish();

                    qDebug() << "Зафиксировано понижение температуры менее 10 градусов Цельсия на " + rec.field("namestation").value().toString();

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

    m_transactTimer->start(rec.field("average_period").value().toInt() *1000);

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




