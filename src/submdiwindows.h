/***************************************************************************
  *      Copyright (C) 2010 by Renaud Guezennec                             *
 *                                                                         *
 *                                                                         *
 *   rolisteam is free software; you can redistribute it and/or modify     *
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
#ifndef SUBMDIWINDOWS_H
#define SUBMDIWINDOWS_H

#include <QMdiSubWindow>

#include "ToolBar.h"
class SubMdiWindows : public QMdiSubWindow
{
    Q_OBJECT
public:
    SubMdiWindows(QWidget* parent=0 );
    enum SubWindowType {MAP,TCHAT,PICTURE,TEXT,CHARACTERSHEET};

public slots:
    void changedStatus(Qt::WindowStates oldState,Qt::WindowStates newState);
    virtual void currentToolChanged(ToolsBar::SelectableTool);
    virtual void currentCursorChanged(QCursor*);
    virtual void currentColorChanged(QColor&);

    virtual void currentPenSizeChanged(int);

    virtual void currentNPCSizeChanged(int);
    virtual SubWindowType getType();

    virtual bool defineMenu(QMenu* menu)=0;

protected:
    bool m_active;
    QCursor* m_currentCursor;
    ToolsBar::SelectableTool m_currentTool;
    QColor m_penColor;
    int m_penSize;
    int m_npcSize;
    SubWindowType m_type;

};

#endif // SUBMDIWINDOWS_H
