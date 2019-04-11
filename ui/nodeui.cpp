#include "nodeui.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "ui/effectui.h"
#include "global/math.h"

const int kRoundedRectRadius = 5;
const int kNodePlugSize = 10;

NodeUI::NodeUI() :
  central_widget_(nullptr),
  clicked_socket_(-1)
{
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

NodeUI::~NodeUI()
{
  if (scene() != nullptr) {
    scene()->removeItem(proxy_);
    scene()->removeItem(this);
  }
}

void NodeUI::AddToScene(QGraphicsScene *scene)
{
  scene->addItem(this);

  if (central_widget_ != nullptr) {
    proxy_ = scene->addWidget(central_widget_);
    proxy_->setPos(pos() + QPoint(1 + kRoundedRectRadius, 1 + kRoundedRectRadius));
    proxy_->setParentItem(this);
  }
}

void NodeUI::Resize(const QSize &s)
{
  QRectF rectangle;
  rectangle.setTopLeft(pos());
  rectangle.setSize(s + 2 * QSize(kRoundedRectRadius, kRoundedRectRadius));

  QRectF inner_rect = rectangle;
  inner_rect.translate(kNodePlugSize / 2, 0);

  rectangle.setWidth(rectangle.width() + kNodePlugSize);

  path_ = QPainterPath();
  path_.addRoundedRect(inner_rect, kRoundedRectRadius, kRoundedRectRadius);

  setRect(rectangle);
}

void NodeUI::SetWidget(EffectUI *widget)
{
  central_widget_ = widget;
}

void NodeUI::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  QVector<QRectF> sockets = GetNodeSocketRects();
  if (!sockets.isEmpty()) {
    painter->setPen(Qt::black);
    painter->setBrush(Qt::gray);

    for (int i=0;i<sockets.size();i++) {
      painter->drawEllipse(sockets.at(i));
    }
  }

  QPalette palette = qApp->palette();

  if (option->state & QStyle::State_Selected) {
    painter->setPen(palette.highlight().color());
  } else {
    painter->setPen(palette.base().color());
  }
  painter->setBrush(palette.window());
  painter->drawPath(path_);
}

void NodeUI::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QVector<QRectF> sockets = GetNodeSocketRects();

  clicked_socket_ = -1;

  for (int i=0;i<sockets.size();i++) {
    if (sockets.at(i).contains(event->pos())) {
      clicked_socket_ = i;
      break;
    }
  }

  if (clicked_socket_ > -1) {
    drag_line_start_ = pos() + sockets.at(clicked_socket_).center();
    drag_line_ = scene()->addPath(GetEdgePath(drag_line_start_, event->scenePos()),
                                  QPen(Qt::white, 2));

    event->accept();
  } else {
    QGraphicsItem::mousePressEvent(event);
  }
}

void NodeUI::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (clicked_socket_ > -1) {

    drag_line_->setPath(GetEdgePath(drag_line_start_, event->scenePos()));

    event->accept();

  } else {
    QGraphicsItem::mouseMoveEvent(event);
  }
}

void NodeUI::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  if (clicked_socket_ > -1) {
    scene()->removeItem(drag_line_);
    delete drag_line_;
    event->accept();
  } else {
    QGraphicsItem::mouseReleaseEvent(event);
  }
}

QPainterPath NodeUI::GetEdgePath(const QPointF &start_pos, const QPointF &end_pos)
{
  double mid_x = double_lerp(start_pos.x(), end_pos.x(), 0.5);

  QPainterPath path_;
  path_.moveTo(start_pos);
  path_.cubicTo(QPointF(mid_x, start_pos.y()),
                QPointF(mid_x, end_pos.y()),
                end_pos);

  return path_;
}

QVector<QRectF> NodeUI::GetNodeSocketRects()
{
  QVector<QRectF> rects;

  if (proxy_ != nullptr) {
    Effect* e = central_widget_->GetEffect();

    for (int i=0;i<e->row_count();i++) {

      EffectRow* row = e->row(i);
      qreal x = (row->IsNodeOutput()) ? rect().right() - kNodePlugSize : rect().x();
      int y = central_widget_->GetRowY(i);

      if (row->IsNodeInput() || row->IsNodeOutput()) {
        rects.append(QRectF(x,
                            proxy_->pos().y() + y - kNodePlugSize/2,
                            kNodePlugSize,
                            kNodePlugSize));
      }
    }
  }

  return rects;
}
