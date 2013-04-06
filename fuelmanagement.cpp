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

#include "fuelmanagement.h"

FuelManagement::FuelManagement(QGraphicsObject *parent)
	: QGraphicsObject(parent)
	, settings("./settings.ini", QSettings::IniFormat)
	, fuelAmount(0.0)
	, fuelFlow(0.0)
	, timeToDestination(0.0)
	, currentMode(fuelModeManagement)
	, remainingFuelRect(0, 2, 210, 36)
	, remainingFuelAtDestinationRect(0, 42, 210, 36)
	, fuelFlowRect(0, 82, 210, 36)
	, fuelingRect(0, 122, 100, 36)
	, homeRect(110, 122, 100, 36)
	, addLitersTextRect(0, 42, 50, 36)
	, add50LitersRect(54, 42, 36, 36)
	, add10LitersRect(94, 42, 36, 36)
	, add5LitersRect(134, 42, 36, 36)
	, add1LitersRect(174, 42, 36, 36)
	, clearRect(0, 82, 100, 36)
	, fuelTopRect(110, 82, 100, 36)
{
	connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(saveFuelState()));
	fuelAmount = settings.value("Fueling/LastShutdown", 0.0).toDouble();
}

QRectF FuelManagement::boundingRect() const
{
	return QRectF(0, 0, 210, 160);
}

void FuelManagement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	//Save thje painter and deactivate Antialising for rectangle drawing
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

	QPen edgePen(Qt::transparent, 0);
	double fuelAtDestination = fuelAmount - (fuelFlow * timeToDestination);

	//Draw fuel management
	switch(currentMode)
	{
	case fuelModeManagement:
		painter->setBrush(Qt::black);
		painter->setPen(Qt::transparent);
		painter->drawRect(boundingRect());

		painter->setBrush(Qt::darkBlue);
		painter->setPen(edgePen);
		painter->drawRoundedRect(remainingFuelRect, 5, 5);
		painter->drawRoundedRect(remainingFuelAtDestinationRect, 5, 5);
		painter->drawRoundedRect(fuelFlowRect, 5, 5);
		painter->drawRoundedRect(fuelingRect, 5, 5);
		painter->drawRoundedRect(homeRect, 5, 5);

		painter->setPen(Qt::white);
		painter->drawText(remainingFuelRect, Qt::AlignVCenter | Qt::AlignLeft, " Remaining Fuel:");
		painter->drawText(remainingFuelAtDestinationRect, Qt::AlignVCenter | Qt::AlignLeft, " Remaining Fuel at Destination:");
		painter->drawText(fuelFlowRect, Qt::AlignVCenter | Qt::AlignLeft, " Fuel flow:");

		painter->drawText(remainingFuelRect, Qt::AlignVCenter | Qt::AlignRight, QString::number(fuelAmount, 'f', 1).append(" l "));
		painter->drawText(remainingFuelAtDestinationRect, Qt::AlignVCenter | Qt::AlignRight, QString::number(fuelAtDestination, 'f', 1).append(" l "));
		painter->drawText(fuelFlowRect, Qt::AlignVCenter | Qt::AlignRight, QString::number(fuelFlow, 'f', 1).append(" l "));

		painter->drawText(fuelingRect, Qt::AlignCenter, "Fueling");
		painter->drawText(homeRect, Qt::AlignCenter, "Home");
		break;
	case fuelModeFueling:
		painter->setBrush(Qt::black);
		painter->setPen(Qt::transparent);
		painter->drawRect(boundingRect());

		painter->setBrush(Qt::darkBlue);
		painter->setPen(edgePen);
		painter->drawRoundedRect(remainingFuelRect, 5, 5);
		painter->save();
		painter->setBrush(Qt::red);
		painter->drawRoundedRect(fuelingRect, 5, 5);
		painter->restore();
		painter->drawRoundedRect(homeRect, 5, 5);

		painter->drawRoundedRect(add50LitersRect, 5, 5);
		painter->drawRoundedRect(add10LitersRect, 5, 5);
		painter->drawRoundedRect(add5LitersRect, 5, 5);
		painter->drawRoundedRect(add1LitersRect, 5, 5);

		painter->drawRoundedRect(clearRect, 5, 5);
		painter->drawRoundedRect(fuelTopRect, 5, 5);

		painter->setPen(Qt::white);
		painter->drawText(remainingFuelRect, Qt::AlignVCenter | Qt::AlignLeft, " Remaining Fuel:");
		painter->drawText(remainingFuelRect, Qt::AlignVCenter | Qt::AlignRight, QString::number(fuelAmount, 'f', 1).append(" l "));
		painter->drawText(addLitersTextRect, Qt::AlignVCenter| Qt::AlignLeft, " Add Fuel\n in liters");
		painter->drawText(add50LitersRect, Qt::AlignCenter, "+50");
		painter->drawText(add10LitersRect, Qt::AlignCenter, "+10");
		painter->drawText(add5LitersRect, Qt::AlignCenter, "+5");
		painter->drawText(add1LitersRect, Qt::AlignCenter, "+1");

		painter->drawText(clearRect, Qt::AlignCenter, "CLEAR");
		painter->drawText(fuelTopRect, Qt::AlignCenter, "FUEL TOP");

		painter->drawText(fuelingRect, Qt::AlignCenter, "Close\nApplication");
		painter->drawText(homeRect, Qt::AlignCenter, "Home");
		break;
	}

	painter->restore();
}

void FuelManagement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(homeRect.contains(event->pos()))
	{
		removeOverlay();
		currentMode = fuelModeManagement;
		update();
	}
	if(currentMode == fuelModeManagement)
	{
		if(fuelingRect.contains(event->pos()))
		{
			currentMode = fuelModeFueling;
			update();
		}
	}
	else if(currentMode == fuelModeFueling)
	{
		if(fuelingRect.contains(event->pos()))
		{
			qApp->quit();
		}
		if(add50LitersRect.contains(event->pos()))
		{
			fuelAmount += 50.0;
		}
		else if(add10LitersRect.contains(event->pos()))
		{
			fuelAmount += 10.0;
		}
		else if(add5LitersRect.contains(event->pos()))
		{
			fuelAmount += 5.0;
		}
		else if(add1LitersRect.contains(event->pos()))
		{
			fuelAmount += 1.0;
		}
		else if(fuelTopRect.contains(event->pos()))
		{
			fuelAmount = settings.value("Fueling/Capacity", 0.0).toDouble();
		}
		else if(clearRect.contains(event->pos()))
		{
			fuelAmount = 0.0;
		}
		update();
	}
}

void FuelManagement::setFuelFlow(double value)
{
	fuelFlow = value;
}

void FuelManagement::setTimeToDestination(double time)
{
	timeToDestination = time;
}
