/*
 * Copyright (c) 2010 Kevin Smith
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "QtStatusWidget.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <QPoint>
#include <QStackedWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <qdebug.h>
#include <QListWidget>
#include <QListWidgetItem>

#include "Swift/QtUI/QtElidingLabel.h"
#include "Swift/QtUI/QtLineEdit.h"
#include "Swift/QtUI/QtSwiftUtil.h"

namespace Swift {

QtStatusWidget::QtStatusWidget(QWidget *parent) : QWidget(parent), editCursor_(Qt::IBeamCursor), viewCursor_(Qt::PointingHandCursor) {
	isClicking_ = false;
	setMaximumHeight(24);

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0,0,0,0);

	stack_ = new QStackedWidget(this);
	stack_->setLineWidth(2);
	stack_->setFrameShape(QFrame::StyledPanel);
	mainLayout->addWidget(stack_);

	QWidget* page1 = new QWidget(this);
	stack_->addWidget(page1);
	QHBoxLayout* page1Layout = new QHBoxLayout(page1);
	page1Layout->setSpacing(0);
	page1Layout->setContentsMargins(0,0,0,0);
	page1->setCursor(viewCursor_);

	statusIcon_ = new QLabel(this);
	statusIcon_->setMinimumSize(16, 16);
	statusIcon_->setMaximumSize(16, 16);
	page1Layout->addWidget(statusIcon_);

	statusTextLabel_ = new QtElidingLabel(this);
	page1Layout->addWidget(statusTextLabel_);

	icons_[StatusShow::Online] = QIcon(":/icons/online.png");
	icons_[StatusShow::Away] = QIcon(":/icons/away.png");
	icons_[StatusShow::DND] = QIcon(":/icons/dnd.png");
	icons_[StatusShow::None] = QIcon(":/icons/offline.png");
	
	setStatusType(StatusShow::None);

	QWidget* page2 = new QWidget(this);
	QHBoxLayout* page2Layout = new QHBoxLayout(page2);
	page2Layout->setSpacing(0);
	page2Layout->setContentsMargins(0,0,0,0);
	stack_->addWidget(page2);
	
	statusEdit_ = new QtLineEdit(this);
	page2Layout->addWidget(statusEdit_);
	connect(statusEdit_, SIGNAL(returnPressed()), this, SLOT(handleEditComplete()));
	connect(statusEdit_, SIGNAL(escapePressed()), this, SLOT(handleEditCancelled()));
	connect(statusEdit_, SIGNAL(textChanged(const QString&)), this, SLOT(generateList()));

	setStatusText("");


	menu_ = new QListWidget();
	menu_->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint );
	menu_->setAlternatingRowColors(true);
	menu_->setFocusProxy(statusEdit_);
	menu_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	menu_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QSizePolicy policy(menu_->sizePolicy());
	policy.setVerticalPolicy(QSizePolicy::Expanding);
	menu_->setSizePolicy(policy);


	connect(menu_, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(handleItemClicked(QListWidgetItem*)));

	viewMode();
}

QtStatusWidget::~QtStatusWidget() {
	delete menu_;
}

void QtStatusWidget::handleApplicationFocusChanged(QWidget* /*old*/, QWidget* /*now*/) {
	QWidget* now = qApp->focusWidget();
	if (!editing_ || stack_->currentIndex() == 0) {
		return;
	}
	if (!now || (now != menu_ && now != statusEdit_ && !now->isAncestorOf(statusEdit_) && !now->isAncestorOf(menu_) && !statusEdit_->isAncestorOf(now) && !menu_->isAncestorOf(now))) {
		handleEditCancelled();
	}
	
}

void QtStatusWidget::mousePressEvent(QMouseEvent*) {
	if (stack_->currentIndex() == 0) {
		handleClicked();
	}
}

void QtStatusWidget::generateList() {
	if (!editing_) {
		return;
	}
	QString text = statusEdit_->text();
	newStatusText_ = text;
	menu_->clear();
	foreach (StatusShow::Type type, icons_.keys()) {
		QListWidgetItem* item = new QListWidgetItem(text, menu_);
		item->setIcon(icons_[type]);
		item->setData(Qt::UserRole, QVariant(type));
	}
	foreach (StatusShow::Type type, icons_.keys()) {
		QListWidgetItem* item = new QListWidgetItem(P2QSTRING(StatusShow::typeToFriendlyName(type)), menu_);
		item->setIcon(icons_[type]);
		item->setData(Qt::UserRole, QVariant(type));
	}
}


void QtStatusWidget::handleClicked() {
	editing_ = true;
	QDesktopWidget* desktop = QApplication::desktop();
	int screen = desktop->screenNumber(this);
	QPoint point = mapToGlobal(QPoint(0, height()));
	QRect geometry = desktop->availableGeometry(screen);
	int x = point.x();
	int y = point.y();
	int width = 200;
	int height = 80;

	int screenWidth = geometry.x() + geometry.width();
	if (x + width > screenWidth) {
		x = screenWidth - width;
	}
	generateList();

	height = menu_->sizeHintForRow(0) * menu_->count();
	menu_->setGeometry(x, y, width, height);
	menu_->move(x, y);
	menu_->setMaximumWidth(width);
	menu_->show();
	activateWindow();
	statusEdit_->selectAll();
	stack_->setCurrentIndex(1);
	statusEdit_->setFocus();
	connect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(handleApplicationFocusChanged(QWidget*, QWidget*)), Qt::QueuedConnection);
}

void QtStatusWidget::viewMode() {
	disconnect(qApp, SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(handleApplicationFocusChanged(QWidget*, QWidget*)));
	editing_ = false;
	menu_->hide();
	stack_->setCurrentIndex(0);	
}

void QtStatusWidget::handleEditComplete() {
	editing_ = false;
	statusText_ = newStatusText_;
	viewMode();
	emit onChangeStatusRequest(selectedStatusType_, statusText_);
}

void QtStatusWidget::handleEditCancelled() {
	editing_ = false;
	setStatusText(statusText_);
	viewMode();
}

StatusShow::Type QtStatusWidget::getSelectedStatusShow() {
	return selectedStatusType_;
}

void QtStatusWidget::handleItemClicked(QListWidgetItem* item) {
	editing_ = false;
	selectedStatusType_ = (StatusShow::Type)(item->data(Qt::UserRole).toInt());
	newStatusText_ = item->data(Qt::DisplayRole).toString();
	statusEdit_->setText(newStatusText_);
	handleEditComplete();
}


void QtStatusWidget::setStatusText(const QString& text) {
	statusText_ = text;
	statusEdit_->setText(text);
	QString escapedText(text.isEmpty() ? "(No message)" : text);
	escapedText.replace("<","&lt;");
	statusTextLabel_->setText("<i>" + escapedText + "</i>");
}

void QtStatusWidget::setStatusType(StatusShow::Type type) {
	selectedStatusType_ = icons_.contains(type) ? type : StatusShow::Online;
	statusIcon_->setPixmap(icons_[selectedStatusType_].pixmap(16, 16));
}

}




