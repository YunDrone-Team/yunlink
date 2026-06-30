#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_CANVAS_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_CANVAS_HPP

#include <vector>

#include <QRect>
#include <QWidget>

#include "packets/flow/packet_flow_model.hpp"

class PacketFlowCanvas : public QWidget {
  public:
    explicit PacketFlowCanvas(QWidget* parent = nullptr);

    void set_snapshot(PacketFlowSnapshot snapshot);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

  private:
    PacketFlowSnapshot snapshot_;
    std::vector<QRect> node_rects_;
    int hovered_index_{-1};

    void update_node_rects(const QRect& bounds);
    const PacketFlowJourney* primary_journey() const;
};

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_CANVAS_HPP
