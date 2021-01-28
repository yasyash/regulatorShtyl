/*
 * Copyright © 2018-2019 Yaroslav Shkliar <mail@ilit.ru>
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

#include "meteotcpsock.h"

#include <QDebug>

#ifdef METEOTCPSOCK_H
MeteoTcpSock::MeteoTcpSock(QObject *parent , QString *ip, quint16 *port) : QObject (parent)

{


    m_sock = new QTcpSocket(this);

    connect(m_sock, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(m_sock, SIGNAL(bytesWritten(qint64)), this, SLOT(writes()));

    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    //connect(this, SIGNAL(dataReady(QByteArray&)), this, SLOT(setData(QByteArray&)) );

    changeInterface(*ip, *port);
    m_sock->setSocketOption(QAbstractSocket::LowDelayOption, 0);
    //qDebug() << "Socket " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);
    // m_sock->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024);
    // qDebug() << "Socket next " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);

    m_sock->setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);

    measure = new  QMap<QString, float>;
    measure_prev = new  QMap<QString, float>;

    measure_prev->insert("bar", 0.0f);
    measure_prev->insert("temp_in", 0.0f);
    measure_prev->insert("hum_in", 0.0f);
    measure_prev->insert("temp_out", 0.0f);
    measure_prev->insert("speed_wind", 0.0f);
    measure_prev->insert("dir_wind", 0.0f);
    measure_prev->insert("dew_pt", 0.0f);
    measure_prev->insert("hum_out", 0.0f);
    measure_prev->insert("heat_indx", 0.0f);
    measure_prev->insert("chill_wind", 0.0f);
    measure_prev->insert("thsw_indx", 0.0f);
    measure_prev->insert("rain_rate", 0.0f);
    measure_prev->insert("uv_indx", 0.0f);
    measure_prev->insert("rad_solar", 0.0f);
    //measure->insert("rain_daily", 0);
    measure_prev->insert("rain", 0.0f); //mm per hour
    measure_prev->insert("et", 0.0f); //evaporotransportation in mm per day
    //measure->insert("batt_remote", 0);

    is_read = false;
    status = "";
    sample_t = 0;

    connected = m_sock->state();


    qDebug() << "Meteostation handling has been initialized.\n\r";

}

MeteoTcpSock::MeteoTcpSock(QObject *parent , QString *ip, quint16 *port, float _in, float _out) : QObject (parent)

{


    m_sock = new QTcpSocket(this);

    connect(m_sock, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(m_sock, SIGNAL(bytesWritten(qint64)), this, SLOT(writes()));

    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    //connect(this, SIGNAL(dataReady(QByteArray&)), this, SLOT(setData(QByteArray&)) );

    changeInterface(*ip, *port);
    m_sock->setSocketOption(QAbstractSocket::LowDelayOption, 0);
    //qDebug() << "Socket " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);
    // m_sock->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024);
    // qDebug() << "Socket next " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);

    m_sock->setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);

    measure = new  QMap<QString, float>;
    measure_prev = new  QMap<QString, float>;

    measure_prev->insert("bar", 0.0f);
    measure_prev->insert("temp_in", _in);
    measure_prev->insert("hum_in", 0.0f);
    measure_prev->insert("temp_out", _out);
    measure_prev->insert("speed_wind", 0.0f);
    measure_prev->insert("dir_wind", 0.0f);
    measure_prev->insert("dew_pt", 0.0f);
    measure_prev->insert("hum_out", 0.0f);
    measure_prev->insert("heat_indx", 0.0f);
    measure_prev->insert("chill_wind", 0.0f);
    measure_prev->insert("thsw_indx", 0.0f);
    measure_prev->insert("rain_rate", 0.0f);
    measure_prev->insert("uv_indx", 0.0f);
    measure_prev->insert("rad_solar", 0.0f);
    //measure->insert("rain_daily", 0);
    measure_prev->insert("rain", 0.0f); //mm per hour
    measure_prev->insert("et", 0.0f); //evaporotransportation in mm per day
    //measure->insert("batt_remote", 0);

    is_read = false;
    status = "";
    sample_t = 0;

    connected = m_sock->state();


    qDebug() << "Meteostation handling has been initialized.\n\r";

}

MeteoTcpSock::MeteoTcpSock(QObject *parent, QString *ip, quint16 *port, QString *_model): QObject (parent)
{
    m_sock = new QTcpSocket(this);

    connect(m_sock, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(m_sock, SIGNAL(bytesWritten(qint64)), this, SLOT(writes()));

    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    //connect(this, SIGNAL(dataReady(QByteArray&)), this, SLOT(setData(QByteArray&)) );

    changeInterface(*ip, *port);
    m_sock->setSocketOption(QAbstractSocket::LowDelayOption, 0);
    //qDebug() << "Socket " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);
    // m_sock->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024);
    // qDebug() << "Socket next " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);

    m_sock->setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);

    measure = new  QMap<QString, float>;
    measure_prev = new  QMap<QString, float>;

    measure_prev->insert("bar", 0.0f);
    measure_prev->insert("temp_in", 0.0f);
    measure_prev->insert("hum_in", 0.0f);
    measure_prev->insert("temp_out", 0.0f);
    measure_prev->insert("speed_wind", 0.0f);
    measure_prev->insert("dir_wind", 0.0f);
    measure_prev->insert("dew_pt", 0.0f);
    measure_prev->insert("hum_out", 0.0f);
    measure_prev->insert("heat_indx", 0.0f);
    measure_prev->insert("chill_wind", 0.0f);
    measure_prev->insert("thsw_indx", 0.0f);
    measure_prev->insert("rain_rate", 0.0f);
    measure_prev->insert("uv_indx", 0.0f);
    measure_prev->insert("rad_solar", 0.0f);
    //measure->insert("rain_daily", 0);
    measure_prev->insert("rain", 0.0f); //mm per hour
    measure_prev->insert("et", 0.0f); //evaporotransportation in mm per day
    //measure->insert("batt_remote", 0);

    is_read = false;
    status = "";
    sample_t = 0;
    model = new QString (*_model);

    connected = m_sock->state();


    qDebug() << "Meteostation model"<< *_model << " handling has been initialized.\n\r";
}

MeteoTcpSock::MeteoTcpSock(QObject *parent , QString *ip, quint16 *port, float _in, float _out, QString *_model) : QObject (parent)

{


    m_sock = new QTcpSocket(this);

    connect(m_sock, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(m_sock, SIGNAL(bytesWritten(qint64)), this, SLOT(writes()));

    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    //connect(this, SIGNAL(dataReady(QByteArray&)), this, SLOT(setData(QByteArray&)) );

    changeInterface(*ip, *port);
    m_sock->setSocketOption(QAbstractSocket::LowDelayOption, 0);
    //qDebug() << "Socket " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);
    // m_sock->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024);
    // qDebug() << "Socket next " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);

    m_sock->setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);

    measure = new  QMap<QString, float>;
    measure_prev = new  QMap<QString, float>;

    measure_prev->insert("bar", 0.0f);
    measure_prev->insert("temp_in", _in);
    measure_prev->insert("hum_in", 0.0f);
    measure_prev->insert("temp_out", _out);
    measure_prev->insert("speed_wind", 0.0f);
    measure_prev->insert("dir_wind", 0.0f);
    measure_prev->insert("dew_pt", 0.0f);
    measure_prev->insert("hum_out", 0.0f);
    measure_prev->insert("heat_indx", 0.0f);
    measure_prev->insert("chill_wind", 0.0f);
    measure_prev->insert("thsw_indx", 0.0f);
    measure_prev->insert("rain_rate", 0.0f);
    measure_prev->insert("uv_indx", 0.0f);
    measure_prev->insert("rad_solar", 0.0f);
    //measure->insert("rain_daily", 0);
    measure_prev->insert("rain", 0.0f); //mm per hour
    measure_prev->insert("et", 0.0f); //evaporotransportation in mm per day
    //measure->insert("batt_remote", 0);

    is_read = false;
    status = "";
    sample_t = 0;
    model = new QString (*_model);

    connected = m_sock->state();


    qDebug() << "Meteostation model"<< *_model << " handling has been initialized.\n\r";

}

MeteoTcpSock::~MeteoTcpSock()
{
    m_sock->close();
    m_sock->disconnectFromHost();
}



void MeteoTcpSock::changeInterface(const QString &address, quint16 portNbr)
{
    m_sock->connectToHost(address, portNbr);
}





void MeteoTcpSock::on_cbEnabled_clicked(bool checked)
{
    if (checked)
    {
    }
    else {
        m_sock->disconnectFromHost();
    }
    //emit tcpPortActive(checked);
}


void MeteoTcpSock::readData()
{

    QStringList list;
    int ind;
    int running;
    QRegExp xRegExp("(-?\\d+(?:[\\.,]\\d+(?:e\\d+)?)?)");
    float _result;
    bool _absend = false;

    QByteArray data = m_sock->readAll();

    //qDebug() << "Meteostation sent data: " << data << " lenght - " << data.length() << " \n\r";

    this->is_read = false;

    //emit (dataReady(data));


    blockSize = 0;
    if (model)
    {
        //for the model
        //QString _tmp = QString::fromStdString(data.toStdString());
        list = QString::fromStdString(data.toStdString()).split(';');

        //QStringList _list_smcln = list.filter("+");

        QStringListIterator _strli(list);
        QStringList _data;
        QString _tmp;
        int _ind;
        float _result;
        int wrong = 0;

        if (list.length() > 1){
            while (_strli.hasNext())
            {

                _tmp = QString::fromStdString(_strli.next().toStdString());
                _ind  =   _tmp.indexOf("Ta");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("temp_out", measure->value("temp_out") + _result);
                    wrong++;
                }
                _ind  =   _tmp.indexOf("Tp");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("dew_pt", measure->value("dew_pt") + _result);
                    wrong++;

                }

                _ind  =   _tmp.indexOf("Tw");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("chill_wind", measure->value("chill_wind") + _result);
                    wrong++;

                }

                _ind  =   _tmp.indexOf("Hr");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("hum_out", measure->value("hum_out") + _result);
                    wrong++;

                }
                _ind  =   _tmp.indexOf("Pa");

                if (_ind != -1){
                    _result = (_tmp.mid(2,_tmp.length()-3).toFloat())/100*75; //hPa to mmHg
                    measure->insert("bar", measure->value("bar") + _result);
                    wrong++;

                }

                _ind  =   _tmp.indexOf("Sa");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("speed_wind", measure->value("speed_wind") + _result);
                    wrong++;

                }

                _ind  =   _tmp.indexOf("Da");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("dir_wind", measure->value("dir_wind") + _result);
                    wrong++;

                }

                _ind  =   _tmp.indexOf("Ra");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("rain_rate", measure->value("rain_rate") + _result);
                    wrong++;

                }
                _ind  =   _tmp.indexOf("Ri");

                if (_ind != -1){
                    _result = _tmp.mid(2,_tmp.length()-3).toFloat();
                    measure->insert("rain", measure->value("rain") + _result);
                    wrong++;

                }


            }
            if (wrong == 9)
                sample_t++;


        }
        qDebug() << "Meteostation's data parsed \n\r" ;



    }
    else {
        //for DV VP

        if ((uchar(data[1])==0x4c)) //write bytes detection
        {


            if ((uchar(data[11]) > 0x04))
            {

                int _sign = (int(data[11]) > 0 ? 1 : -1);
                int _raw = (_sign * ((255 - uchar (data [11]))<<8)  - 255 + uchar(data[10]));
                _result = ((float)(_raw)/10-32)*5/9;

            }
            else {
                _result =  ((float)((uchar(data[11])<<8) + (uchar(data[10])))/10-32)*5/9; //Fahrenheit TO Celsius Conversion Formula
            }

            if (((uchar(data[11])==0) && uchar(data[10]) == 0 && first_run) || ((uchar(data[11])==0x7f)  && first_run))//|| ((uchar(data[11])==0xff)  && first_run))
            {
                // measure_prev->insert("temp_in",23.0f);

                if (_result < 60) {
                    measure->insert("temp_in",  _result);}
                else
                {
                    measure->insert("temp_in",  measure_prev->value("temp_in"));
                }
            }
            else
            {
                _result = compare (_result, measure_prev->value("temp_in"), 0.05f);
                if (_result < 0.0f)
                    _result = measure_prev->value("temp_in");
                if (_result > 60.0f)
                    _result = measure_prev->value("temp_in");

                measure_prev->insert("temp_in",_result);

                measure->insert("temp_in",  _result);
            }
            qDebug() << "Meteostation's temperature is " << _result;
            sample_t++;

            if (_absend) //if temperature outside is absend
                first_run = true;
            else {
                first_run =  false;
            }

        }
    }
}
void MeteoTcpSock::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug()<<   ("Meteostation handling error: The host was not found. Please check the "
                      "host name and port settings.\n\r");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug()<< ("Meteostation handling error: The connection was refused by the peer. "
                    "Make sure the fortune server is running, "
                    "and check that the host name and port "
                    "settings are correct.\n\r");
        break;
    default:
        qDebug()<< ("Meteostation handling error: ") << (m_sock->errorString())<<"\n\r";
    }
    if (m_sock->isOpen())
        m_sock->close();
    connected = m_sock->state();


}

void MeteoTcpSock::sendData( char *data)
{
    if (model)
    {
        char *str = (char*)(malloc(strlen(data) * sizeof(char) + 1));
        *str = '\0';
        strcat(str, data);
        strcat(str,  "\r");
        qint64 lnt = qint64(strlen(str));

        lnt = m_sock->write(str, lnt);
        // lnt = m_sock->flush();
        qDebug()<< "Meteostation "<< *model <<" command: " << data <<"\n\r";

    }
    else {

        char *str = (char*)(malloc(strlen(data) * sizeof(char) + 1));
        *str = '\0';
        strcat(str, data);
        strcat(str,  "\n");
        qint64 lnt = qint64(strlen(str));

        lnt = m_sock->write(str, lnt);
        // lnt = m_sock->flush();
        qDebug()<< "Meteostation command: " << data <<"\n\r";
    }

}

void MeteoTcpSock::writes()
{
    qDebug()<< "written " ;
}

float MeteoTcpSock::compare(float _in, float _prev)
{
    if (!first_run ){
        if (std::abs(_prev - _in) < std::abs(_prev*0.15f)) //new value don't exceed of 15% per sample
        {
            return _in;
        } else {
            return  _prev;
        }
    } else {

        return  _in;
    }
}

float MeteoTcpSock::compare(float _in, float _prev, float coeff)
{
    if (!first_run ){
        if (std::abs(_prev - _in) < std::abs(_prev * coeff)) //new value don't exceed of 15% per sample
        {
            return _in;
        } else {
            return  _prev;
        }
    } else {

        return  _in;
    }
}
#endif
