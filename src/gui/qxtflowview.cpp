/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
** This is a derived work of PictureFlow (http://pictureflow.googlecode.com)
** The original code was distributed under the following terms:
**
** Copyright (C) 2008 Ariya Hidayat (ariya@kde.org)
** Copyright (C) 2007 Ariya Hidayat (ariya@kde.org)
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
****************************************************************************/





/*!
    \class QxtFlowView QxtFlowView
    \ingroup QxtGui
    \brief An ItemView for Images with impressive Flow effects

    an image show widget with animation effect 
    like Apple's CoverFlow (in iTunes and iPod). Images are arranged in form 
    of slides, one main slide is shown at the center with few slides on 
    the left and right sides of the center slide. When the next or previous 
    slide is brought to the front, the whole slides flow to the right or 
    the right with smooth animation effect; until the new slide is finally 
    placed at the center.
    <br>
    <br>
    This is a derived work of PictureFlow ( http://pictureflow.googlecode.com )


 */


#include "qxtflowview_p.h"
#include <QWheelEvent>

QxtFlowView::QxtFlowView(QWidget* parent): QWidget(parent)
{
    d = new QxtFlowViewPrivate;

    d->model=0;
    d->picrole=Qt::DecorationRole;
    d->textrole=Qt::DisplayRole;
    d->piccolumn=0;
    d->textcolumn=0;



    d->state = new QxtFlowViewState;
    d->state->reset();
    d->state->reposition();

    d->renderer = new QxtFlowViewSoftwareRenderer;
    d->renderer->state = d->state;
    d->renderer->widget = this;
    d->renderer->init();

    d->animator = new QxtFlowViewAnimator;
    d->animator->state = d->state;
    QObject::connect(&d->animator->animateTimer, SIGNAL(timeout()), this, SLOT(updateAnimation()));

    QObject::connect(&d->triggerTimer, SIGNAL(timeout()), this, SLOT(render()));

    setAttribute(Qt::WA_StaticContents, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

QxtFlowView::~QxtFlowView()
{
    delete d->renderer;
    delete d->animator;
    delete d->state;
    delete d;
}

void QxtFlowView::setModel(QAbstractItemModel * model)
{
    d->setModel(model);
}

QAbstractItemModel * QxtFlowView::model()
{
    return d->model;
}


QColor QxtFlowView::backgroundColor() const
{
    return QColor(d->state->backgroundColor);
}

void QxtFlowView::setBackgroundColor(const QColor& c)
{
    d->state->backgroundColor = c.rgb();
    triggerRender();
}

QSize QxtFlowView::slideSize() const
{
    return QSize(d->state->slideWidth, d->state->slideHeight);
}

void QxtFlowView::setSlideSize(QSize size)
{
    d->state->slideWidth = size.width();
    d->state->slideHeight = size.height();
    d->state->reposition();
    triggerRender();
}

QxtFlowView::ReflectionEffect QxtFlowView::reflectionEffect() const
{
    return d->state->reflectionEffect;
}

void QxtFlowView::setReflectionEffect(ReflectionEffect effect)
{
    d->state->reflectionEffect = effect;
    d->reset();
}


int QxtFlowView::pictureRole()
{
    return d->picrole;
}

void QxtFlowView::setPictureRole(int a)
{
    d->picrole=a;
    d->reset();
}





int QxtFlowView::picureColumn()
{
    return d->piccolumn;
}

void QxtFlowView::setPicureColumn(int a)
{
    d->piccolumn=a;
    d->reset();
}

#if 0
int QxtFlowView::textRole()
{
    return d->textrole;
}

void QxtFlowView::setTextRole(int a)
{
    d->textrole= a;
    d->reset();
}

int QxtFlowView::textColumn()
{
    return d->textcolumn;
}

void QxtFlowView::setTextColumn(int a)
{
    d->textcolumn=a;
    d->reset();
}
#endif





QModelIndex QxtFlowView::currentIndex() const
{
    return d->model->index(d->state->centerIndex,0,QModelIndex());
}

void QxtFlowView::setCurrentIndex(QModelIndex index)
{
    d->state->centerIndex = index.row();
    d->state->reset();
    d->animator->stop(index.row());
    triggerRender();
}

void QxtFlowView::render()
{
    d->renderer->dirty = true;
    update();
}

void QxtFlowView::triggerRender()
{
    d->triggerRender();
}


void QxtFlowView::showPrevious()
{
    int step = d->animator->step;
    int center = d->state->centerIndex;

    if(step > 0)
        d->animator->start(center);

    if(step == 0)
        if(center > 0)
            d->animator->start(center - 1);

    if(step < 0)
        d->animator->target = qMax(0, center - 2);
}

void QxtFlowView::showNext()
{
    int step = d->animator->step;
    int center = d->state->centerIndex;

    if(step < 0)
        d->animator->start(center);

    if(step == 0)
        if(center < d->state->slideImages.count()-1)
            d->animator->start(center + 1);

    if(step > 0)
        d->animator->target = qMin(center + 2, d->state->slideImages.count()-1);
}

void QxtFlowView::showSlide(QModelIndex index)
{
    d->showSlide(index.row());
}

void QxtFlowView::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Left)
    {
        if(event->modifiers() == Qt::ControlModifier)
            d->showSlide(currentIndex().row()-10);
        else
            showPrevious();
        event->accept();
        return;
    }

    if(event->key() == Qt::Key_Right)
    {
        if(event->modifiers() == Qt::ControlModifier)
            d->showSlide(currentIndex().row()+10);
        else
            showNext();
        event->accept();
        return;
    }

    event->ignore();
}

void QxtFlowView::mousePressEvent(QMouseEvent* event)
{
    if(event->x() > width()/2)
        showNext();
    else
        showPrevious();
}

void QxtFlowView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    d->renderer->paint();
}

void QxtFlowView::resizeEvent(QResizeEvent* event)
{
    triggerRender();
    QWidget::resizeEvent(event);
}



void QxtFlowView::wheelEvent ( QWheelEvent * event )
{

    if (event->orientation() == Qt::Horizontal) 
    {
        event->ignore();
    } 
    else 
    {
        int numSteps = (event->delta() / 8) / 15;
        if(numSteps>0)
        {
            for (int i=0;i<numSteps;i++)
            {
                showNext();
            }
        }
        else
        {
            for (int i=numSteps;i<0;i++)
            {
                showPrevious();
            }
        }
        event->accept();
    }


}




void QxtFlowView::updateAnimation()
{
    int old_center = d->state->centerIndex;
    d->animator->update();
    triggerRender();
    if(d->state->centerIndex != old_center)
        emit currentIndexChanged(d->model->index(d->state->centerIndex,0));
}









void QxtFlowViewPrivate::columnsAboutToBeInserted ( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);

}

void QxtFlowViewPrivate::columnsAboutToBeRemoved ( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);

}

void QxtFlowViewPrivate::columnsInserted ( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);

}

void QxtFlowViewPrivate::columnsRemoved ( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QxtFlowViewPrivate::dataChanged ( const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
}

void QxtFlowViewPrivate::headerDataChanged ( Qt::Orientation orientation, int first, int last )
{
    Q_UNUSED(orientation);
    Q_UNUSED(first);
    Q_UNUSED(last);

}

void QxtFlowViewPrivate::layoutAboutToBeChanged ()
{
}

void QxtFlowViewPrivate::layoutChanged ()
{
}

void QxtFlowViewPrivate::modelAboutToBeReset ()
{
}

void QxtFlowViewPrivate::modelReset ()
{
}

void QxtFlowViewPrivate::rowsAboutToBeInserted ( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QxtFlowViewPrivate::rowsAboutToBeRemoved ( const QModelIndex & parent, int start, int end )
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
}

void QxtFlowViewPrivate::rowsInserted ( const QModelIndex & parent, int start, int end )
{
    if(parent.isValid())
        return;
    for(int i=start;i<=end;i++)
        insertSlide(i,qvariant_cast<QImage>(model->data(model->index(i,piccolumn),picrole)));
}

void QxtFlowViewPrivate::rowsRemoved ( const QModelIndex & parent, int start, int end )
{
    if(parent.isValid())
        return;
    for(int i=start;i<=end;i++)
        removeSlide  (i);

}

void QxtFlowViewPrivate::setModel(QAbstractItemModel * m)
{


    if(model)
    {

        disconnect(this->model,SIGNAL(columnsAboutToBeInserted ( const QModelIndex & , int , int  )),
            this,SLOT(columnsAboutToBeInserted ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(columnsInserted ( const QModelIndex & , int , int  )),
            this,SLOT(columnsInserted ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(columnsRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(columnsRemoved ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(dataChanged ( const QModelIndex & , const QModelIndex &  )),
            this,SLOT(dataChanged ( const QModelIndex & , const QModelIndex &  )));
        disconnect(this->model,SIGNAL(headerDataChanged ( Qt::Orientation , int , int  )),
            this,SLOT(headerDataChanged ( Qt::Orientation , int , int  )));
        disconnect(this->model,SIGNAL(layoutAboutToBeChanged ()),
            this,SLOT(layoutAboutToBeChanged ()));
        disconnect(this->model,SIGNAL(layoutChanged ()),
            this,SLOT(layoutChanged ()));
        disconnect(this->model,SIGNAL(modelAboutToBeReset ()),
            this,SLOT(modelAboutToBeReset ()));
        disconnect(this->model,SIGNAL(modelReset ()),
            this,SLOT(modelReset ()));
        disconnect(this->model,SIGNAL(rowsAboutToBeInserted ( const QModelIndex & , int , int  )),
            this,SLOT(rowsAboutToBeInserted ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(rowsInserted ( const QModelIndex & , int , int  )),
            this,SLOT(rowsInserted ( const QModelIndex & , int , int  )));
        disconnect(this->model,SIGNAL(rowsRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(rowsRemoved ( const QModelIndex & , int , int  )));
    }
    model = m;
    if(model)
    {

        connect(this->model,SIGNAL(columnsAboutToBeInserted ( const QModelIndex & , int , int  )),
            this,SLOT(columnsAboutToBeInserted ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(columnsInserted ( const QModelIndex & , int , int  )),
            this,SLOT(columnsInserted ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(columnsRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(columnsRemoved ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(dataChanged ( const QModelIndex & , const QModelIndex &  )),
            this,SLOT(dataChanged ( const QModelIndex & , const QModelIndex &  )));
        connect(this->model,SIGNAL(headerDataChanged ( Qt::Orientation , int , int  )),
            this,SLOT(headerDataChanged ( Qt::Orientation , int , int  )));
        connect(this->model,SIGNAL(layoutAboutToBeChanged ()),
            this,SLOT(layoutAboutToBeChanged ()));
        connect(this->model,SIGNAL(layoutChanged ()),
            this,SLOT(layoutChanged ()));
        connect(this->model,SIGNAL(modelAboutToBeReset ()),
            this,SLOT(modelAboutToBeReset ()));
        connect(this->model,SIGNAL(modelReset ()),
            this,SLOT(modelReset ()));
        connect(this->model,SIGNAL(rowsAboutToBeInserted ( const QModelIndex & , int , int  )),
            this,SLOT(rowsAboutToBeInserted ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(rowsInserted ( const QModelIndex & , int , int  )),
            this,SLOT(rowsInserted ( const QModelIndex & , int , int  )));
        connect(this->model,SIGNAL(rowsRemoved ( const QModelIndex & , int , int  )),
            this,SLOT(rowsRemoved ( const QModelIndex & , int , int  )));
    }

    reset();
}


void QxtFlowViewPrivate::clear()
{
    int c = state->slideImages.count();
    for(int i = 0; i < c; i++)
        delete state->slideImages[i];
    state->slideImages.resize(0);

    state->reset();
    triggerRender();
}


void QxtFlowViewPrivate::triggerRender()
{
    triggerTimer.setSingleShot(true);
    triggerTimer.start(0);
}



void QxtFlowViewPrivate::insertSlide  (int index, const QImage& image)
{
    state->slideImages.insert(index,new QImage(image));
    triggerRender();
}

void QxtFlowViewPrivate::replaceSlide (int index, const QImage& image)
{
    Q_ASSERT((index >= 0) && (index < state->slideImages.count()));

    QImage* i = image.isNull() ? 0 : new QImage(image);
    delete state->slideImages[index];
    state->slideImages[index] = i;
    triggerRender();
}

void QxtFlowViewPrivate::removeSlide  (int index)
{
    delete state->slideImages[index];
    state->slideImages.remove(index);
    triggerRender();
}


void QxtFlowViewPrivate::showSlide(int index)
{
    if(index == state->centerSlide.slideIndex)
        return;
    animator->start(index);
}



void QxtFlowViewPrivate::reset()
{
    clear();
    if(model)
    {
        for (int i=0;i<model->rowCount();i++)
        {
            insertSlide(i,qvariant_cast<QImage>(model->data(model->index(i,piccolumn),picrole)));
        }
    }
    triggerRender();
}