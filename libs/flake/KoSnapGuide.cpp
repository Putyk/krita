/* This file is part of the KDE project
 * Copyright (C) 2008 Jan Hambrecht <jaham@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KoSnapGuide.h"
#include "KoSnapStrategy.h"

#include <KoPathShape.h>
#include <KoPathPoint.h>
#include <KoViewConverter.h>
#include <KoCanvasBase.h>
#include <KoShapeManager.h>

#include <QtGui/QPainter>

#include <math.h>


KoSnapGuide::KoSnapGuide( KoCanvasBase * canvas )
    : m_canvas(canvas), m_editedShape(0), m_currentStrategy(0)
    , m_usedStrategies(0), m_active(true), m_snapDistance(10)
{
    m_strategies.append( new GridSnapStrategy() );
    m_strategies.append( new NodeSnapStrategy() );
    m_strategies.append( new OrthogonalSnapStrategy() );
    m_strategies.append( new ExtensionSnapStrategy() );
    m_strategies.append( new IntersectionSnapStrategy() );
}

KoSnapGuide::~KoSnapGuide()
{
}

void KoSnapGuide::setEditedShape( KoShape * shape )
{
    m_editedShape = shape;
}

KoShape * KoSnapGuide::editedShape() const
{
    return m_editedShape;
}

void KoSnapGuide::enableSnapStrategies( int strategies )
{
    m_usedStrategies = strategies;
}

int KoSnapGuide::enabledSnapStrategies() const
{
    return m_usedStrategies;
}

void KoSnapGuide::enableSnapping( bool on )
{
    m_active = on;
}

bool KoSnapGuide::isSnapping() const
{
    return m_active;
}

void KoSnapGuide::setSnapDistance( int distance )
{
    m_snapDistance = qAbs( distance );
}

int KoSnapGuide::snapDistance() const
{
    return m_snapDistance;
}

QPointF KoSnapGuide::snap( const QPointF &mousePosition, Qt::KeyboardModifiers modifiers )
{
    m_currentStrategy = 0;

    if( ! m_active || (modifiers & Qt::ShiftModifier) )
        return mousePosition;

    KoSnapProxy proxy( this );

    double minDistance = HUGE_VAL;

    double maxSnapDistance = m_canvas->viewConverter()->viewToDocument( QSizeF( m_snapDistance, m_snapDistance ) ).width();

    foreach( KoSnapStrategy * strategy, m_strategies )
    {
        if( m_usedStrategies & strategy->type() || strategy->type() == KoSnapStrategy::Grid )
        {
            if( ! strategy->snap( mousePosition, &proxy, maxSnapDistance ) )
                continue;

            QPointF snapCandidate = strategy->snappedPosition();
            double distance = KoSnapStrategy::fastDistance( snapCandidate, mousePosition );
            if( distance < minDistance )
            {
                m_currentStrategy = strategy;
                minDistance = distance;
            }
        }
    }

    if( ! m_currentStrategy )
        return mousePosition;

    return m_currentStrategy->snappedPosition();
}

QRectF KoSnapGuide::boundingRect()
{
    QRectF rect;

    if( m_currentStrategy )
        rect = m_currentStrategy->decoration().boundingRect();

    return rect.adjusted( -2, -2, 2, 2 );
}

void KoSnapGuide::paint( QPainter &painter, const KoViewConverter &converter )
{
    Q_UNUSED(converter);

    if( ! m_currentStrategy || ! m_active )
        return;

    QPen pen( Qt::red );
    pen.setStyle( Qt::DotLine );
    painter.setPen( pen );
    painter.setBrush( Qt::NoBrush );
    painter.drawPath( m_currentStrategy->decoration() );
}

KoCanvasBase * KoSnapGuide::canvas() const
{
    return m_canvas;
}

void KoSnapGuide::setIgnoredPathPoints( const QList<KoPathPoint*> &ignoredPoints )
{
    m_ignoredPoints = ignoredPoints;
}

QList<KoPathPoint*> KoSnapGuide::ignoredPathPoints() const
{
    return m_ignoredPoints;
}

void KoSnapGuide::reset()
{
    m_currentStrategy = 0;
    m_editedShape = 0;
    m_ignoredPoints.clear();
}

/////////////////////////////////////////////////////////
// snap proxy
/////////////////////////////////////////////////////////

KoSnapProxy::KoSnapProxy( KoSnapGuide * snapGuide )
    : m_snapGuide(snapGuide)
{
}

QList<QPointF> KoSnapProxy::pointsInRect( const QRectF &rect )
{
    QList<QPointF> points;
    QList<KoShape*> shapes = shapesInRect( rect );
    foreach( KoShape * shape, shapes )
    {
        foreach( QPointF point, pointsFromShape( shape ) )
        {
            if( rect.contains( point ) )
                points.append( point );
        }
    }

    return points;
}

QList<KoShape*> KoSnapProxy::shapesInRect( const QRectF &rect, bool omitEditedShape )
{
    QList<KoShape*> shapes = m_snapGuide->canvas()->shapeManager()->shapesAt( rect );

    if( ! omitEditedShape && m_snapGuide->editedShape() )
    {
        QRectF bound = m_snapGuide->editedShape()->boundingRect();
        if( rect.intersects( bound ) || rect.contains( bound ) )
            shapes.append( m_snapGuide->editedShape() );
    }
    return shapes;
}

QList<QPointF> KoSnapProxy::pointsFromShape( KoShape * shape )
{
    QList<QPointF> pathPoints;

    KoPathShape * path = dynamic_cast<KoPathShape*>( shape );
    if( ! path )
    {
        // return the special snap points of the shape
        return shape->snapData().snapPoints();
    }

    QMatrix m = path->absoluteTransformation(0);

    QList<KoPathPoint*> ignoredPoints = m_snapGuide->ignoredPathPoints();

    int subpathCount = path->subpathCount();
    for( int subpathIndex = 0; subpathIndex < subpathCount; ++subpathIndex )
    {
        int pointCount = path->pointCountSubpath( subpathIndex );
        for( int pointIndex = 0; pointIndex < pointCount; ++pointIndex )
        {
            KoPathPoint * p = path->pointByIndex( KoPathPointIndex( subpathIndex, pointIndex )  );
            if( ! p || ignoredPoints.contains( p ) )
                continue;

            pathPoints.append( m.map( p->point() ) );
        }
    }

    if( shape == m_snapGuide->editedShape() )
        pathPoints.removeLast();

    return pathPoints;
}

QList<KoPathSegment> KoSnapProxy::segmentsInRect( const QRectF &rect )
{
    QList<KoShape*> shapes = shapesInRect( rect, true );

    QList<KoPathSegment> segments;
    foreach( KoShape * shape, shapes )
    {
        QList<KoPathSegment> shapeSegments;
        QRectF rectOnShape = shape->documentToShape( rect );
        KoPathShape * path = dynamic_cast<KoPathShape*>( shape );
        if( path )
        {
            shapeSegments = path->segmentsAt( rectOnShape );
        }
        else
        {
            foreach( KoPathSegment s, shape->snapData().snapSegments() )
            {
                QRectF controlRect = s.controlPointRect();
                if( ! rect.intersects( controlRect ) && ! controlRect.contains( rect ) )
                    continue;
                QRectF bound = s.boundingRect();
                if( ! rect.intersects( bound ) && ! bound.contains( rect ) )
                    continue;
                shapeSegments.append( s );
            }
        }

        QMatrix m = shape->absoluteTransformation(0);
        // transform segments to document coordinates
        foreach( KoPathSegment s, shapeSegments )
        {
            segments.append( s.mapped( m ) );
        }
    }
    return segments;
}

QList<KoShape*> KoSnapProxy::shapes( bool omitEditedShape )
{
    QList<KoShape*> shapes = m_snapGuide->canvas()->shapeManager()->shapes();
    if( ! omitEditedShape && m_snapGuide->editedShape() )
        shapes.append( m_snapGuide->editedShape() );
    return shapes;
}

KoCanvasBase * KoSnapProxy::canvas()
{
    return m_snapGuide->canvas();
}
