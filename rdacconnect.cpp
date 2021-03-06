//////////////////////////////////////////////////////////////////////////
//                                                                      //
// EngineMonitor, a graphical gauge to monitor an aircraft's engine     //
// Copyright (C) 2012 Tobias Rad                                        //
//                2017 Ryan Story                                       //
// This program is free software: you can redistribute it and/or modify //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation, either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// This program is distributed in the hope that it will be useful,      //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with this program. If not, see <http://www.gnu.org/licenses/>. //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "rdacconnect.h"

RDACmessage1::RDACmessage1() : flow1(0)
  , pulseRatio1(0)
  , flow2(0)
  , pulseRatio2(0)
  , oilTemp(0)
  , oilPress(0)
  , aux1(0)
  , aux2(0)
  , fuelPress(0)
  , coolant(0)
  , fuelLevel1(0)
  , fuelLevel2(0)
  , rpm1(0)
  , rpm2(0)
  , internalTemp(0)
  , volts(0)
{
}

RDACmessage2::RDACmessage2() : oilTemperature(0)
  , oilPressure(0)
  , fuelLevel1(0)
  , fuelLevel2(0)
  , voltage(0)
  , internalTemperature(0)
  , cht1(0)
  , cht2(0)
  , manifoldPressure(0)
{
}

RDACmessage3::RDACmessage3() : timeBetweenPulses(0)
{
}

RDACmessage4::RDACmessage4()
{
}

RDACconnect::RDACconnect(QObject *parent) : QObject(parent)
  , settings("./settings.ini", QSettings::IniFormat, parent)
{
    serial = new QSerialPort(this);

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,
                SLOT(handleError(QSerialPort::SerialPortError)));


    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
}

void RDACconnect::readData()
{
	bool startPatternFound = false;

    data.append(serial->read(1));

//            emit userMessage("RDAC COM error", "Error reading data, closing application", true);
//            exec();
//        }

    if(searchStart(&data))
    {
        startPatternFound = true;
        quint8 messageType = 0x00;
        switch(checkPatternValidity(&data, messageType))
        {
        case rdacResultMessageComplete:
            emit statusMessage("Everything OK - Last update: " + lastMessageReception.value(3).toString("hh:mm:ss.zzz"), Qt::white);
            switch(messageType)
            {
                case 0x01:
                    handleMessage1(&data);
                    break;
                case 0x02:
                    handleMessage2(&data);
                    break;
                case 0x03:
                    handleMessage3(&data);
                    break;
                case 0x04:
                    handleMessage4(&data);
                    break;
                default:
                    data.remove(0, 1);
                    break;
            }
            break;
        case rdacResultMessageIncomplete:
            break;
        default:
            emit statusMessage("Found pattern not valid", Qt::yellow);
            break;
        }
    }
    if(!startPatternFound)
    {
        emit statusMessage("No start pattern found yet", Qt::red);
    }
}

quint8 RDACconnect::calculateChecksum1(QByteArray data)
{
    quint8 checksum = 0x55;
    qDebug() << checksum;
    for(int i = 2; i < data.size()-2; ++i)
	{
        checksum += quint8(data.at(i));
	}
	return checksum;
}

quint8 RDACconnect::calculateChecksum2(QByteArray data)
{
    quint8 checksum = 0xAA;
    for(int i = 2; i < data.size()-2; ++i)
	{
        checksum += quint8(data.at(i));
	}
	return checksum;
}

bool RDACconnect::searchStart(QByteArray *data)
{
	while(data->size() >= 3)
    {
        if(data->at(0) == 0x05)
        {
            if(data->at(1) == 0x02)
			{
                if((data->at(2) == 0x01))
                {
                    //numTries += 1;
					return true;
				}
			}
		}
		data->remove(0, 1);
	}
	return false;
}

RDACconnect::rdacResults RDACconnect::checkPatternValidity(QByteArray *data, quint8 &messageType)
{
	// Determine and check neccessary size of data
	quint8 requiredSize = 0;
    messageType = data->at(2);
	switch(messageType)
	{
		case 0x01:
            requiredSize = 66;
			break;
		case 0x02:
			requiredSize = 23;
			break;
		case 0x03:
			requiredSize = 7;
			break;
		case 0x04:
			requiredSize = 29;
			break;
		default:
			data->remove(0, 1);
			return rdacResultMessageIllegalDatatype;
	}
	if(data->size() < requiredSize)
	{
		return rdacResultMessageIncomplete;
	}

	// Calculate and check checksums
    if(quint8(data->at(requiredSize - 2)) == calculateChecksum1(data->mid(0, requiredSize)))
	{
        if(quint8(data->at(requiredSize - 1)) == calculateChecksum2(data->mid(0, requiredSize)))
		{
            //numSuccess += 1;
            //qDebug() << "Sucesses:" << numSuccess << "; Tries:" << numTries << "; " << numSuccess/numTries << "%";
			return rdacResultMessageComplete;
		}
		else
		{
			qWarning() << "Checksum 2 incorrect" << quint8(data->at(requiredSize - 1));
			data->remove(0, 1);
			return rdacResultMessageInvalidChecksum2;
		}
	}
	else
	{
		qWarning() << "Checksum 1 incorrect" << quint8(data->at(requiredSize - 2));
		data->remove(0, 1);
		return rdacResultMessageInvalidChecksum1;
	}
}

void RDACconnect::handleMessage1(QByteArray *data)
{
	lastMessageReception.insert(1, QDateTime::currentDateTimeUtc());
//    QFile file("/home/rstory/datapacket.log");
//    file.open(QIODevice::WriteOnly);
//    file.write(data->toHex());
//    file.close();
	RDACmessage1 message;
    memcpy(&message, data->mid(4, 62).constData(), 62);

    if (message.pulseRatio1 == 65535) {
        message.pulseRatio1 = 0;
    }

    if (message.pulseRatio2 == 65535) {
        message.pulseRatio2 = 0;
    }

    //qDebug() << Q_FUNC_INFO << "Pulses" << message.flow1;

    volts = round(message.volts/5.73758)*0.1;

    lastMessage1 = QDateTime::currentDateTimeUtc();

    qreal oilPressVolt = message.oilPress / (4096/5);

    qreal fuelflow = (message.flow1 / 4) * 60.0 * 60.0; // This converts the pulse data from the RDAC (# of pulses per 4 second period) into pulses/hour
    emit rdacUpdateMessage(fuelflow, volts);
}

void RDACconnect::handleMessage2(QByteArray *data)
{
	lastMessageReception.insert(2, QDateTime::currentDateTimeUtc());
	RDACmessage2 message;
	memcpy(&message, data->mid(3, 18).constData(), 18);
	data->remove(0, 23);

	double voltage = (static_cast<double>(message.voltage) + 115.0) * 0.0069693802;
	double oilPressure = 0.3320318366 * message.oilPressure - 31.2628022226;
	if(oilPressure < 0.0)
	{
		oilPressure = 0.0;
	}

	qreal insideAirTemperature = qreal(message.internalTemperature) / 100.0;
	qreal outsideAirTemperature = qreal(message.cht1) / 100.0;

	emit updateDataMessage2(insideAirTemperature, outsideAirTemperature, message.cht2, message.oilTemperature, oilPressure, voltage, message.manifoldPressure);
}

void RDACconnect::handleMessage3(QByteArray *data)
{
	lastMessageReception.insert(3, QDateTime::currentDateTimeUtc());
	RDACmessage3 message;
	memcpy(&message, data->mid(3, 2).constData(), 2);
	data->remove(0, 7);

	double revFudge = (6000.0 / 19.6) * 15586.0;
	double rpm = revFudge / message.timeBetweenPulses;
	if(message.timeBetweenPulses > 30000)
	{
		rpm = 0.0;
	}

	emit updateDataMessage3(rpm);
	qDebug() << Q_FUNC_INFO << rpm;
}

void RDACconnect::handleMessage4(QByteArray *data)
{
	lastMessageReception.insert(4, QDateTime::currentDateTimeUtc());
	RDACmessage4 message;
	memcpy(&message, data->mid(3, 24).constData(), 24);

	data->remove(0, 29);

	emit updateDataMessage4egt(message.thermocouple[0], message.thermocouple[1], message.thermocouple[2], message.thermocouple[3]);
	emit updateDataMessage4cht(message.thermocouple[4], message.thermocouple[5], message.thermocouple[6], message.thermocouple[7]);
}

void RDACconnect::openSerialPort()
{
    QSerialPortInfo portToUse;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QString s = QObject::tr("Port:") + info.portName() + "\n"
                    + QObject::tr("Location:") + info.systemLocation() + "\n"
                    + QObject::tr("Description:") + info.description() + "\n"
                    + QObject::tr("Manufacturer:") + info.manufacturer() + "\n"
                    + QObject::tr("Serial number:") + info.serialNumber() + "\n"
                    + QObject::tr("Vendor Identifier:") + (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString()) + "\n"
                    + QObject::tr("Product Identifier:") + (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString()) + "\n"
                    + QObject::tr("Busy:") + (info.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) + "\n";

        if(!info.isBusy() && (info.description().contains("Arduino") || info.manufacturer().contains("Arduino")))
            portToUse = info;
        qDebug() << s;
    }

    if(portToUse.isNull() || !portToUse.isValid())
    {
        qDebug() << "port is not valid:" << portToUse.portName();
        return;
    }

    // Enumerate the serial port
    // Find one that sounds like Arduino, or the highest one on the list
    // Open it if it isn't busy

    serial->setPortName(portToUse.portName());
    serial->setBaudRate(QSerialPort::Baud38400);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to" << portToUse.description() << "on" << portToUse.portName();
    } else {
        qCritical() << "Serial Port error:" << serial->errorString();

        qDebug() << tr("Open error");
    }
}



void RDACconnect::closeSerialPort()
{
    serial->close();
    qDebug() << tr("Disconnected");
}

void RDACconnect::writeData(const QByteArray &data)
{
    serial->write(data);
}

void RDACconnect::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        qCritical() << "Serial Port error:" << serial->errorString();
        closeSerialPort();
    }
}
