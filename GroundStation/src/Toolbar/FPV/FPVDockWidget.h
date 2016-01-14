//---------------------------------------------------------------------------------------------------------------
//
// Filename   : FPVDockWidget.h
// Author     : Kuba Sejdak
// Created on : 11.02.2015
//
// This file is a part of SkyViper project.
//
// %LICENSE%
//
//---------------------------------------------------------------------------------------------------------------

#ifndef FPVDOCKWIDGET_H
#define FPVDOCKWIDGET_H

#include <QDockWidget>

namespace Ui {
    class FPVDockWidget;
}

class FPVDockWidget : public QDockWidget {
    Q_OBJECT

public:
    explicit FPVDockWidget(QWidget* parent = 0);
    ~FPVDockWidget();

private:
    Ui::FPVDockWidget* m_ui;
};

#endif
