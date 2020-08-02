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

#ifndef PROCESSOR_H
#define PROCESSOR_H


#include <QTimer>
#include <QMap>
#include <QSqlDatabase>
#include <QMutex>
#include <QUuid>
#include <QVector>


#include "ups_status.h"

class processor : public QObject
{
    Q_OBJECT
public:
    processor(QObject *_parent = 0,  QStringList *cmdline = 0 );
    ~processor();




public:
    static QMap<QString, int>   * ms_data; //assosiative array of polling data
    static QMap<QString, int>   * ms_measure; //assosiative array of measurement quantities
    static QMap<QString, int>   * ms_range; //assosiative array of measurement equipments range

signals:
    void finished ();						// signal finished

public slots:
    void terminate ()			//termination
    {
        emit finished ();
    }


private slots:

    void startTransactTimer(QSqlDatabase *conn); //start by signal dbForm
    void transactionDB   (void);   //transaction timer event
    void readSocketStatus(void);

private:

    QTimer * m_pollTimer;

    QTimer * m_transactTimer; //timer for trunsaction
    QMap<QString, float>   * m_data; //assosiative array of polling data
    QMap<QString, int>   * m_measure; //assosiative array of measurement quantities
    QMap<QString, int>   * m_range; //assosiative array of measurement equipments range

    QMap<QString, QUuid>   * m_uuid; //assosiative array of sensors uuid
    //QList<int> *m_pool;


    QSqlDatabase * m_conn;
    QUuid * m_uuidStation;


    bool verbose; //verbose mode flag

    ups_status *m_ups = nullptr;   //member for UPS status
    QString m_ups_ip;
    quint16 m_ups_port;
    QString m_ups_username;





};

#endif // PROCESSOR_H
