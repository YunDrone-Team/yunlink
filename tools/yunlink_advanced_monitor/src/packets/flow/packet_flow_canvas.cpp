#include "packets/flow/packet_flow_canvas.hpp"

#include <algorithm>

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include "packets/flow/packet_flow_format.hpp"

namespace {

QColor node_fill(const PacketFlowStep& step) {
    if (!step.observed) {
        return QColor("#f2f4f7");
    }
    if (step.failed) {
        return QColor("#fff1f0");
    }
    if (step.active) {
        return QColor("#e8f3ff");
    }
    return QColor("#ecfdf3");
}

QColor node_border(const PacketFlowStep& step, bool hovered) {
    if (hovered) {
        return QColor("#1d4ed8");
    }
    if (!step.observed) {
        return QColor("#98a2b3");
    }
    if (step.failed) {
        return QColor("#d92d20");
    }
    if (step.active) {
        return QColor("#1570ef");
    }
    return QColor("#12b76a");
}

QString elide_text(const QString& text, const QFontMetrics& metrics, int width) {
    return metrics.elidedText(text, Qt::ElideRight, std::max(20, width));
}

void draw_arrow(QPainter* painter, const QPoint& from, const QPoint& to, const QColor& color) {
    painter->setPen(QPen(color, 2));
    painter->drawLine(from, to);
    const int head = 7;
    QPolygon triangle;
    triangle << to << QPoint(to.x() - head, to.y() - head / 2)
             << QPoint(to.x() - head, to.y() + head / 2);
    painter->setBrush(color);
    painter->drawPolygon(triangle);
}

}  // namespace

PacketFlowCanvas::PacketFlowCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(320);
    setMouseTracking(true);
}

void PacketFlowCanvas::set_snapshot(PacketFlowSnapshot snapshot) {
    snapshot_ = std::move(snapshot);
    update();
}

void PacketFlowCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#ffffff"));

    const QRect bounds = rect().adjusted(18, 18, -18, -18);
    update_node_rects(bounds);

    const auto* journey = primary_journey();
    painter.setPen(QColor("#101828"));
    QFont title_font = font();
    title_font.setPointSize(title_font.pointSize() + 2);
    title_font.setBold(true);
    painter.setFont(title_font);
    const QString title =
        journey == nullptr ? QString("协议流程") : QString::fromStdString(journey->title);
    painter.drawText(bounds.left(), bounds.top(), title);

    painter.setFont(font());
    painter.setPen(QColor("#475467"));
    const QString subtitle =
        journey == nullptr ? QString("暂无流程") : QString::fromStdString(journey->subtitle);
    painter.drawText(bounds.left(), bounds.top() + 22, subtitle);

    if (journey == nullptr || journey->steps.empty()) {
        painter.setPen(QColor("#667085"));
        painter.drawText(bounds.adjusted(0, 70, 0, 0), "暂无流程数据。");
        return;
    }

    for (size_t i = 1; i < node_rects_.size(); ++i) {
        const auto& previous = node_rects_[i - 1];
        const auto& current = node_rects_[i];
        const bool observed =
            journey->steps[i - 1].observed && journey->steps[i].observed;
        draw_arrow(&painter,
                   QPoint(previous.right() + 4, previous.center().y()),
                   QPoint(current.left() - 4, current.center().y()),
                   observed ? QColor("#667085") : QColor("#d0d5dd"));
    }

    QFontMetrics metrics(font());
    for (size_t i = 0; i < journey->steps.size() && i < node_rects_.size(); ++i) {
        const auto& step = journey->steps[i];
        const QRect box = node_rects_[i];
        const bool hovered = hovered_index_ == static_cast<int>(i);
        painter.setPen(QPen(node_border(step, hovered), step.active || hovered ? 2 : 1));
        painter.setBrush(node_fill(step));
        painter.drawRoundedRect(box, 6, 6);

        painter.setPen(step.observed ? QColor("#101828") : QColor("#667085"));
        QFont label_font = font();
        label_font.setBold(true);
        painter.setFont(label_font);
        painter.drawText(box.adjusted(8, 8, -8, -8),
                         Qt::AlignTop | Qt::AlignHCenter,
                         elide_text(QString::fromStdString(packet_flow_stage_short(step.stage)),
                                    QFontMetrics(label_font),
                                    box.width() - 16));

        painter.setFont(font());
        painter.setPen(QColor("#475467"));
        painter.drawText(box.adjusted(8, 34, -8, -8),
                         Qt::AlignTop | Qt::AlignHCenter,
                         elide_text(QString::fromStdString(step.subtitle), metrics, box.width() - 16));
    }

    const int list_top = node_rects_.empty() ? bounds.top() + 170 : node_rects_.front().bottom() + 28;
    painter.setPen(QColor("#344054"));
    painter.drawText(bounds.left(), list_top, "最近流程");
    int y = list_top + 22;
    const size_t count = std::min<size_t>(snapshot_.journeys.size(), 6);
    for (size_t i = 0; i < count; ++i) {
        const auto& item = snapshot_.journeys[i];
        const QRect row(bounds.left(), y - 14, bounds.width(), 24);
        if (i == 0) {
            painter.fillRect(row, QColor("#f9fafb"));
        }
        painter.setPen(QColor("#101828"));
        painter.drawText(row.adjusted(8, 0, -8, 0),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         elide_text(QString::fromStdString(item.title + "  " + item.subtitle),
                                    metrics,
                                    row.width() - 16));
        y += 26;
    }
}

void PacketFlowCanvas::mouseMoveEvent(QMouseEvent* event) {
    int index = -1;
    for (size_t i = 0; i < node_rects_.size(); ++i) {
        if (node_rects_[i].contains(event->pos())) {
            index = static_cast<int>(i);
            break;
        }
    }
    if (index == hovered_index_) {
        return;
    }
    hovered_index_ = index;
    const auto* journey = primary_journey();
    if (journey != nullptr && index >= 0 && index < static_cast<int>(journey->steps.size())) {
        const auto& step = journey->steps[static_cast<size_t>(index)];
        QToolTip::showText(event->globalPos(),
                           QString::fromStdString(step.title + "\n" + step.detail),
                           this);
    } else {
        QToolTip::hideText();
    }
    update();
}

void PacketFlowCanvas::leaveEvent(QEvent*) {
    hovered_index_ = -1;
    QToolTip::hideText();
    update();
}

void PacketFlowCanvas::update_node_rects(const QRect& bounds) {
    node_rects_.clear();
    const auto* journey = primary_journey();
    if (journey == nullptr || journey->steps.empty()) {
        return;
    }
    const int count = static_cast<int>(journey->steps.size());
    const int gap = 14;
    const int available = bounds.width() - gap * (count - 1);
    const int node_width = std::max(86, available / count);
    const int node_height = 86;
    const int top = bounds.top() + 58;
    int x = bounds.left();
    for (int i = 0; i < count; ++i) {
        node_rects_.push_back(QRect(x, top, node_width, node_height));
        x += node_width + gap;
    }
}

const PacketFlowJourney* PacketFlowCanvas::primary_journey() const {
    if (snapshot_.journeys.empty()) {
        return nullptr;
    }
    return &snapshot_.journeys.front();
}
