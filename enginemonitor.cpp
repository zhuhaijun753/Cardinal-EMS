//////////////////////////////////////////////////////////////////////////
//                                                                      //
// EngineMonitor, a graphical gauge to monitor an aircraft's engine     //
// Copyright (C) 2012 Tobias Rad                                        //
//                                                                      //
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

#include <QtSvg>

#include "enginemonitor.h"

EngineMonitor::EngineMonitor(QWidget *parent) : QGraphicsView(parent)
  , graphicsScene(this)
  , settings("./settings.ini", QSettings::IniFormat, parent)
{
	//Initializing the window behaviour and it's scene
	setWindowFlags(Qt::FramelessWindowHint);
	graphicsScene.setBackgroundBrush(Qt::black);
	setScene(&graphicsScene);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

	//Setting up the items to be displayed
	setupRpmIndicator();
	setupExhaustGasTemperature();
	setupCylinderHeadTemperature();
	setupBarGraphs();
	setupStatusItem();
	setupTimeToDestinationItem();
	setupFuelManagement();
	graphicsScene.update();
	setupLogFile();

//	//Demo timer, for testing purposes only
//	QTimer *demoTimer = new QTimer(this);
//	connect(demoTimer, SIGNAL(timeout()), this, SLOT(demoFunction()));
//	demoTimer->setSingleShot(false);
//	demoTimer->start(200);
}

EngineMonitor::~EngineMonitor()
{
	logFile->close();
}

void EngineMonitor::setupLogFile()
{
	logFile = new QFile(QString("EngineData ").append(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh.mm.ss")).append(".csv"), this);
	if(logFile->open(QIODevice::WriteOnly))
	{
		QTimer *writeLogFileTimer = new QTimer(this);
		connect(writeLogFileTimer, SIGNAL(timeout()), this, SLOT(writeLogFile()));
		writeLogFileTimer->setSingleShot(false);
		writeLogFileTimer->start(1000);
	}
	else
	{
		userMessageHandler("Unable to open log file", "Unable to open log file, closing application.", true);
	}

	logFile->write("[Header]\n");
	logFile->write("Created with EM-One, Build BETA\n");
	logFile->write(QString("Call Sign: %1\n").arg(settings.value("Aircraft/CALL_SIGN").toString()).toAscii());
	logFile->write(QString("Aircraft Model: %1\n").arg(settings.value("Aircraft/AIRCRAFT_MODEL").toString()).toAscii());
	logFile->write(QString("Aircraft S/N: %1\n").arg(settings.value("Aircraft/AIRCRAFT_S/N").toString()).toAscii());
	logFile->write(QString("Engine Type: %1\n").arg(settings.value("Aircraft/ENGINE_TYPE").toString()).toAscii());
	logFile->write(QString("Engine S/N: %1\n").arg(settings.value("Aircraft/ENGINE_S/N").toString()).toAscii());
	logFile->write("All temperatures in degree Celsius, oil pressure in psi, fuel flow in liters per hour.\n");
	logFile->write("[data]\n");
	logFile->write("INDEX,TIME,EGT1,EGT2,EGT3,EGT4,CHT1,CHT2,CHT3,CHT4,OILT, OILP,OAT,IAT,BAT,CUR,RPM,MAP,FF,MARK\n");
}

void EngineMonitor::writeLogFile()
{
	static quint64 sample = 0;
	logFile->write(QString::number(sample).append(',').toAscii());
	logFile->write(QDateTime::currentDateTimeUtc().toString("yyyy-dd-MM hh:mm:ss").append(',').toAscii());
	QList<double> egtValues = exhaustGasTemperature.getCurrentValues();
	logFile->write(QString::number(egtValues.value(0, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(egtValues.value(1, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(egtValues.value(2, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(egtValues.value(3, 0.0), 'f', 0).append(',').toAscii());
	QList<double> chtValues = cylinderHeadTemperature.getCurrentValues();
	logFile->write(QString::number(chtValues.value(0, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(chtValues.value(1, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(chtValues.value(2, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(chtValues.value(3, 0.0), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(oilTemperature.getValue(), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(oilPressure.getValue(), 'f', 0).append(',').toAscii());
	logFile->write(QString::number(outsideAirTemperature.getValue(), 'f', 0).append(',').toAscii());
	logFile->write(QString("IAT/ITEMP").append(',').toAscii());
	logFile->write(QString::number(voltMeter.getValue(), 'f', 1).append(',').toAscii());
	logFile->write(QString("CUR/CHT2").append(',').toAscii());
	logFile->write(QString::number(rpmIndicator.getValue(), 'f', 0).append(',').toAscii());
	logFile->write(QString("MAP/MAP").append(',').toAscii());
	logFile->write(QString::number(fuelFlow.getValue(), 'f', 1).append(',').toAscii());
	logFile->write(QString(exhaustGasTemperature.isLeanAssistActive() ? "1" : "0").append('\n').toAscii());
	logFile->flush();
	++sample;
}

void EngineMonitor::setupRpmIndicator()
{
	rpmIndicator.setPos(-350, -100);
	rpmIndicator.setStartSpan(230.0, 240.0);
	rpmIndicator.setBorders(0.0, 2800.0, 300.0, 2550.0);
	rpmIndicator.addBetweenValue(0.0);
	rpmIndicator.addBetweenValue(300.0);
	rpmIndicator.addBetweenValue(600.0);
	rpmIndicator.addBetweenValue(900.0);
	rpmIndicator.addBetweenValue(1200.0);
	rpmIndicator.addBetweenValue(1500.0);
	rpmIndicator.addBetweenValue(1800.0);
	rpmIndicator.addBetweenValue(2100.0);
	rpmIndicator.addBetweenValue(2400.0);
	rpmIndicator.addBetweenValue(2700.0);
	graphicsScene.addItem(&rpmIndicator);
}

void EngineMonitor::setupExhaustGasTemperature()
{
	exhaustGasTemperature.setPos(-475, 175);
	exhaustGasTemperature.setBorders(300.0, 850.0, 750.0, 800.0);
	exhaustGasTemperature.addBetweenValue(450);
	exhaustGasTemperature.addBetweenValue(550);
	exhaustGasTemperature.addBetweenValue(650);
	exhaustGasTemperature.addBetweenValue(750);
	exhaustGasTemperature.addBetweenValue(800);
	exhaustGasTemperature.addBetweenValue(850);
	exhaustGasTemperature.setLeanWindow(200.0);
	graphicsScene.addItem(&exhaustGasTemperature);
}

void EngineMonitor::setupCylinderHeadTemperature()
{
	cylinderHeadTemperature.setPos(-200, 175);
	cylinderHeadTemperature.setBorders(60.0, 160.0, 140.0, 150.0);
	cylinderHeadTemperature.addBetweenValue(80);
	cylinderHeadTemperature.addBetweenValue(100);
	cylinderHeadTemperature.addBetweenValue(120);
	cylinderHeadTemperature.addBetweenValue(140);
	cylinderHeadTemperature.addBetweenValue(150);
	cylinderHeadTemperature.addBetweenValue(160);
	graphicsScene.addItem(&cylinderHeadTemperature);
}

void EngineMonitor::setupBarGraphs()
{
	oilTemperature.setPos(0, -150);
	oilTemperature.setTitle("OIL T");
	oilTemperature.setUnit(QString::fromUtf8("°C"));
	oilTemperature.setBorders(80.0, 180.0);
	oilTemperature.addColorStop(ColorStop(Qt::green, 80.0, 160.0));
	oilTemperature.addColorStop(ColorStop(Qt::yellow, 160.0, 180.0));
	graphicsScene.addItem(&oilTemperature);

	oilPressure.setPos(100, -150);
	oilPressure.setTitle("OIL P");
	oilPressure.setUnit("psi");
	oilPressure.setBorders(0.0, 60.0);
	oilPressure.addColorStop(ColorStop(Qt::red, 0.0, 15.0));
	oilPressure.addColorStop(ColorStop(Qt::yellow, 55.0, 60.0));
	oilPressure.addBetweenValue(15.0);
	oilPressure.addBetweenValue(35.0);
	oilPressure.addBetweenValue(50.0);
	graphicsScene.addItem(&oilPressure);

	voltMeter.setPos(0, 0);
	voltMeter.setTitle("VOLTS");
	voltMeter.setUnit("V");
	voltMeter.setBorders(11.5, 18.0);
	voltMeter.addColorStop(ColorStop(Qt::yellow, 11.5, 13.0));
	voltMeter.setPrecision(1, 1);
	graphicsScene.addItem(&voltMeter);

	ampereMeter.setPos(100, 0);
	ampereMeter.setTitle("AMPS");
	ampereMeter.setUnit("A");
	ampereMeter.setBorders(-50.0, 50.0);
	ampereMeter.addColorStop(ColorStop(Qt::red, -50.0, 0.0));
	ampereMeter.addColorStop(ColorStop(Qt::yellow, 30.0, 50.0));
	ampereMeter.addBetweenValue(0.0);
	graphicsScene.addItem(&ampereMeter);

	fuelFlow.setPos(0, 150);
	fuelFlow.setTitle("FF");
	fuelFlow.setUnit("lph");
	fuelFlow.setBorders(0, 30);
	fuelFlow.addBetweenValue(10.0);
	fuelFlow.setPrecision(1);
	fuelFlow.addBetweenValue(20.0);
	graphicsScene.addItem(&fuelFlow);

	insideAirTemperature.setPos(100, 150);
	insideAirTemperature.setTitle("IAT");
	insideAirTemperature.setUnit(QString::fromUtf8("°C"));
	insideAirTemperature.setBorders(-10.0, 40);
	insideAirTemperature.addBetweenValue(0.0);
	insideAirTemperature.addBetweenValue(10.0);
	insideAirTemperature.addBetweenValue(20.0);
	insideAirTemperature.addBetweenValue(30.0);
	graphicsScene.addItem(&insideAirTemperature);
	insideAirTemperature.setVisible(false);
	connect(&outsideAirTemperature, SIGNAL(hasBeenClicked()), &outsideAirTemperature, SLOT(makeInvisible()));
	connect(&outsideAirTemperature, SIGNAL(hasBeenClicked()), &insideAirTemperature, SLOT(makeVisible()));

	outsideAirTemperature.setPos(100, 150);
	outsideAirTemperature.setTitle("OAT");
	outsideAirTemperature.setUnit(QString::fromUtf8("°C"));
	outsideAirTemperature.setBorders(-10.0, 40);
	outsideAirTemperature.addBetweenValue(0.0);
	outsideAirTemperature.addBetweenValue(10.0);
	outsideAirTemperature.addBetweenValue(20.0);
	outsideAirTemperature.addBetweenValue(30.0);
	graphicsScene.addItem(&outsideAirTemperature);
	connect(&insideAirTemperature, SIGNAL(hasBeenClicked()), &insideAirTemperature, SLOT(makeInvisible()));
	connect(&insideAirTemperature, SIGNAL(hasBeenClicked()), &outsideAirTemperature, SLOT(makeVisible()));
}

void EngineMonitor::setupStatusItem()
{
	statusItem.setPos(-200, -240);
	graphicsScene.addItem(&statusItem);
}

void EngineMonitor::setupTimeToDestinationItem()
{
	timeToDestinationItem.setPos(-650, -240);
	timeToDestinationItem.setDefaultTextColor(Qt::white);
	graphicsScene.addItem(&timeToDestinationItem);
}

void EngineMonitor::setupFuelManagement()
{
	//fuelManagement.setPos(-70, 70);
	fuelManagement.setPos(-525, -240);
	fuelManagement.scale(1.8, 1.8);
	fuelManagement.setVisible(false);
	connect(&fuelFlow, SIGNAL(hasBeenClicked()), &fuelManagement, SLOT(activateOverlay()));
	graphicsScene.addItem(&fuelManagement);
}

void EngineMonitor::setDataMessage1(double fuelFlowValue)
{
	fuelFlow.setValue(fuelFlowValue);
	fuelManagement.setFuelFlow(fuelFlowValue);
}

void EngineMonitor::setDataMessage2(double outsideAirTemperatureValue, double ampereValue, double oilTemperatureValue, double oilPressureValue, double voltageValue)
{
	outsideAirTemperature.setValue(outsideAirTemperatureValue);
	ampereMeter.setValue(ampereValue);
	oilTemperature.setValue(oilTemperatureValue);
	oilPressure.setValue(oilPressureValue);
	voltMeter.setValue(voltageValue);
}

void EngineMonitor::setDataMessage3(double revolutionPerMinute)
{
	rpmIndicator.setValue(revolutionPerMinute);
}

void EngineMonitor::setDataMessage4egt(quint16 egt1, quint16 egt2, quint16 egt3, quint16 egt4)
{
	exhaustGasTemperature.setValues(egt1, egt2, egt3, egt4);
}

void EngineMonitor::setDataMessage4cht(quint16 cht1, quint16 cht2, quint16 cht3, quint16 cht4)
{
	cylinderHeadTemperature.setValues(cht1, cht2, cht3, cht4);
}

void EngineMonitor::setTimeToDestination(double time)
{
	timeToDestinationItem.setPlainText(QString::number(time, 'f', 1).prepend("Time to destination: ").append(" minutes"));
	fuelManagement.setTimeToDestination(time);
}

void EngineMonitor::userMessageHandler(QString title, QString content, bool endApplication)
{
	QMessageBox::warning(this, title, content);
	if(endApplication)
	{
		qApp->quit();
	}
}

void EngineMonitor::showStatusMessage(QString text, QColor color)
{
	statusItem.setPlainText(text);
	statusItem.setDefaultTextColor(color);
}

void EngineMonitor::demoFunction()
{
	qsrand(QDateTime::currentDateTime().toTime_t());
	static double rpm = 0.0;
	rpm += 5.0;
	if(rpm > 2800.0)
	{
		saveSceneToSvg("./out/maxRPM.svg");
		rpm = 0.0;
	}
	rpmIndicator.setValue(rpm);

	static double basicEGT = 0.0;
	static bool egtUp = true;
	static bool leaned = false;
	static double off13 = 0.0;
	static double off24 = 0.0;
	if(leaned)
	{
		if(basicEGT < 1190.0)
		{
			off13 = double(qrand())/double(RAND_MAX)*3.0;
			off24 = double(qrand())/double(RAND_MAX)*5.0;
		}
	}
	else if(egtUp)
	{
		basicEGT += 1.0;
	}
	else
	{
		basicEGT -= 1.0;
	}
	if(basicEGT > 1200.0 && egtUp)
	{
		egtUp = false;
	}
	if(basicEGT < 1150.0 && !egtUp)
	{
		leaned = true;
		egtUp = true;
	}
	exhaustGasTemperature.setValues(basicEGT+20.0+off13, basicEGT+10.0-off24, basicEGT+5.0-off13, basicEGT+30.0+off24);

	static double basicCHT = 0.0;
	basicCHT += 1.0;
	if(basicCHT > 250.0)
	{
		basicCHT = 0.0;
	}
	static double offset1 = double(qrand())/double(RAND_MAX)*25.0;
	static double offset2 = double(qrand())/double(RAND_MAX)*7.0;
	static double offset3 = double(qrand())/double(RAND_MAX)*15.0;
	static double offset4 = double(qrand())/double(RAND_MAX)*9.0;
	cylinderHeadTemperature.setValues(basicCHT+offset1,
									  basicCHT+offset2,
									  basicCHT+offset3,
									  basicCHT+offset4);

	static double oilTemp = 160.0;
	if(oilTemp < 120.0)
	{
		oilTemp = 160.0;
	}
	oilTemp -= 0.1;
	oilTemperature.setValue(oilTemp);

	static double oilPress = 0.0;
	oilPress += 0.05;
	if(oilPress > 60.0)
	{
		oilPress = 0.0;
	}
	oilPressure.setValue(oilPress);

	static double volts = 11.5;
	volts += 0.01;
	if(volts > 18.0)
	{
		volts = 11.5;
	}
	voltMeter.setValue(volts);

	static double amperes = 50.0;
	amperes -= 0.1;
	if(amperes < -50.0)
	{
		amperes = 50.0;
	}
	ampereMeter.setValue(amperes);

	static double flow = 30.0;
	flow -= 0.05;
	if(flow < 0.0)
	{
		flow = 30.0;
	}
	fuelFlow.setValue(flow);

	static double airTemp = -10.0;
	airTemp += 0.01;
	if(airTemp > 40.0)
	{
		airTemp = -10.0;
	}
	outsideAirTemperature.setValue(airTemp);
}

void EngineMonitor::saveSceneToSvg(const QString fileName)
{
	QSvgGenerator generator;
	generator.setFileName(fileName);
	generator.setSize(QSize(800, 600));
	generator.setViewBox(QRect(0, 0, 800, 600));
	generator.setTitle(tr("SVG Generator Example Drawing"));
	generator.setDescription(tr("An SVG drawing created by the SVG Generator"));
	QPainter painter;
	painter.begin(&generator);
	graphicsScene.render(&painter);
	painter.end();
}
