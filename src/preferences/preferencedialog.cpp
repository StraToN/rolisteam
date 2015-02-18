/***************************************************************************
 *	 Copyright (C) 2009 by Renaud Guezennec                                *
 *   http://renaudguezennec.homelinux.org/accueil,3.html                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "preferencedialog.h"
#include "ui_preferencedialog.h"
#include "preferencesmanager.h"

#include <QFileDialog>

PreferenceDialog::PreferenceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferenceDialog)
{
    ui->setupUi(this);
    m_options = PreferencesManager::getInstance();

    //init value:
    ui->m_wsBgPathLine->setText(m_options->value("worspace/background/image",":/resources/icones/fond workspace macos.bmp").toString());
    connect(ui->m_wsBgBrowserButton,SIGNAL(clicked()),this,SLOT(changeBackgroundImage()));







    connect(ui->m_buttonbox,SIGNAL(clicked(QAbstractButton * )),this,SLOT(applyAllChanges(QAbstractButton * )));
}

PreferenceDialog::~PreferenceDialog()
{
    delete ui;
}

void PreferenceDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void PreferenceDialog::changeBackgroundImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Rolisteam Background"),".",tr(
        "Supported files (*.jpg *.png *.gif *.svg)"));

    if(!fileName.isEmpty())
    {
        ui->m_wsBgPathLine->setText(fileName);
    }
}
void PreferenceDialog::applyAllChanges(QAbstractButton * button)
{
    if(QDialogButtonBox::ApplyRole==ui->m_buttonbox->buttonRole(button))
    {
        m_options->registerValue("worspace/background/image",ui->m_wsBgPathLine->text());

        emit preferencesChanged();
    }
}
