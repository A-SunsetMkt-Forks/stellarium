/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "SkyGui.hpp"
#include "StelObjectMgr.hpp"
#include "StelGuiItems.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelCore.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include <QGraphicsView>
#include <QDebug>
#include <QTimeLine>
#include <QGraphicsSceneMouseEvent>
#include <QSettings>
#include <QTextDocument>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>

InfoPanel::InfoPanel(QGraphicsItem* parent) : QGraphicsTextItem("", parent)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	QString objectInfo = conf->value("gui/selected_object_info", "default").toString();
	if (objectInfo == "all")
	{
		infoTextFilters = StelObject::InfoStringGroup(StelObject::AllInfo);
	}
	else if (objectInfo == "short")
	{
		infoTextFilters = StelObject::InfoStringGroup(StelObject::ShortInfo);
	}
	else if (objectInfo == "none")
	{
		infoTextFilters = StelObject::InfoStringGroup(StelObject::None);
	}
	else if (objectInfo == "custom")
	{
		infoTextFilters = GETSTELMODULE(StelObjectMgr)->getCustomInfoStrings();
	}
	else
	{
		if (objectInfo != "default")
			qWarning() << "config.ini option gui/selected_object_info is invalid, using \"default\"";
		infoTextFilters = StelObject::InfoStringGroup(StelObject::DefaultInfo);
	}

	QFont font=QGuiApplication::font();
	font.setPixelSize(StelApp::getInstance().getScreenFontSize());
	setFont(font);
	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &QGraphicsTextItem::setFont);
	connect(&StelApp::getInstance(), &StelApp::screenFontSizeChanged, this, [=](int size){
		QFont font=QGuiApplication::font();
		font.setPixelSize(StelApp::getInstance().getScreenFontSize());
		setFont(font);
	});

	if (conf->value("gui/flag_info_shadow", false).toBool())
	{
		// Add a drop shadow for better visibility
		QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
		effect->setBlurRadius(6);
		effect->setColor(QColor(0, 0, 0));
		effect->setOffset(0,0);
		setGraphicsEffect(effect);
	}
}

InfoPanel::~InfoPanel()
{
}

void InfoPanel::setTextFromObjects(const QList<StelObjectP>& selected)
{
	if (selected.isEmpty())
	{
		if (!document()->isEmpty())
			document()->clear();
	}
	else
	{
		// just print details of the first item for now
		// Must set lastRTS for currently selected object here...
		StelCore *core=StelApp::getInstance().getCore();
		infoHTML = selected[0]->getInfoString(core, infoTextFilters);
		selected[0]->removeExtraInfoStrings(StelObject::AllInfo);
		setHtml(infoHTML);
	}
}

QString InfoPanel::getSelectedText() const
{
	return toPlainText();
}

QString InfoPanel::getSelectedHTML() const
{
	return infoHTML;
}

SkyGui::SkyGui(QGraphicsItem * parent)
	: QGraphicsWidget(parent)
	, lastBottomBarWidth(0)
	, btHorizAutoHide(nullptr)
	, btVertAutoHide(nullptr)
	, autoHidebts(nullptr)
	, autoHideBottomBar(true)
	, autoHideLeftBar(true)
	, stelGui(nullptr)
{
	setObjectName("StelSkyGui");

	// Construct the Windows buttons bar
	leftBar = new LeftStelBar(this);
	// Construct the bottom buttons bar
	bottomBar = new BottomStelBar(this,
				      QPixmap(":/graphicGui/btbgLeft.png"),
				      QPixmap(":/graphicGui/btbgRight.png"),
				      QPixmap(":/graphicGui/btbgMiddle.png"),
				      QPixmap(":/graphicGui/btbgSingle.png"));
	infoPanel = new InfoPanel(this);

	// Used to display some progress bar in the lower right corner, e.g. when loading a file
	progressBarMgr = new StelProgressBarMgr(this);
	connect(&StelApp::getInstance(), SIGNAL(progressBarAdded(const StelProgressController*)), progressBarMgr, SLOT(addProgressBar(const StelProgressController*)));
	connect(&StelApp::getInstance(), SIGNAL(progressBarRemoved(const StelProgressController*)), progressBarMgr, SLOT(removeProgressBar(const StelProgressController*)));

	// The path drawn around the button bars
	buttonBarsFrame = new StelBarsFrame(this);

	animLeftBarTimeLine = new QTimeLine(200, this);
	animLeftBarTimeLine->setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
	connect(animLeftBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos()));

	animBottomBarTimeLine = new QTimeLine(200, this);
	animBottomBarTimeLine->setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
	connect(animBottomBarTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(updateBarsPos()));

	setAcceptHoverEvents(true);
}

void SkyGui::init(StelGui* astelGui)
{
	stelGui = astelGui;

	// Create the 2 auto hide buttons in the bottom left corner
	autoHidebts = new CornerButtons(this);
	QPixmap pxmapOn = QPixmap(":/graphicGui/miscHorAutoHide-on.png");
	QPixmap pxmapOff = QPixmap(":/graphicGui/miscHorAutoHide-off.png");
	btHorizAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), "actionAutoHideHorizontalButtonBar", true);
	pxmapOn = QPixmap(":/graphicGui/miscVertAutoHide-on.png");
	pxmapOff = QPixmap(":/graphicGui/miscVertAutoHide-off.png");
	btVertAutoHide = new StelButton(autoHidebts, pxmapOn, pxmapOff, QPixmap(), "actionAutoHideVerticalButtonBar", true);

	btHorizAutoHide->setPos(1,btVertAutoHide->getButtonPixmapHeight()-btHorizAutoHide->getButtonPixmapHeight()+1);
	btVertAutoHide->setPos(0,0);
	btVertAutoHide->setZValue(1000);

	updateInfoPanelPos();

	// If auto hide is off, show the relevant toolbars
	if (!autoHideBottomBar)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}

	if (!autoHideLeftBar)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}

	buttonBarsFrame->setZValue(-0.1);
	updateBarsPos();
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
	connect(&StelApp::getInstance(), &StelApp::screenFontSizeChanged, this, &SkyGui::updateInfoPanelPos);
	connect(bottomBar, SIGNAL(sizeChanged()), this, SLOT(updateBarsPos()));
	// The first draw of path may show overshooting date line if there are too few buttons in the bottom bar.
	// Correct this by a redraw 1/2s after startup
	QTimer::singleShot(500, this, [=](){buttonBarsFrame->updatePath(bottomBar, leftBar);});
}

void SkyGui::updateInfoPanelPos()
{
	const auto factor = StelApp::getInstance().screenFontSizeRatio();
	infoPanel->setPos(8 * factor, 8 * factor);
}

void SkyGui::resizeEvent(QGraphicsSceneResizeEvent* event)
{
	QGraphicsWidget::resizeEvent(event);
	updateBarsPos();
}

void SkyGui::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	const int hh = getSkyGuiHeight();

	const double x = event->pos().x();
	const double y = event->pos().y();
	double maxX = leftBar->boundingRect().width()+2.*buttonBarsFrame->getRoundSize();
	double maxY = hh-(leftBar->boundingRect().height()+bottomBar->boundingRect().height()+2.*buttonBarsFrame->getRoundSize());
	const double minX = 0;

	if (x<=maxX && y>=maxY && animLeftBarTimeLine->state()==QTimeLine::NotRunning && leftBar->pos().x()<minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Forward);
		animLeftBarTimeLine->start();
	}
	if (autoHideLeftBar && (x>maxX+30 || y<maxY-30) && animLeftBarTimeLine->state()==QTimeLine::NotRunning && leftBar->pos().x()>=minX)
	{
		animLeftBarTimeLine->setDirection(QTimeLine::Backward);
		animLeftBarTimeLine->start();
	}

	maxX = leftBar->boundingRect().width()+bottomBar->boundingRect().width()+2.*buttonBarsFrame->getRoundSize();
	maxY = hh-bottomBar->boundingRect().height()+2.*buttonBarsFrame->getRoundSize();
	if (x<=maxX && y>=maxY && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()<1.)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Forward);
		animBottomBarTimeLine->start();
	}
	if (autoHideBottomBar && (x>maxX+30 || y<maxY-30) && animBottomBarTimeLine->state()==QTimeLine::NotRunning && animBottomBarTimeLine->currentValue()>=0.9999999)
	{
		animBottomBarTimeLine->setDirection(QTimeLine::Backward);
		animBottomBarTimeLine->start();
	}
}

// Used to recompute the bars position when we toggle the gui off and on.
// This was not necessary with Qt < 5.4.  So it might be a bug.
QVariant SkyGui::itemChange(GraphicsItemChange change, const QVariant & value)
{
	if (change == QGraphicsItem::ItemVisibleHasChanged && value.toBool())
		updateBarsPos();
	return QGraphicsItem::itemChange(change, value);
}

int SkyGui::getSkyGuiWidth() const
{
	return static_cast<int>(geometry().width());
}

int SkyGui::getSkyGuiHeight() const
{
	return static_cast<int>(geometry().height());
}

qreal SkyGui::getBottomBarHeight() const
{
	return bottomBar->boundingRect().height();
}

qreal SkyGui::getLeftBarWidth() const
{
	return leftBar->boundingRect().width();
}

//! Update the position of the button bars in the main window
void SkyGui::updateBarsPos()
{
	const int ww = getSkyGuiWidth();  // actually: window width
	const int hh = getSkyGuiHeight(); // actually: window height
	bool updatePath = false;

	// Use a position cache to avoid useless redraw triggered by the position set if the bars don't move
	const double rangeX = leftBar->boundingRectNoHelpLabel().width()+2.*buttonBarsFrame->getRoundSize()+1.;
	const qreal newLeftBarX = buttonBarsFrame->getRoundSize()-(1.-animLeftBarTimeLine->currentValue())*rangeX-0.5;
	const qreal newLeftBarY = hh-leftBar->boundingRectNoHelpLabel().height()-bottomBar->boundingRectNoHelpLabel().height()-20;
	if (!qFuzzyCompare(leftBar->pos().x(), newLeftBarX) || !qFuzzyCompare(leftBar->pos().y(), newLeftBarY))
	{
		leftBar->setPos(qRound(newLeftBarX), qRound(newLeftBarY));
		updatePath = true;
	}

	const double rangeY = bottomBar->getButtonsBoundingRect().height()+1.5+bottomBar->getGap();
	const qreal newBottomBarX = leftBar->boundingRectNoHelpLabel().right()+buttonBarsFrame->getRoundSize();
	const qreal newBottomBarY = hh-bottomBar->boundingRectNoHelpLabel().height()+bottomBar->getGap()-buttonBarsFrame->getRoundSize()+0.5+(1.-animBottomBarTimeLine->currentValue())*rangeY;

	if (!qFuzzyCompare(bottomBar->pos().x(), newBottomBarX) || !qFuzzyCompare(bottomBar->pos().y(), newBottomBarY))
	{
		bottomBar->setPos(qRound(newBottomBarX), qRound(newBottomBarY));
		updatePath = true;
	}

	if (lastBottomBarWidth != static_cast<int>(bottomBar->boundingRectNoHelpLabel().width()))
	{
		lastBottomBarWidth = static_cast<int>(bottomBar->boundingRectNoHelpLabel().width());
		updatePath = true;
	}

	if (updatePath)
		buttonBarsFrame->updatePath(bottomBar, leftBar);

	const qreal newProgressBarX = ww-progressBarMgr->boundingRect().width()-20;
	const qreal newProgressBarY = hh-progressBarMgr->boundingRect().height()+7;	
	progressBarMgr->setPos(newProgressBarX, newProgressBarY);
	progressBarMgr->setZValue(400);	

	// Update position of the auto-hide buttons
	btHorizAutoHide->setPos(1,btVertAutoHide->getButtonPixmapHeight()-btHorizAutoHide->getButtonPixmapHeight()+1);
	autoHidebts->setPos(0, hh-autoHidebts->childrenBoundingRect().height()+1);
	double opacity = qMax(animLeftBarTimeLine->currentValue(), animBottomBarTimeLine->currentValue());
	autoHidebts->setOpacity(qMax(0.01, opacity));	// Work around a qt bug

	// Update the screen as soon as possible.
	StelMainView::getInstance().thereWasAnEvent();
}

void SkyGui::setStelStyle(const QString& style)
{
	Q_UNUSED(style)
	buttonBarsFrame->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	buttonBarsFrame->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
	bottomBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
	leftBar->setColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
}

// Add a new progress bar in the lower right corner of the screen.
void SkyGui::addProgressBar(StelProgressController* p)
{
	progressBarMgr->addProgressBar(p);
}

void SkyGui::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
	stelGui->update();
}
